#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <string>

#include "TestSupport.h"
#include "db/db.h"

TEST_CASE("a fresh database is usable immediately", "[DB]") {
  testing::TempDir tmp;

  // Regression: the table was never created, so the first query aborted with
  // "no such table: paths".
  DB db(tmp / "fresh.sqlite3");
  REQUIRE(db.getPaths().empty());
}

TEST_CASE("the database file and its parents are created", "[DB]") {
  testing::TempDir tmp;
  std::string path = tmp / "nested/deeper/db.sqlite3";

  DB db(path);
  REQUIRE(std::filesystem::exists(path));
}

TEST_CASE("upsertPath inserts a new path", "[DB]") {
  testing::TempDir tmp;
  DB db(tmp / "db.sqlite3");

  db.upsertPath("/home/tester/Downloads", 1000);

  std::vector<DB::PathEntry> paths = db.getPaths();
  REQUIRE(paths.size() == 1);
  REQUIRE(paths[0].path == "/home/tester/Downloads");
  REQUIRE(paths[0].access_count == 1);
  REQUIRE(paths[0].last_accessed == 1000);
}

TEST_CASE("upsertPath increments an existing path", "[DB]") {
  testing::TempDir tmp;
  DB db(tmp / "db.sqlite3");

  db.upsertPath("/home/tester/Downloads", 1000);
  db.upsertPath("/home/tester/Downloads", 2000);
  db.upsertPath("/home/tester/Downloads", 3000);

  std::vector<DB::PathEntry> paths = db.getPaths();
  REQUIRE(paths.size() == 1);
  REQUIRE(paths[0].access_count == 3);
  REQUIRE(paths[0].last_accessed == 3000);
}

TEST_CASE("removePath deletes only the named path", "[DB]") {
  testing::TempDir tmp;
  DB db(tmp / "db.sqlite3");

  db.upsertPath("/a", 1000);
  db.upsertPath("/b", 1000);
  db.removePath("/a");

  std::vector<DB::PathEntry> paths = db.getPaths();
  REQUIRE(paths.size() == 1);
  REQUIRE(paths[0].path == "/b");
}

TEST_CASE("removePath is a no-op for an unknown path", "[DB]") {
  testing::TempDir tmp;
  DB db(tmp / "db.sqlite3");

  db.upsertPath("/a", 1000);
  REQUIRE_NOTHROW(db.removePath("/nonexistent"));
  REQUIRE(db.getPaths().size() == 1);
}

TEST_CASE("data survives reopening the database", "[DB]") {
  testing::TempDir tmp;
  std::string path = tmp / "db.sqlite3";

  {
    DB db(path);
    db.upsertPath("/a", 1000);
    db.upsertPath("/a", 2000);
  }

  DB reopened(path);
  std::vector<DB::PathEntry> paths = reopened.getPaths();
  REQUIRE(paths.size() == 1);
  REQUIRE(paths[0].access_count == 2);
}

TEST_CASE("defaultPath follows XDG_DATA_HOME", "[DB]") {
  testing::EnvGuard data("XDG_DATA_HOME", "/custom/data");

  REQUIRE(DB::defaultPath() == "/custom/data/cdeez/db.sqlite3");
}
