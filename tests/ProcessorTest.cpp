#include <catch2/catch_test_macros.hpp>
#include <ctime>
#include <filesystem>
#include <optional>
#include <sstream>
#include <string>
#include <utility>
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

  std::vector<std::string> matches = processor.rank({"work"});
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
  std::vector<std::string> matches = processor.rank({"work"}, here);
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

  REQUIRE(processor.resolve({"work"}) == here);
  REQUIRE(processor.resolve({"work"}, here) == elsewhere);
}

TEST_CASE("excluding the only match yields nothing", "[Processor]") {
  testing::TempDir tmp;
  DB db(tmp / "db.sqlite3");
  Processor processor(db);

  std::string only = tmp.makeDir("work");
  processor.add(only);

  REQUIRE(processor.resolve({"work"}, only) == std::nullopt);
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

  std::vector<std::string> matches = processor.rank({"work"});
  REQUIRE(matches.size() == 1);
  REQUIRE(matches[0] == live);
}

TEST_CASE("import reads the score-and-path format", "[Processor]") {
  testing::TempDir tmp;
  DB db(tmp / "db.sqlite3");
  Processor processor(db);

  std::string alpha = tmp.makeDir("alpha");
  std::string beta = tmp.makeDir("beta");

  // This is exactly what `zoxide query --list --score` emits.
  std::istringstream input("   12.5 " + alpha + "\n    4.0 " + beta + "\n");
  Processor::ImportResult result = processor.import(input);

  REQUIRE(result.imported == 2);
  REQUIRE(result.skipped == 0);

  std::vector<DB::PathEntry> paths = db.getPaths();
  REQUIRE(paths.size() == 2);
  for (const auto &entry : paths) {
    if (entry.path == alpha)
      REQUIRE(entry.access_count == 13); // rounded
    else
      REQUIRE(entry.access_count == 4);
  }
}

TEST_CASE("import reads bare paths", "[Processor]") {
  testing::TempDir tmp;
  DB db(tmp / "db.sqlite3");
  Processor processor(db);

  std::string alpha = tmp.makeDir("alpha");

  std::istringstream input(alpha + "\n");
  REQUIRE(processor.import(input).imported == 1);
  REQUIRE(db.getPaths()[0].access_count == 1);
}

TEST_CASE("import skips directories that do not exist", "[Processor]") {
  testing::TempDir tmp;
  DB db(tmp / "db.sqlite3");
  Processor processor(db);

  std::istringstream input("   9.0 " + std::string(tmp / "missing") + "\n");
  Processor::ImportResult result = processor.import(input);

  REQUIRE(result.imported == 0);
  REQUIRE(result.skipped == 1);
  REQUIRE(db.getPaths().empty());
}

TEST_CASE("import ignores blank lines and surrounding space", "[Processor]") {
  testing::TempDir tmp;
  DB db(tmp / "db.sqlite3");
  Processor processor(db);

  std::string alpha = tmp.makeDir("alpha");

  std::istringstream input("\n\n   " + alpha + "   \n\n");
  REQUIRE(processor.import(input).imported == 1);
}

TEST_CASE("import merges with visits already recorded", "[Processor]") {
  testing::TempDir tmp;
  DB db(tmp / "db.sqlite3");
  Processor processor(db);

  std::string alpha = tmp.makeDir("alpha");
  processor.add(alpha);
  processor.add(alpha);

  std::istringstream input("   5.0 " + alpha + "\n");
  processor.import(input);

  REQUIRE(db.getPaths()[0].access_count == 7);
}

TEST_CASE("import honours the home and root exclusions", "[Processor]") {
  testing::TempDir tmp;
  testing::EnvGuard home("HOME", tmp.path().c_str());
  DB db(tmp / "db.sqlite3");
  Processor processor(db);

  std::istringstream input("   9.0 " + tmp.path().string() + "\n   9.0 /\n");
  Processor::ImportResult result = processor.import(input);

  REQUIRE(result.imported == 0);
  REQUIRE(result.skipped == 2);
}

TEST_CASE("tag records a shorthand for a directory", "[Processor][tags]") {
  testing::TempDir tmp;
  DB db(tmp / "db.sqlite3");
  Processor processor(db);

  std::string cf = tmp.makeDir("cp/codeforces");
  processor.tag("cf", cf);

  REQUIRE(processor.tags() ==
          std::vector<std::pair<std::string, std::string>>{{"cf", cf}});
  REQUIRE(processor.resolve({"cf"}) == cf);
}

TEST_CASE("tags are matched exactly, ignoring case", "[Processor][tags]") {
  testing::TempDir tmp;
  DB db(tmp / "db.sqlite3");
  Processor processor(db);

  processor.tag("CF", tmp.makeDir("cp/codeforces"));

  REQUIRE(processor.resolve({"cf"}).has_value());
  REQUIRE(processor.resolve({"CF"}).has_value());
  // A tag is a deliberate shorthand, so it is never matched fuzzily.
  REQUIRE(processor.resolve({"cff"}) == std::nullopt);
}

TEST_CASE("a tag beats a fuzzy match", "[Processor][tags]") {
  testing::TempDir tmp;
  DB db(tmp / "db.sqlite3");
  Processor processor(db);

  std::string tagged = tmp.makeDir("cp/codeforces");
  std::string frecent = tmp.makeDir("cfg");
  for (int i = 0; i < 20; ++i)
    processor.add(frecent);

  processor.tag("cf", tagged);

  REQUIRE(processor.resolve({"cf"}) == tagged);
}

TEST_CASE("a tag scopes the search to its own subtree", "[Processor][tags]") {
  testing::TempDir tmp;
  DB db(tmp / "db.sqlite3");
  Processor processor(db);

  std::string cf = tmp.makeDir("cp/codeforces");
  std::string inside = tmp.makeDir("cp/codeforces/div2");
  std::string outside = tmp.makeDir("elsewhere/div2");

  processor.tag("cf", cf);
  processor.add(inside);
  for (int i = 0; i < 20; ++i)
    processor.add(outside);

  // The far more frecent match outside the tag must not win.
  REQUIRE(processor.resolve({"cf", "div2"}) == inside);
}

TEST_CASE("a scoped search that finds nothing does not fall back",
          "[Processor][tags]") {
  testing::TempDir tmp;
  DB db(tmp / "db.sqlite3");
  Processor processor(db);

  processor.tag("cf", tmp.makeDir("cp/codeforces"));
  std::string outside = tmp.makeDir("elsewhere/notes");
  processor.add(outside);

  // Answering a different question than the one asked is how you end up in
  // the wrong directory.
  REQUIRE(processor.resolve({"cf", "notes"}) == std::nullopt);
}

TEST_CASE("a tag finds an unvisited directory beneath it",
          "[Processor][tags]") {
  testing::TempDir tmp;
  DB db(tmp / "db.sqlite3");
  Processor processor(db);

  std::string cf = tmp.makeDir("cp/codeforces");
  std::string never = tmp.makeDir("cp/codeforces/div2");
  processor.tag("cf", cf);

  // div2 was never recorded, so only the literal-path check can find it.
  REQUIRE(processor.resolve({"cf", "div2"}) == never);
}

TEST_CASE("a broken tag is reported, not silently ignored",
          "[Processor][tags]") {
  testing::TempDir tmp;
  DB db(tmp / "db.sqlite3");
  Processor processor(db);

  std::string cf = tmp.makeDir("cp/codeforces");
  processor.tag("cf", cf);
  std::filesystem::remove_all(cf);

  REQUIRE_THROWS_AS(processor.resolve({"cf"}), std::runtime_error);
  // The tag survives: the target may be an unmounted drive.
  REQUIRE(processor.tags().size() == 1);
}

TEST_CASE("tagging something that is not a directory fails",
          "[Processor][tags]") {
  testing::TempDir tmp;
  DB db(tmp / "db.sqlite3");
  Processor processor(db);

  REQUIRE_THROWS_AS(processor.tag("cf", tmp / "missing"), std::runtime_error);
  REQUIRE(processor.tags().empty());
}

TEST_CASE("retagging moves the shorthand", "[Processor][tags]") {
  testing::TempDir tmp;
  DB db(tmp / "db.sqlite3");
  Processor processor(db);

  processor.tag("cf", tmp.makeDir("old"));
  std::string current = tmp.makeDir("new");
  processor.tag("cf", current);

  REQUIRE(processor.tags().size() == 1);
  REQUIRE(processor.resolve({"cf"}) == current);
}

TEST_CASE("untag removes the shorthand", "[Processor][tags]") {
  testing::TempDir tmp;
  DB db(tmp / "db.sqlite3");
  Processor processor(db);

  processor.tag("cf", tmp.makeDir("cp/codeforces"));
  processor.untag("cf");

  REQUIRE(processor.tags().empty());
  REQUIRE(processor.resolve({"cf"}) == std::nullopt);
}

TEST_CASE("aging never removes a tag", "[Processor][tags]") {
  testing::TempDir tmp;
  DB db(tmp / "db.sqlite3");
  Processor processor(db, 10);

  processor.tag("cf", tmp.makeDir("cp/codeforces"));

  std::string busy = tmp.makeDir("busy");
  for (int i = 0; i < 30; ++i)
    processor.add(busy);

  // Tags are deliberate, so they live outside the frecency database.
  REQUIRE(processor.tags().size() == 1);
  REQUIRE(processor.resolve({"cf"}).has_value());
}
