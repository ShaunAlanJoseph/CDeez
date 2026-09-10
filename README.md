# cdeez

[![CI](https://github.com/ShaunAlanJoseph/CDeez/actions/workflows/ci.yml/badge.svg)](https://github.com/ShaunAlanJoseph/CDeez/actions/workflows/ci.yml)

A `cd` that learns where you go, and forgives how you type.

```
~/Projects/website $ cd dwnlds
~/Downloads $ cd fuzzy
~/Projects/cdeez/src/fuzzy $ cd muzic
~/Music $
```

`cdeez` records every directory you visit and ranks them by how often and how
recently you were there. Typing a fragment of a directory name jumps you to the
best match — and unlike tools that require an exact substring, it tolerates
typos.

## Install

Requires a C++20 compiler, CMake 3.16 or newer, and SQLite3. CI builds
against CMake 3.28 (Ubuntu 24.04) and 4.4 (Arch).

```sh
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
cmake --install build --prefix ~/.local
```

Then add the shell integration to your shell's config:

```sh
# ~/.zshrc
eval "$(cdeez init zsh)"

# ~/.bashrc
eval "$(cdeez init bash)"

# ~/.config/fish/config.fish
cdeez init fish | source
```

That replaces `cd` with a wrapper and registers a hook that records every
directory change. Everything `cd` already did still works — no arguments, `-`,
`-2`, `-P`, relative and absolute paths all go straight through to the shell's
own `cd`. `cdeez` is consulted only when the argument isn't a directory that
exists.

Run `cdeez init zsh` on its own to read the script before you eval it.

## Usage

The shell function is the interface. The binary underneath has two commands:

| Command | Effect |
|---|---|
| `cdeez add <path>` | Record a visit. Called by the `chpwd` hook. |
| `cdeez query [--exclude <path>] [--list] <term>...` | Print the best match to stdout, or exit 1. `--list` prints every match. |
| `cdeez init <shell>` | Print the integration script for zsh, bash or fish. |
| `cdeez list` | Print every known directory, most frecent first. |
| `cdeez import` | Read directories from stdin and record them. |
| `cdeez --help`, `--version` | Usage and version. |

Exit codes: `0` resolved, `1` no match, `2` usage error, `3` database error.

Data lives in `$XDG_DATA_HOME/cdeez/db.sqlite3`, falling back to
`~/.local/share/cdeez/db.sqlite3`.

## Coming from zoxide

An empty database makes cdeez worse than whatever you're replacing, so bring
your history with you:

```sh
zoxide query --list --score | cdeez import
```

`import` reads one directory per line from stdin, optionally preceded by a
score, and skips anything that no longer exists. That format is what `zoxide
query --list --score` emits, but a plain list of paths works too — so the same
command imports from `z`, autojump, or `find`.

## How ranking works

Every candidate gets a **frecency** score — visit count scaled by how recently
it was seen:

| Last visited | Multiplier |
|---|---|
| within an hour | ×4 |
| within a day | ×2 |
| within a week | ×1 |
| within a month | ×0.5 |
| older | ×0.25 |

That score is then multiplied by the strength of the textual match. Four
matchers run as a **cascade**: each candidate is claimed by the first tier that
matches it, and the tier determines the weight.

| Tier | Matcher | Matches | Weight |
|---|---|---|---|
| 1 | Suffix | the path ends with the query | ×4 |
| 2 | Knuth–Morris–Pratt | the query appears anywhere in the path | ×3 |
| 3 | Subsequence, per path segment | the query's letters appear in order | ×2 |
| 4 | Damerau–Levenshtein | the query is within an edit distance of the basename | ×1 |

Matching is case-insensitive. An exact-case match still wins a tie against one
that only matched after lowering, so `cd Work` and `cd work` can pick different
directories when both exist.

Tiers 1–3 are binary: they either match or they don't. Tier 4 is the only one
that returns a graded score, and it's the one that makes typos work:

```
query          ->  result
fuzzy          ->  ~/Projects/cdeez/src/fuzzy
nvim           ->  ~/.config/nvim
dwnlds         ->  ~/Downloads
documnts       ->  ~/Documents
muzic          ->  ~/Music
websit         ->  ~/Projects/website
zzqqwx         ->  (no match, exit 1)
qqq            ->  (no match, exit 1)
```

### Several keywords

Keywords are matched as one `/`-separated query, so each must match a path
segment, in order:

```
cd src fuzzy       ->  ~/Projects/cdeez/src/fuzzy
cd projects websit ->  ~/Projects/website
cd fuzzy src       ->  (no match — order matters)
```

Tier 4 is skipped when the query spans more than one segment, since
Damerau–Levenshtein compares against a single path component and
`documents/taxes` is not a typo of any one directory name.

### Why tier 4 needs a floor

Damerau–Levenshtein similarity is `1 - distance / max(len)`, which is almost
never zero when scored against a full absolute path — any query shares *some*
letters with `/home/you/something`. Without a cut-off, every typo resolved to
whatever directory was most frecent, which is worse than reporting nothing.

Two changes fixed it. The tier scores against the **basename** rather than the
whole path, removing similarity that comes from path length alone; and a
candidate must clear a minimum score to count as matched. The threshold was
picked by measuring the gap:

| | range |
|---|---|
| real typos (`dwnlds`→`downloads`, `confg`→`config`, …) | 0.667 – 0.889 |
| unrelated queries (`zzqqwx`, `wrgnbl`, `music`→`downloads`, …) | 0.000 – 0.250 |

Anything in `(0.250, 0.667]` separates the two cleanly. The cut-off is **0.5**,
biased toward the strict end: a false negative makes you retype, while a false
positive silently drops you in the wrong directory.

### Aging

Visit counts don't grow forever. Once they total 10000, every entry is scaled
down proportionally and anything that falls below a single visit is deleted.
The database stays bounded, and a directory you stopped using a year ago fades
out instead of competing with one you use daily.

### Interactive selection

`cdi` pipes every match through [fzf](https://github.com/junegunn/fzf) instead
of picking one, which is where fuzzy matching earns its keep — an ambiguous
query becomes a menu rather than a wrong guess.

```
cdi work     # every directory matching "work", ranked, pick one
cdi          # every known directory
```

### The current directory is never a result

The shell wrapper passes `--exclude "$PWD"` on every query, so `cd work` from
inside `~/work` takes you to the *next* best match rather than doing nothing.

### Recording is separate from jumping

The shell hook calls `cdeez add` on *every* directory change, however you got
there — `cd`, a script, `pushd`. `cdeez query` never writes to the database, so
a lookup has no side effects and a jump isn't counted twice.

## Development

```sh
cmake -B build -G Ninja
cmake --build build
ctest --test-dir build
```

Tests use Catch2 v3, via the system package if present and `FetchContent`
otherwise. CI builds with `-Wall -Wextra -Wpedantic -Werror` on Ubuntu (gcc and
clang) and Arch, and checks formatting against `.clang-format`.

Pass `-DCDEEZ_BUILD_TESTS=OFF` to build without the test suite.

## License

MIT — see [LICENSE](LICENSE).
