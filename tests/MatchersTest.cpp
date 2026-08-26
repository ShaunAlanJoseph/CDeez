#include <catch2/catch_test_macros.hpp>
#include <string>

#include "fuzzy/DamerauLevenshteinMatcher.h"
#include "fuzzy/KMPMatcher.h"
#include "fuzzy/SequenceMatcher.h"
#include "fuzzy/SuffixMatcher.h"
#include "fuzzy/TokenizedMatcher.h"

TEST_CASE("SuffixMatcher only matches at the end", "[matchers]") {
  REQUIRE(SuffixMatcher("Downloads").score("/home/shxun/Downloads") == 1.0);
  REQUIRE(SuffixMatcher("loads").score("/home/shxun/Downloads") == 1.0);
  REQUIRE(SuffixMatcher("Down").score("/home/shxun/Downloads") == 0.0);
  REQUIRE(SuffixMatcher("shxun").score("/home/shxun/Downloads") == 0.0);
}

TEST_CASE("SuffixMatcher handles a needle longer than the haystack",
          "[matchers]") {
  REQUIRE(SuffixMatcher("/home/shxun/Downloads").score("Downloads") == 0.0);
}

TEST_CASE("KMPMatcher finds a substring anywhere", "[matchers]") {
  REQUIRE(KMPMatcher("shxun").score("/home/shxun/Downloads") == 1.0);
  REQUIRE(KMPMatcher("/home").score("/home/shxun/Downloads") == 1.0);
  REQUIRE(KMPMatcher("Downloads").score("/home/shxun/Downloads") == 1.0);
  REQUIRE(KMPMatcher("nope").score("/home/shxun/Downloads") == 0.0);
}

TEST_CASE("KMPMatcher handles repeated prefixes", "[matchers]") {
  REQUIRE(KMPMatcher("aaab").score("aaaaab") == 1.0);
  REQUIRE(KMPMatcher("aaab").score("aaaaa") == 0.0);
}

TEST_CASE("SequenceMatcher matches subsequences, favouring clusters",
          "[matchers]") {
  REQUIRE(SequenceMatcher("dwn").score("Downloads") == 0.0);
  REQUIRE(SequenceMatcher("dwn").score("downloads") > 0.0);
  REQUIRE(SequenceMatcher("nope").score("downloads") == 0.0);

  double clustered = SequenceMatcher("load").score("downloads");
  double scattered = SequenceMatcher("dnla").score("downloads");
  REQUIRE(clustered > scattered);
}

TEST_CASE("SequenceMatcher rejects a needle longer than the haystack",
          "[matchers]") {
  REQUIRE(SequenceMatcher("downloads").score("dl") == 0.0);
}

TEST_CASE("TokenizedMatcher matches tokens in order", "[matchers]") {
  REQUIRE(TokenizedLevenshteinMatcher("shxun/downloads")
              .score("/home/shxun/downloads") == 1.0);
  REQUIRE(TokenizedLevenshteinMatcher("downloads/shxun")
              .score("/home/shxun/downloads") == 0.0);
}

TEST_CASE("DamerauLevenshteinMatcher scores similarity", "[matchers]") {
  REQUIRE(DamerauLevenshteinMatcher("downloads").score("downloads") == 1.0);
  REQUIRE(DamerauLevenshteinMatcher("zzzz").score("aaaa") == 0.0);

  double typo = DamerauLevenshteinMatcher("downlaods").score("downloads");
  double unrelated = DamerauLevenshteinMatcher("music").score("downloads");
  REQUIRE(typo > 0.8);
  REQUIRE(unrelated < typo);
}

TEST_CASE("DamerauLevenshteinMatcher counts a transposition as one edit",
          "[matchers]") {
  double transposed = DamerauLevenshteinMatcher("ab").score("ba");
  double substituted = DamerauLevenshteinMatcher("ab").score("cd");
  REQUIRE(transposed > substituted);
}

TEST_CASE("matchers handle an empty needle", "[matchers]") {
  REQUIRE(DamerauLevenshteinMatcher("").score("downloads") == 0.0);
  REQUIRE(SequenceMatcher("").score("downloads") == 0.0);
}
