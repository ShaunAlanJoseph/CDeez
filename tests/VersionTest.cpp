#include <catch2/catch_test_macros.hpp>
#include <string>

#include "Version.h"

TEST_CASE("the version is substituted at configure time", "[Version]") {
  std::string version = cdeez::VERSION;

  // A broken configure_file leaves the placeholder behind rather than failing,
  // so every build would report "@PROJECT_VERSION@".
  REQUIRE_FALSE(version.empty());
  REQUIRE(version.find('@') == std::string::npos);
}

TEST_CASE("the revision is either a commit or empty", "[Version]") {
  std::string revision = cdeez::REVISION;

  // Empty when built from a release tarball; never the unsubstituted marker.
  REQUIRE(revision.find('@') == std::string::npos);
}
