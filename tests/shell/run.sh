#!/bin/sh
# Behavioural tests for the shell integration.
#
# Every case drives the real `cd` wrapper in a real shell and asserts on the
# directory it lands in. Nothing here inspects the script text: a wrapper that
# contains all the right strings but never changes directory must fail.
#
# Usage: run.sh /path/to/cdeez

set -u

CDEEZ=${1:-}
if [ -z "$CDEEZ" ] || [ ! -x "$CDEEZ" ]; then
  echo "usage: $0 /path/to/cdeez" >&2
  exit 2
fi
CDEEZ=$(cd "$(dirname "$CDEEZ")" && pwd)/$(basename "$CDEEZ")
HERE=$(cd "$(dirname "$0")" && pwd)

pass=0
fail=0
skipped=""
case_n=0

# Matching considers the whole path, so a random temp directory name can
# contain a query string by chance and match legitimately. The sandbox name is
# fixed and chosen to contain none of the queries used below.
SANDBOX=${TMPDIR:-/tmp}/cdz-sh.$$
trap 'rm -rf "$SANDBOX"' EXIT INT TERM

new_home() {
  case_n=$((case_n + 1))
  home=$SANDBOX/h$case_n
  mkdir -p "$home"
  make_home "$home"
  printf '%s' "$home"
}

# Each shell needs its own preamble: how to load the integration, and flags
# that stop it reading the developer's own config.
preamble() {
  case $1 in
    zsh)  echo 'eval "$(cdeez init zsh)"' ;;
    bash) echo 'eval "$(cdeez init bash)"' ;;
    fish) echo 'cdeez init fish | source' ;;
  esac
}

run_shell() {
  case $1 in
    zsh)  zsh -f "$2" ;;
    bash) bash --noprofile --norc "$2" ;;
    fish) fish --no-config "$2" ;;
  esac
}

make_home() {
  mkdir -p "$1/downloads" "$1/music" "$1/cfg" \
           "$1/projects/website" "$1/projects/cdeez/src/fuzzy" \
           "$1/cp/codeforces/div2" "$1/work" "$1/workspace" "$1/my documents"
}

for shell in zsh bash fish; do
  if ! command -v "$shell" >/dev/null 2>&1; then
    skipped="$skipped $shell"
    continue
  fi

  while IFS='|' read -r name script expected only; do
    case $name in ''|\#*) continue ;; esac
    # A case may restrict itself to shells whose semantics it relies on.
    if [ -n "${only:-}" ]; then
      case " $only " in *" $shell "*) ;; *) continue ;; esac
    fi

    home=$(new_home)

    script_file=$home/case
    {
      preamble "$shell"
      echo "cd \"\$HOME\""
      echo "$script"
      printf 'printf "%%s\\n" "$PWD"\n'
    } > "$script_file"

    actual=$(HOME=$home PATH=$(dirname "$CDEEZ"):$PATH run_shell "$shell" "$script_file" 2>/dev/null | tail -1)

    if [ "$expected" = "=" ]; then
      want=$home
    else
      want=$home/$expected
    fi

    if [ "$actual" = "$want" ]; then
      pass=$((pass + 1))
    else
      fail=$((fail + 1))
      printf '  FAIL  %-28s %s\n' "$shell" "$name"
      printf '          expected: %s\n' "${want#"$home"/}"
      printf '          actual:   %s\n' "${actual#"$home"/}"
    fi

  done < "$HERE/cases.txt"
done

# The recording hook differs per shell, so it gets its own check rather than a
# shared case: bash records from PROMPT_COMMAND, which a script never triggers.
for shell in zsh fish; do
  command -v "$shell" >/dev/null 2>&1 || continue

  home=$(new_home)
  script_file=$home/hook
  {
    preamble "$shell"
    echo 'cd "$HOME/downloads"'
    echo 'cd "$HOME"'
    echo 'cdeez list'
  } > "$script_file"

  if HOME=$home PATH=$(dirname "$CDEEZ"):$PATH run_shell "$shell" "$script_file" 2>/dev/null \
       | grep -q "downloads"; then
    pass=$((pass + 1))
  else
    fail=$((fail + 1))
    printf '  FAIL  %-28s %s\n' "$shell" "hook records a directory change"
  fi
done

[ -n "$skipped" ] && printf '  skipped (not installed):%s\n' "$skipped"
printf '  %d passed, %d failed\n' "$pass" "$fail"
[ "$fail" -eq 0 ]
