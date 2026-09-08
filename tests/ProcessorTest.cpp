#include <catch2/catch_test_macros.hpp>
#include <ctime>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

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

TEST_CASE("list returns known paths ranked by frecency", "[Processor]") {
  testing::TempDir tmp;
  DB db(tmp / "db.sqlite3");
  Processor processor(db);

  std::string rare = tmp.makeDir("rare");
  std::string frequent = tmp.makeDir("frequent");

  processor.add(rare);
  for (int i = 0; i < 5; ++i)
    processor.add(frequent);

  std::vector<std::string> paths = processor.list();
  REQUIRE(paths.size() == 2);
  REQUIRE(paths[0] == frequent);
  REQUIRE(paths[1] == rare);
}

TEST_CASE("list is empty for a fresh database", "[Processor]") {
  testing::TempDir tmp;
  DB db(tmp / "db.sqlite3");
  Processor processor(db);

  REQUIRE(processor.list().empty());
}

TEST_CASE("add ages the database once visits exceed the cap", "[Processor]") {
  testing::TempDir tmp;
  DB db(tmp / "db.sqlite3");
  Processor processor(db, 10);

  std::string busy = tmp.makeDir("busy");
  for (int i = 0; i < 11; ++i)
    processor.add(busy);

  // Scaled back to the cap rather than growing without bound.
  REQUIRE(db.totalAccessCount() <= 10);
}

TEST_CASE("aging drops directories that fall out of use", "[Processor]") {
  testing::TempDir tmp;
  DB db(tmp / "db.sqlite3");
  Processor processor(db, 10);

  std::string rare = tmp.makeDir("rare");
  std::string busy = tmp.makeDir("busy");

  processor.add(rare);
  for (int i = 0; i < 20; ++i)
    processor.add(busy);

  std::vector<std::string> paths = processor.list();
  REQUIRE(paths.size() == 1);
  REQUIRE(paths[0] == busy);
}

TEST_CASE("no aging happens below the cap", "[Processor]") {
  testing::TempDir tmp;
  DB db(tmp / "db.sqlite3");
  Processor processor(db, 100);

  std::string dir = tmp.makeDir("dir");
  for (int i = 0; i < 5; ++i)
    processor.add(dir);

  REQUIRE(db.totalAccessCount() == 5);
}

TEST_CASE("resolve matches regardless of case", "[Processor]") {
  testing::TempDir tmp;
  DB db(tmp / "db.sqlite3");
  Processor processor(db);

  std::string downloads = tmp.makeDir("Downloads");
  processor.add(downloads);

  REQUIRE(processor.resolve({"downloads"}) == downloads);
  REQUIRE(processor.resolve({"DOWNLOADS"}) == downloads);
  REQUIRE(processor.resolve({"DoWnLoAdS"}) == downloads);
}

TEST_CASE("an exact-case match outranks one that needed lowering",
          "[Processor]") {
  testing::TempDir tmp;
  DB db(tmp / "db.sqlite3");
  Processor processor(db);

  std::string exact = tmp.makeDir("a/work");
  std::string differentCase = tmp.makeDir("b/Work");

  processor.add(exact);
  processor.add(differentCase);

  REQUIRE(processor.resolve({"work"}) == exact);
  REQUIRE(processor.resolve({"Work"}) == differentCase);
}

TEST_CASE("resolve matches path segments in order", "[Processor]") {
  testing::TempDir tmp;
  DB db(tmp / "db.sqlite3");
  Processor processor(db);

  std::string nested = tmp.makeDir("projects/website");
  processor.add(nested);

  REQUIRE(processor.resolve({"projects/website"}) == nested);
  REQUIRE(processor.resolve({"website"}) == nested);
  REQUIRE(processor.resolve({"website/projects"}) == std::nullopt);
}

TEST_CASE("the typo tier is skipped for multi-segment queries", "[Processor]") {
  testing::TempDir tmp;
  DB db(tmp / "db.sqlite3");
  Processor processor(db);

  std::string documents = tmp.makeDir("Documents");
  processor.add(documents);

  // Damerau-Levenshtein compares against one path component, so it must not
  // let "documents/taxes" match the directory "Documents".
  REQUIRE(processor.resolve({"documents/taxes"}) == std::nullopt);
}

TEST_CASE("rank returns every match, best first", "[Processor]") {
  testing::TempDir tmp;
  DB db(tmp / "db.sqlite3");
  Processor processor(db);

  std::string rare = tmp.makeDir("a/work");
  std::string frequent = tmp.makeDir("b/work");
  tmp.makeDir("c/unrelated");
  processor.add(tmp / "c/unrelated");

  processor.add(rare);
  for (int i = 0; i < 5; ++i)
    processor.add(frequent);

  std::vector<std::string> matches = processor.rank("work");
  REQUIRE(matches.size() == 2);
  REQUIRE(matches[0] == frequent);
  REQUIRE(matches[1] == rare);
}

TEST_CASE("rank omits the excluded path", "[Processor]") {
  testing::TempDir tmp;
  DB db(tmp / "db.sqlite3");
  Processor processor(db);

  std::string here = tmp.makeDir("a/work");
  std::string elsewhere = tmp.makeDir("b/work");

  for (int i = 0; i < 5; ++i)
    processor.add(here);
  processor.add(elsewhere);

  // Without this, standing in a directory and querying its own name is a
  // no-op jump that wastes the query.
  std::vector<std::string> matches = processor.rank("work", here);
  REQUIRE(matches.size() == 1);
  REQUIRE(matches[0] == elsewhere);
}

TEST_CASE("resolve honours the exclusion", "[Processor]") {
  testing::TempDir tmp;
  DB db(tmp / "db.sqlite3");
  Processor processor(db);

  std::string here = tmp.makeDir("a/work");
  std::string elsewhere = tmp.makeDir("b/work");

  for (int i = 0; i < 5; ++i)
    processor.add(here);
  processor.add(elsewhere);

  REQUIRE(processor.resolve("work") == here);
  REQUIRE(processor.resolve("work", here) == elsewhere);
}

TEST_CASE("excluding the only match yields nothing", "[Processor]") {
  testing::TempDir tmp;
  DB db(tmp / "db.sqlite3");
  Processor processor(db);

  std::string only = tmp.makeDir("work");
  processor.add(only);

  REQUIRE(processor.resolve("work", only) == std::nullopt);
}

TEST_CASE("rank omits directories that no longer exist", "[Processor]") {
  testing::TempDir tmp;
  DB db(tmp / "db.sqlite3");
  Processor processor(db);

  std::string stale = tmp.makeDir("a/work");
  std::string live = tmp.makeDir("b/work");
  processor.add(stale);
  processor.add(live);

  std::filesystem::remove_all(stale);

  std::vector<std::string> matches = processor.rank("work");
  REQUIRE(matches.size() == 1);
  REQUIRE(matches[0] == live);
}
