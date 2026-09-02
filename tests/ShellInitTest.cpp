#include <catch2/catch_test_macros.hpp>
#include <optional>
#include <string>

#include "shell/ShellInit.h"

TEST_CASE("initScript supports the three shells", "[ShellInit]") {
  REQUIRE(shell::initScript("zsh").has_value());
  REQUIRE(shell::initScript("bash").has_value());
  REQUIRE(shell::initScript("fish").has_value());
}

TEST_CASE("initScript rejects anything else", "[ShellInit]") {
  REQUIRE(shell::initScript("tcsh") == std::nullopt);
  REQUIRE(shell::initScript("") == std::nullopt);
  REQUIRE(shell::initScript("ZSH") == std::nullopt);
}

TEST_CASE("each script defines cd and records on directory change",
          "[ShellInit]") {
  for (const char *name : {"zsh", "bash", "fish"}) {
    std::string script = *shell::initScript(name);

    INFO("shell: " << name);
    REQUIRE(script.find("cdeez add") != std::string::npos);
    REQUIRE(script.find("cdeez query") != std::string::npos);
    REQUIRE(script.find("cd") != std::string::npos);
  }
}

TEST_CASE("each script hooks the shell's directory-change mechanism",
          "[ShellInit]") {
  REQUIRE(shell::initScript("zsh")->find("chpwd_functions") !=
          std::string::npos);
  REQUIRE(shell::initScript("bash")->find("PROMPT_COMMAND") !=
          std::string::npos);
  REQUIRE(shell::initScript("fish")->find("--on-variable PWD") !=
          std::string::npos);
}

TEST_CASE("each script registers completions", "[ShellInit]") {
  REQUIRE(shell::initScript("zsh")->find("compdef") != std::string::npos);
  REQUIRE(shell::initScript("bash")->find("complete -F") != std::string::npos);
  REQUIRE(shell::initScript("fish")->find("complete -c cdeez") !=
          std::string::npos);
}
