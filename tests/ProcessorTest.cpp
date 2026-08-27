#include <catch2/catch_test_macros.hpp>
#include <ctime>
#include <filesystem>
#include <optional>
#include <string>

#include "TestSupport.h"
#include "core/Processor.h"
#include "db/db.h"

TEST_CASE("add records a directory", "[Processor]") {
  testing::TempDir tmp;
  DB db(tmp / "db.sqlite3");
  Processor processor(db);

  std::string dir = tmp.makeDir("downloads");
  processor.add(dir);

  std::vector<DB::PathEntry> paths = db.getPaths();
  REQUIRE(paths.size() == 1);
  REQUIRE(paths[0].path == dir);
}

TEST_CASE("add normalizes before storing", "[Processor]") {
  testing::TempDir tmp;
  DB db(tmp / "db.sqlite3");
  Processor processor(db);

  std::string dir = tmp.makeDir("projects/deep");
  processor.add(dir + "/../deep/");

  REQUIRE(db.getPaths().size() == 1);
  REQUIRE(db.getPaths()[0].path == dir);
}

TEST_CASE("add rejects anything that is not a directory", "[Processor]") {
  testing::TempDir tmp;
  DB db(tmp / "db.sqlite3");
  Processor processor(db);

  REQUIRE_THROWS_AS(processor.add(tmp / "nonexistent"), std::runtime_error);
  REQUIRE(db.getPaths().empty());
}

TEST_CASE("add silently skips home and root", "[Processor]") {
  testing::TempDir tmp;
  testing::EnvGuard home("HOME", tmp.path().c_str());
  DB db(tmp / "db.sqlite3");
  Processor processor(db);

  // Silent rather than throwing: the shell hook runs on every directory
  // change, so an error here would appear at the prompt.
  REQUIRE_NOTHROW(processor.add(tmp.path().string()));
  REQUIRE_NOTHROW(processor.add("/"));
  REQUIRE(db.getPaths().empty());
}

TEST_CASE("resolve finds nothing in an empty database", "[Processor]") {
  testing::TempDir tmp;
  DB db(tmp / "db.sqlite3");
  Processor processor(db);

  REQUIRE(processor.resolve({"anything"}) == std::nullopt);
}

TEST_CASE("resolve matches a basename", "[Processor]") {
  testing::TempDir tmp;
  DB db(tmp / "db.sqlite3");
  Processor processor(db);

  std::string downloads = tmp.makeDir("downloads");
  tmp.makeDir("music");
  processor.add(downloads);
  processor.add(tmp / "music");

  REQUIRE(processor.resolve({"downloads"}) == downloads);
}

TEST_CASE("resolve tolerates typos", "[Processor]") {
  testing::TempDir tmp;
  DB db(tmp / "db.sqlite3");
  Processor processor(db);

  std::string downloads = tmp.makeDir("downloads");
  processor.add(downloads);

  REQUIRE(processor.resolve({"dwnlds"}) == downloads);
  REQUIRE(processor.resolve({"downlaods"}) == downloads);
}

TEST_CASE("resolve rejects a query that matches nothing", "[Processor]") {
  testing::TempDir tmp;
  DB db(tmp / "db.sqlite3");
  Processor processor(db);

  processor.add(tmp.makeDir("downloads"));

  // Regression: scoring the needle against the whole path meant any query
  // matched, so a typo silently jumped to the most frecent directory.
  REQUIRE(processor.resolve({"zzqqwx"}) == std::nullopt);
  REQUIRE(processor.resolve({"wrgnbl"}) == std::nullopt);
}

TEST_CASE("resolve breaks ties by frecency", "[Processor]") {
  testing::TempDir tmp;
  DB db(tmp / "db.sqlite3");
  Processor processor(db);

  std::string rare = tmp.makeDir("a/work");
  std::string frequent = tmp.makeDir("b/work");

  processor.add(rare);
  for (int i = 0; i < 5; ++i)
    processor.add(frequent);

  REQUIRE(processor.resolve({"work"}) == frequent);
}

TEST_CASE("resolve skips entries whose directory is gone", "[Processor]") {
  testing::TempDir tmp;
  DB db(tmp / "db.sqlite3");
  Processor processor(db);

  std::string stale = tmp.makeDir("a/work");
  std::string live = tmp.makeDir("b/work");

  for (int i = 0; i < 5; ++i)
    processor.add(stale);
  processor.add(live);

  std::filesystem::remove_all(stale);

  // The stale entry outranks the live one, so returning the live path proves
  // the walk continues past candidates that no longer exist.
  REQUIRE(processor.resolve({"work"}) == live);
}

TEST_CASE("resolve does not modify the database", "[Processor]") {
  testing::TempDir tmp;
  DB db(tmp / "db.sqlite3");
  Processor processor(db);

  std::string downloads = tmp.makeDir("downloads");
  processor.add(downloads);

  REQUIRE(processor.resolve({"downloads"}) == downloads);

  // Recording belongs to the shell hook, so a query must leave counts alone.
  std::vector<DB::PathEntry> paths = db.getPaths();
  REQUIRE(paths.size() == 1);
  REQUIRE(paths[0].access_count == 1);
}
