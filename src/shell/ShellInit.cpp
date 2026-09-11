#include "ShellInit.h"

#include <optional>
#include <string>

namespace {
  constexpr const char *ZSH_SCRIPT = R"SH(__cdeez_pwd()  { \builtin pwd -L }
__cdeez_cd()   { \builtin cd -- "$@" }
__cdeez_hook() { \command cdeez add "$(__cdeez_pwd)" }

\builtin typeset -ga chpwd_functions
chpwd_functions=("${(@)chpwd_functions:#__cdeez_hook}")
chpwd_functions+=(__cdeez_hook)

cd() {
  if [[ "$#" -eq 0 ]]; then
    __cdeez_cd ~
  elif [[ "$#" -eq 1 && "$1" == "-" ]]; then
    __cdeez_cd "${OLDPWD}"
  elif [[ "$1" == -* && "$1" != "-" ]]; then
    \builtin cd "$@"
  elif [[ "$#" -eq 1 ]] && (\builtin cd -q -- "$1") 2>/dev/null; then
    __cdeez_cd "$1"
  else
    \builtin local result
    result="$(\command cdeez query --exclude "$(__cdeez_pwd)" -- "$@")" \
      && __cdeez_cd "${result}"
  fi
}

# Tagged directories first, annotated with their tag, then everything else by
# frecency, with duplicates dropped.
__cdeez_candidates() {
  \command cdeez tags | \command awk -F'\t' '{ print $2 "\t[" $1 "]" }'
  \command cdeez list
}

cdi() {
  if ! \command -v fzf >/dev/null 2>&1; then
    \builtin print -u2 "cdi: fzf is not installed"
    return 1
  fi

  \builtin local result
  if [[ "$#" -eq 0 ]]; then
    result="$(__cdeez_candidates | \command awk -F'\t' '!seen[$1]++' \
      | fzf --height 40% --reverse --no-sort --delimiter='\t' --with-nth=1,2 \
      | \command cut -f1)"
  else
    result="$(\command cdeez query --list --exclude "$(__cdeez_pwd)" -- "$@" \
      | fzf --height 40% --reverse --no-sort)"
  fi

  [[ -n "$result" ]] && __cdeez_cd "${result}"
}

if [[ -o zle ]]; then
  __cdeez_complete() {
    if (( CURRENT == 2 )); then
      compadd -- add query init list import tag untag tags
    elif (( CURRENT == 3 )) && [[ "$words[2]" == init ]]; then
      compadd -- zsh bash fish
    elif (( CURRENT == 3 )) && [[ "$words[2]" == untag ]]; then
      compadd -- ${(f)"$(\command cdeez tags | \command cut -f1)"}
    elif (( CURRENT == 3 )) && [[ "$words[2]" == (add|tag) ]]; then
      _files -/
    fi
  }
  compdef __cdeez_complete cdeez

  # Keep the shell's own directory completion and offer tags alongside it.
  __cdeez_cd_complete() {
    _cd
    \builtin local -a cdeez_tags
    cdeez_tags=(${(f)"$(\command cdeez tags | \command cut -f1)"})
    (( ${#cdeez_tags} )) && compadd -a cdeez_tags
  }
  compdef __cdeez_cd_complete cd
fi
)SH";

  constexpr const char *BASH_SCRIPT = R"SH(__cdeez_pwd()  { \builtin pwd -L; }
__cdeez_cd()   { \builtin cd -- "$@"; }
__cdeez_hook() { \command cdeez add "$(__cdeez_pwd)"; }

case "${PROMPT_COMMAND:-}" in
  *__cdeez_hook*) ;;
  *) PROMPT_COMMAND="__cdeez_hook${PROMPT_COMMAND:+;$PROMPT_COMMAND}" ;;
esac

cd() {
  if [ "$#" -eq 0 ]; then
    __cdeez_cd ~
  elif [ "$#" -eq 1 ] && [ "$1" = "-" ]; then
    __cdeez_cd "${OLDPWD}"
  elif [ "${1#-}" != "$1" ] && [ "$1" != "-" ]; then
    \builtin cd "$@"
  elif [ "$#" -eq 1 ] && (\builtin cd -- "$1") >/dev/null 2>&1; then
    __cdeez_cd "$1"
  else
    local result
    result="$(\command cdeez query --exclude "$(__cdeez_pwd)" -- "$@")" \
      && __cdeez_cd "${result}"
  fi
}

__cdeez_candidates() {
  \command cdeez tags | \command awk -F'\t' '{ print $2 "\t[" $1 "]" }'
  \command cdeez list
}

cdi() {
  if ! \command -v fzf >/dev/null 2>&1; then
    echo "cdi: fzf is not installed" >&2
    return 1
  fi

  local result
  if [ "$#" -eq 0 ]; then
    result="$(__cdeez_candidates | \command awk -F'\t' '!seen[$1]++' \
      | fzf --height 40% --reverse --no-sort --delimiter='\t' --with-nth=1,2 \
      | \command cut -f1)"
  else
    result="$(\command cdeez query --list --exclude "$(__cdeez_pwd)" -- "$@" \
      | fzf --height 40% --reverse --no-sort)"
  fi

  [ -n "$result" ] && __cdeez_cd "${result}"
}

__cdeez_complete() {
  local cur="${COMP_WORDS[COMP_CWORD]}"
  local prev="${COMP_WORDS[COMP_CWORD-1]}"
  if [ "$COMP_CWORD" -eq 1 ]; then
    COMPREPLY=($(compgen -W "add query init list import tag untag tags" -- "$cur"))
  elif [ "$prev" = "init" ]; then
    COMPREPLY=($(compgen -W "zsh bash fish" -- "$cur"))
  elif [ "$prev" = "untag" ]; then
    COMPREPLY=($(compgen -W "$(\command cdeez tags | \command cut -f1)" -- "$cur"))
  elif [ "$prev" = "add" ] || [ "$prev" = "tag" ]; then
    COMPREPLY=($(compgen -d -- "$cur"))
  fi
}
complete -F __cdeez_complete cdeez

# -o dirnames keeps normal directory completion; the tags are offered too.
__cdeez_cd_complete() {
  local cur="${COMP_WORDS[COMP_CWORD]}"
  COMPREPLY=($(compgen -W "$(\command cdeez tags | \command cut -f1)" -- "$cur"))
}
complete -o dirnames -F __cdeez_cd_complete cd
)SH";

  constexpr const char *FISH_SCRIPT =
      R"SH(# Re-source fish's own cd under a new name, so that `cd -`, $CDPATH and the
# directory history keep working and we only add the fuzzy fallback.
if status get-file functions/cd.fish >/dev/null 2>&1
    status get-file functions/cd.fish \
        | string replace --regex -- '^function cd\s' 'function __cdeez_cd ' | source
else
    string replace --regex -- '^function cd\s' 'function __cdeez_cd ' \
        <$__fish_data_dir/functions/cd.fish | source
end

function __cdeez_hook --on-variable PWD
    command cdeez add "$PWD"
end

function cd
    if test (count $argv) -ge 1
        and not string match -q -- '-*' $argv[1]
        and not test (count $argv) -eq 1 -a -d "$argv[1]"
        set -l result (command cdeez query --exclude "$PWD" -- $argv)
        and __cdeez_cd $result
        return $status
    end

    __cdeez_cd $argv
end

function cdi
    if not command -v fzf >/dev/null 2>&1
        echo "cdi: fzf is not installed" >&2
        return 1
    end

    set -l result
    if test (count $argv) -eq 0
        set result (begin
                command cdeez tags | awk -F'\t' '{ print $2 "\t[" $1 "]" }'
                command cdeez list
            end | awk -F'\t' '!seen[$1]++' \
            | fzf --height 40% --reverse --no-sort --delimiter='\t' --with-nth=1,2 \
            | cut -f1)
    else
        set result (command cdeez query --list --exclude "$PWD" -- $argv \
            | fzf --height 40% --reverse --no-sort)
    end

    test -n "$result"
    and __cdeez_cd $result
end

complete -c cdeez -f
complete -c cdeez -n __fish_use_subcommand -a add   -d 'Record a visit'
complete -c cdeez -n __fish_use_subcommand -a query -d 'Print the best match'
complete -c cdeez -n __fish_use_subcommand -a init  -d 'Print shell integration'
complete -c cdeez -n __fish_use_subcommand -a list  -d 'List known directories'
complete -c cdeez -n '__fish_seen_subcommand_from init' -a 'zsh bash fish'
complete -c cdeez -n __fish_use_subcommand -a import -d 'Import directories from stdin'
complete -c cdeez -n __fish_use_subcommand -a tag   -d 'Tag a directory'
complete -c cdeez -n __fish_use_subcommand -a untag -d 'Remove a tag'
complete -c cdeez -n __fish_use_subcommand -a tags  -d 'List tags'
complete -c cdeez -n '__fish_seen_subcommand_from add tag' -a '(__fish_complete_directories)'
complete -c cdeez -n '__fish_seen_subcommand_from untag' -a '(command cdeez tags | cut -f1)'

# Offered alongside fish's own directory completion.
complete -c cdeez -n 'false'
complete -c cd -a '(command cdeez tags | cut -f1)' -d 'cdeez tag'
)SH";
} // namespace

std::optional<std::string> shell::initScript(const std::string &shell) {
  /*
  @brief Builds the shell integration script.
  @param shell One of "zsh", "bash" or "fish".
  @param command The name to define, alongside that name plus "i".
  @return The script, or nullopt if the shell is unsupported.
  */
  if (shell == "zsh")
    return ZSH_SCRIPT;
  if (shell == "bash")
    return BASH_SCRIPT;
  if (shell == "fish")
    return FISH_SCRIPT;
  return std::nullopt;
}
