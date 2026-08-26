#include <catch2/catch_test_macros.hpp>
#include <string>
#include <vector>

#include "utils/StringUtils.h"

TEST_CASE("tokenize splits on the delimiter", "[StringUtils]") {
  REQUIRE(utils::tokenize("/home/shxun/Projects") ==
          std::vector<std::string>{"", "home", "shxun", "Projects"});
  REQUIRE(utils::tokenize("home/shxun") ==
          std::vector<std::string>{"home", "shxun"});
  REQUIRE(utils::tokenize("a:b:c", ':') ==
          std::vector<std::string>{"a", "b", "c"});
}

TEST_CASE("tokenize drops only a trailing empty token", "[StringUtils]") {
  REQUIRE(utils::tokenize("home/") == std::vector<std::string>{"home"});
  REQUIRE(utils::tokenize("/") == std::vector<std::string>{""});
  REQUIRE(utils::tokenize("").empty());
}

TEST_CASE("baseName returns the last path component", "[StringUtils]") {
  REQUIRE(utils::baseName("/home/shxun/Downloads") == "Downloads");
  REQUIRE(utils::baseName("/home") == "home");
  REQUIRE(utils::baseName("Downloads") == "Downloads");
}

TEST_CASE("baseName handles roots and empty input", "[StringUtils]") {
  REQUIRE(utils::baseName("/") == "");
  REQUIRE(utils::baseName("") == "");
}
