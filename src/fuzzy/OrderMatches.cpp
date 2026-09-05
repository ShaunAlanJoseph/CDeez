#include "OrderMatches.h"

#include <cstddef>
#include <string>
#include <vector>

#include "DamerauLevenshteinMatcher.h"
#include "KMPMatcher.h"
#include "MatcherBase.h"
#include "SuffixMatcher.h"
#include "TokenizedMatcher.h"
#include "utils/StringUtils.h"

namespace {
  constexpr double WEIGHT_SUFFIX = 4.0;
  constexpr double WEIGHT_KMP = 3.0;
  constexpr double WEIGHT_TOKEN = 2.0;
  constexpr double WEIGHT_DAMERAU = 1.0;

  constexpr double MIN_DAMERAU_SCORE = 0.5;

  constexpr double CASE_MISMATCH_PENALTY = 0.9;

  constexpr double MIN_BINARY_SCORE = 1.0;
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
  std::string loweredNeedle = utils::toLower(needle);

  std::vector<std::string> lowered;
  lowered.reserve(haystacks.size());
  for (const auto &haystack : haystacks)
    lowered.push_back(utils::toLower(haystack.str));

  std::vector<double> caseFactors;
  caseFactors.reserve(haystacks.size());
  for (const auto &haystack : haystacks)
    caseFactors.push_back(haystack.str.find(needle) != std::string::npos
                              ? 1.0
                              : CASE_MISMATCH_PENALTY);

  auto applyTier = [&](const MatcherBase &matcher, double weight,
                       double minimum, bool useBaseName) {
    for (size_t i = 0; i < haystacks.size(); ++i) {
      if (haystacks[i].matched)
        continue;

      double score =
          matcher.score(useBaseName ? utils::baseName(lowered[i]) : lowered[i]);
      if (score < minimum)
        continue;

      haystacks[i].score *= score * weight * caseFactors[i];
      haystacks[i].matched = true;
    }
  };

  applyTier(SuffixMatcher(loweredNeedle), WEIGHT_SUFFIX, MIN_BINARY_SCORE,
            false);
  applyTier(KMPMatcher(loweredNeedle), WEIGHT_KMP, MIN_BINARY_SCORE, false);
  applyTier(TokenizedLevenshteinMatcher(loweredNeedle), WEIGHT_TOKEN,
            MIN_BINARY_SCORE, false);

  // Damerau-Levenshtein judges one path component only.
  if (loweredNeedle.find('/') == std::string::npos)
    applyTier(DamerauLevenshteinMatcher(loweredNeedle), WEIGHT_DAMERAU,
              MIN_DAMERAU_SCORE, true);
}
