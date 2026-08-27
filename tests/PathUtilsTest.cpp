#include <catch2/catch_test_macros.hpp>
#include <cstdlib>
#include <filesystem>
#include <string>

#include "TestSupport.h"
#include "utils/PathUtils.h"

TEST_CASE("expandHome expands only tilde paths", "[PathUtils]") {
  testing::EnvGuard home("HOME", "/home/tester");

  REQUIRE(utils::expandHome("~") == "/home/tester");
  REQUIRE(utils::expandHome("~/Downloads") == "/home/tester/Downloads");
}

TEST_CASE("expandHome leaves everything else untouched", "[PathUtils]") {
  testing::EnvGuard home("HOME", "/home/tester");

  // Regression: an inverted condition once mangled these into $HOME + path[1:].
  REQUIRE(utils::expandHome("/etc") == "/etc");
  REQUIRE(utils::expandHome("src/main.cpp") == "src/main.cpp");
  REQUIRE(utils::expandHome("") == "");
  REQUIRE(utils::expandHome("~tester") == "~tester");
}

TEST_CASE("normalizePath collapses dot segments", "[PathUtils]") {
  REQUIRE(utils::normalizePath("/usr/share/../share") == "/usr/share");
  REQUIRE(utils::normalizePath("/usr/./share") == "/usr/share");
  REQUIRE(utils::normalizePath("/a/b/../../..") == "/");
}

TEST_CASE("normalizePath strips a trailing slash but never empties the root",
          "[PathUtils]") {
  REQUIRE(utils::normalizePath("/usr/") == "/usr");
  REQUIRE(utils::normalizePath("/") == "/");
  REQUIRE(utils::normalizePath("//") == "/");
}

TEST_CASE("normalizePath makes relative paths absolute", "[PathUtils]") {
  std::filesystem::path cwd = std::filesystem::current_path();

  REQUIRE(utils::normalizePath(".") == cwd.string());
  REQUIRE(utils::normalizePath("sub") == (cwd / "sub").string());
}

TEST_CASE("xdgDataHome honours an absolute XDG_DATA_HOME", "[PathUtils]") {
  testing::EnvGuard data("XDG_DATA_HOME", "/custom/data");

  REQUIRE(utils::xdgDataHome() == "/custom/data");
}

TEST_CASE("xdgDataHome falls back when XDG_DATA_HOME is unusable",
          "[PathUtils]") {
  testing::EnvGuard home("HOME", "/home/tester");

  SECTION("unset") {
    testing::EnvGuard data("XDG_DATA_HOME", nullptr);
    REQUIRE(utils::xdgDataHome() == "/home/tester/.local/share");
  }

  SECTION("empty") {
    testing::EnvGuard data("XDG_DATA_HOME", "");
    REQUIRE(utils::xdgDataHome() == "/home/tester/.local/share");
  }

  SECTION("relative, which the XDG spec says to ignore") {
    testing::EnvGuard data("XDG_DATA_HOME", "relative/data");
    REQUIRE(utils::xdgDataHome() == "/home/tester/.local/share");
  }
}
