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

TEST_CASE("the script defines cd by default", "[ShellInit]") {
  for (const char *name : {"zsh", "bash", "fish"}) {
    std::string script = *shell::initScript(name);

    INFO("shell: " << name);
    REQUIRE(script.find("cdi") != std::string::npos);
  }
}

TEST_CASE("the cmd option renames both commands", "[ShellInit]") {
  std::string zsh = *shell::initScript("zsh", "j");

  REQUIRE(zsh.find("\nj() {") != std::string::npos);
  REQUIRE(zsh.find("\nji() {") != std::string::npos);
  REQUIRE(zsh.find("\ncd() {") == std::string::npos);

  std::string fish = *shell::initScript("fish", "j");
  REQUIRE(fish.find("\nfunction j\n") != std::string::npos);
  REQUIRE(fish.find("\nfunction ji\n") != std::string::npos);
}

TEST_CASE("renaming leaves the real cd alone", "[ShellInit]") {
  std::string script = *shell::initScript("zsh", "j");

  // The wrapper must still delegate to the shell's own cd, or it recurses.
  REQUIRE(script.find("builtin cd") != std::string::npos);
  REQUIRE(script.find("__cdeez_cd") != std::string::npos);
  REQUIRE(script.find("@CMD@") == std::string::npos);
}

TEST_CASE("an unknown shell is rejected with a custom command", "[ShellInit]") {
  REQUIRE(shell::initScript("tcsh", "j") == std::nullopt);
}
