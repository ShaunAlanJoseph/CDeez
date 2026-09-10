cdeez(1)

# NAME

cdeez - a cd that learns where you go, and forgives how you type

# SYNOPSIS

*cdeez* add _path_++
*cdeez* query [*--exclude* _path_] [*--list*] _term_...++
*cdeez* init _zsh_|_bash_|_fish_++
*cdeez* list++
*cdeez* import++
*cdeez* --help | --version

# DESCRIPTION

*cdeez* records every directory you visit and ranks them by how often and how
recently you were there. Given a fragment of a directory name it prints the
best match, tolerating typos.

The binary is not meant to be called directly. *cdeez init* prints a shell
integration that replaces *cd* with a wrapper and installs a hook to record
directory changes.

# COMMANDS

*add* _path_
	Record a visit to _path_, which must be an existing directory. Prints
	nothing. The home directory and _/_ are silently ignored.

*query* [*--exclude* _path_] [*--list*] _term_...
	Print the best match to standard output. Exits 1 if nothing matches.
	*--exclude* omits _path_ from the results; the shell integration passes
	the current directory, so a jump never lands where you already are.
	*--list* prints every match, best first, instead of only the best.
	Several terms are joined with _/_, so each must match a path segment in
	order: *cdeez query src fuzzy* is the same as *cdeez query src/fuzzy*.

*init* _shell_
	Print the shell integration script for _zsh_, _bash_ or _fish_.
	The script also registers completions for *cdeez* itself.

*list*
	Print every known directory, most frecent first, one per line.

*import*
	Read directories from standard input, one per line, each optionally
	preceded by a score, and record them. Directories that no longer exist
	are skipped. Counts are added to any already recorded.

	The format matches *zoxide query --list --score*:

	```
	zoxide query --list --score | cdeez import
	```

	A plain list of paths works too, so the same command imports from *z*,
	*autojump*, or *find*(1).

*--help*, *-h*
	Print usage and exit.

*--version*, *-V*
	Print the version and exit.

# SHELL FUNCTIONS

The script from *cdeez init* defines two commands:

*cd* _term_...
	Behaves exactly like the shell's own *cd* whenever the argument is a
	real directory, an option, _-_, or absent. Otherwise it jumps to the
	best match.

*cdi* [_term_...]
	Pipe every match through *fzf*(1) and change to the chosen directory.
	With no arguments, offers every known directory.

# EXIT STATUS

[[ *0*
:< Success.
|  *1*
:  No directory matched the query.
|  *2*
:  Usage error.
|  *3*
:  Database or filesystem error.

# RANKING

Each known directory is scored by *frecency*: its visit count, multiplied by 4
if seen within the hour, 2 within the day, 1 within the week, 0.5 within the
month and 0.25 otherwise.

That score is multiplied by the strength of the textual match. Four matchers
run as a cascade and the first one to match a candidate sets its weight:

[[ *Tier*
:< *Matcher*
:< *Weight*
|  1
:  the path ends with the query
:  4
|  2
:  the query appears anywhere in the path (Knuth-Morris-Pratt)
:  3
|  3
:  the query's letters appear in order within a path segment
:  2
|  4
:  the query is close to the basename (Damerau-Levenshtein)
:  1

Matching is case-insensitive, though an exact-case match wins a tie against one
that only matched after lowering.

Tier 4 requires a similarity of at least 0.5, so an unrecognised query reports
no match rather than jumping to whatever directory is most frecent. It is
skipped entirely when the query spans more than one path segment.

Visit counts do not grow without bound. Once they total 10000, every entry is
scaled down proportionally and anything falling below a single visit is
deleted, so directories you have stopped using fade out.

# FILES

_$XDG_DATA_HOME/cdeez/db.sqlite3_
	The path database. Falls back to _~/.local/share/cdeez/db.sqlite3_ when
	*XDG_DATA_HOME* is unset or not absolute.

# ENVIRONMENT

*XDG_DATA_HOME*
	Base directory for the path database.

*HOME*
	Used to expand _~_ and to decide which directory to exclude from the
	database.

# SEE ALSO

*cd*(1), *zoxide*(1)

# AUTHORS

Shaun Alan Joseph. Source and issue tracker at
https://github.com/ShaunAlanJoseph/CDeez
