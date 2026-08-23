#include "OrderMatches.h"

#include <string>
#include <vector>

#include "DamerauLevenshteinMatcher.h"
#include "KMPMatcher.h"
#include "SuffixMatcher.h"
#include "TokenizedMatcher.h"
#include "utils/StringUtils.h"

namespace {
  constexpr double WEIGHT_SUFFIX = 4.0;
  constexpr double WEIGHT_KMP = 3.0;
  constexpr double WEIGHT_TOKEN = 2.0;
  constexpr double WEIGHT_DAMERAU = 1.0;

  constexpr double MIN_DAMERAU_SCORE = 0.5;
} // namespace

void scoreMatches(const std::string &needle,
                  std::vector<MatchResult> &haystacks) {
  /*
  @brief Scores each haystack string against the needle using multiple matching
  algorithms.
  @param needle The string to match against.
  @param haystacks A vector of MatchResult objects containing haystack strings
  with initial scores.
  */

  // Suffix Matcher
  {
    SuffixMatcher matcher(needle);
    for (auto &haystack : haystacks) {
      if (haystack.matched)
        continue;
      double score = matcher.score(haystack.str);
      if (score > 0.0) {
        haystack.score *= score * WEIGHT_SUFFIX;
        haystack.matched = true;
      }
    }
  }

  // KMP Matcher
  {
    KMPMatcher matcher(needle);
    for (auto &haystack : haystacks) {
      if (haystack.matched)
        continue;
      double score = matcher.score(haystack.str);
      if (score > 0.0) {
        haystack.score *= score * WEIGHT_KMP;
        haystack.matched = true;
      }
    }
  }

  // Tokenized Matcher
  {
    TokenizedLevenshteinMatcher matcher(needle);
    for (auto &haystack : haystacks) {
      if (haystack.matched)
        continue;
      double score = matcher.score(haystack.str);
      if (score > 0.0) {
        haystack.score *= score * WEIGHT_TOKEN;
        haystack.matched = true;
      }
    }
  }

  // Damerau-Levenshtein Matcher
  {
    DamerauLevenshteinMatcher matcher(needle);
    for (auto &haystack : haystacks) {
      if (haystack.matched)
        continue;
      double score = matcher.score(utils::baseName(haystack.str));
      if (score >= MIN_DAMERAU_SCORE) {
        haystack.score *= score * WEIGHT_DAMERAU;
        haystack.matched = true;
      }
    }
  }
}
