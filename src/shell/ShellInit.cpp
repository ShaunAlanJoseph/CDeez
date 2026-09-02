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
    result="$(\command cdeez query "$1")" && __cdeez_cd "${result}"
  fi
}

if [[ -o zle ]]; then
  __cdeez_complete() {
    if (( CURRENT == 2 )); then
      compadd -- add query init list
    elif (( CURRENT == 3 )) && [[ "$words[2]" == init ]]; then
      compadd -- zsh bash fish
    elif (( CURRENT == 3 )) && [[ "$words[2]" == add ]]; then
      _files -/
    fi
  }
  compdef __cdeez_complete cdeez
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
    result="$(\command cdeez query "$1")" && __cdeez_cd "${result}"
  fi
}

__cdeez_complete() {
  local cur="${COMP_WORDS[COMP_CWORD]}"
  local prev="${COMP_WORDS[COMP_CWORD-1]}"
  if [ "$COMP_CWORD" -eq 1 ]; then
    COMPREPLY=($(compgen -W "add query init list" -- "$cur"))
  elif [ "$prev" = "init" ]; then
    COMPREPLY=($(compgen -W "zsh bash fish" -- "$cur"))
  elif [ "$prev" = "add" ]; then
    COMPREPLY=($(compgen -d -- "$cur"))
  fi
}
complete -F __cdeez_complete cdeez
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
    if test (count $argv) -eq 1
        and not string match -q -- '-*' $argv[1]
        and not test -d $argv[1]
        set -l result (command cdeez query $argv[1])
        and __cdeez_cd $result
        return $status
    end

    __cdeez_cd $argv
end

complete -c cdeez -f
complete -c cdeez -n __fish_use_subcommand -a add   -d 'Record a visit'
complete -c cdeez -n __fish_use_subcommand -a query -d 'Print the best match'
complete -c cdeez -n __fish_use_subcommand -a init  -d 'Print shell integration'
complete -c cdeez -n __fish_use_subcommand -a list  -d 'List known directories'
complete -c cdeez -n '__fish_seen_subcommand_from init' -a 'zsh bash fish'
complete -c cdeez -n '__fish_seen_subcommand_from add' -a '(__fish_complete_directories)'
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
