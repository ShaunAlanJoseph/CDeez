#include <sqlite3.h>

#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <stdexcept>
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

TEST_CASE("totalAccessCount sums every entry", "[DB]") {
  testing::TempDir tmp;
  DB db(tmp / "db.sqlite3");

  REQUIRE(db.totalAccessCount() == 0);

  db.upsertPath("/a", 1000);
  db.upsertPath("/a", 1000);
  db.upsertPath("/b", 1000);

  REQUIRE(db.totalAccessCount() == 3);
}

TEST_CASE("ageAll scales counts down", "[DB]") {
  testing::TempDir tmp;
  DB db(tmp / "db.sqlite3");

  for (int i = 0; i < 100; ++i)
    db.upsertPath("/a", 1000);

  db.ageAll(0.5);

  std::vector<DB::PathEntry> paths = db.getPaths();
  REQUIRE(paths.size() == 1);
  REQUIRE(paths[0].access_count == 50);
}

TEST_CASE("ageAll drops entries that fall below one visit", "[DB]") {
  testing::TempDir tmp;
  DB db(tmp / "db.sqlite3");

  for (int i = 0; i < 100; ++i)
    db.upsertPath("/busy", 1000);
  db.upsertPath("/rare", 1000);

  db.ageAll(0.5);

  std::vector<DB::PathEntry> paths = db.getPaths();
  REQUIRE(paths.size() == 1);
  REQUIRE(paths[0].path == "/busy");
}

TEST_CASE("ageAll keeps the stored value integral", "[DB]") {
  testing::TempDir tmp;
  std::string path = tmp / "db.sqlite3";

  {
    DB db(path);
    for (int i = 0; i < 10; ++i)
      db.upsertPath("/a", 1000);
    db.ageAll(0.333);
  }

  // Read the raw storage class: SQLite column types are advisory, so without
  // a CAST the product would be stored as a REAL. Reading through
  // sqlite3_column_int would truncate either way and hide the difference.
  sqlite3 *raw = nullptr;
  REQUIRE(sqlite3_open(path.c_str(), &raw) == SQLITE_OK);

  sqlite3_stmt *stmt = nullptr;
  REQUIRE(sqlite3_prepare_v2(raw, "SELECT typeof(access_count) FROM paths;", -1,
                             &stmt, nullptr) == SQLITE_OK);
  REQUIRE(sqlite3_step(stmt) == SQLITE_ROW);

  std::string type =
      reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
  sqlite3_finalize(stmt);
  sqlite3_close(raw);

  REQUIRE(type == "integer");
}

namespace {
  int rawUserVersion(const std::string &path) {
    sqlite3 *raw = nullptr;
    REQUIRE(sqlite3_open(path.c_str(), &raw) == SQLITE_OK);

    sqlite3_stmt *stmt = nullptr;
    REQUIRE(sqlite3_prepare_v2(raw, "PRAGMA user_version;", -1, &stmt,
                               nullptr) == SQLITE_OK);
    REQUIRE(sqlite3_step(stmt) == SQLITE_ROW);

    int version = sqlite3_column_int(stmt, 0);
    sqlite3_finalize(stmt);
    sqlite3_close(raw);
    return version;
  }

  void runRaw(const std::string &path, const char *sql) {
    sqlite3 *raw = nullptr;
    REQUIRE(sqlite3_open(path.c_str(), &raw) == SQLITE_OK);
    REQUIRE(sqlite3_exec(raw, sql, nullptr, nullptr, nullptr) == SQLITE_OK);
    sqlite3_close(raw);
  }
} // namespace

TEST_CASE("a new database records its schema version", "[DB]") {
  testing::TempDir tmp;
  std::string path = tmp / "db.sqlite3";

  {
    DB db(path);
  }

  REQUIRE(rawUserVersion(path) > 0);
}

TEST_CASE("a database predating versioning is migrated in place", "[DB]") {
  testing::TempDir tmp;
  std::string path = tmp / "db.sqlite3";

  // Exactly what earlier releases left behind: the paths table, with data,
  // and no user_version stamp.
  runRaw(path, "CREATE TABLE paths ("
               "path TEXT PRIMARY KEY, "
               "access_count INTEGER NOT NULL, "
               "last_accessed INTEGER NOT NULL);"
               "INSERT INTO paths VALUES ('/kept', 7, 1000);");
  REQUIRE(rawUserVersion(path) == 0);

  {
    DB db(path);
    std::vector<DB::PathEntry> paths = db.getPaths();
    REQUIRE(paths.size() == 1);
    REQUIRE(paths[0].path == "/kept");
    REQUIRE(paths[0].access_count == 7);
  }

  REQUIRE(rawUserVersion(path) > 0);
}

TEST_CASE("a database from a newer release is refused", "[DB]") {
  testing::TempDir tmp;
  std::string path = tmp / "db.sqlite3";

  {
    DB db(path);
  }
  runRaw(path, "PRAGMA user_version = 99;");

  // Better to refuse than to silently misread a schema we don't know.
  REQUIRE_THROWS_AS(DB(path), std::runtime_error);
}
