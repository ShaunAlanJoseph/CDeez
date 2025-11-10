#include "OrderMatches.h"
#include "SuffixMatcher.h"
#include "KMPMatcher.h"
#include "TokenizedMatcher.h"
#include "DamerauLevenshteinMatcher.h"

namespace
{
    constexpr long double WEIGHT_SUFFIX = 4.0L;
    constexpr long double WEIGHT_KMP = 3.0L;
    constexpr long double WEIGHT_TOKEN = 2.0L;
    constexpr long double WEIGHT_DAMERAU = 1.0L;
}

void scoreMatches(const std::string &needle, std::vector<MatchResult> &haystacks)
{
    /*
    @brief Scores each haystack string against the needle using multiple matching algorithms.
    @param needle The string to match against.
    @param haystacks A vector of MatchResult objects containing haystack strings with initial scores.
    */

    // Suffix Matcher
    {
        SuffixMatcher matcher(needle);
        for (auto &haystack : haystacks)
        {
            if (haystack.matched)
                continue;
            ld score = matcher.score(haystack.str);
            if (score > 0.0) {
                haystack.score *= score * WEIGHT_SUFFIX;
                haystack.matched = true;
            }
        }
    }

    // KMP Matcher
    {
        KMPMatcher matcher(needle);
        for (auto &haystack : haystacks)
        {
            if (haystack.matched)
                continue;
            ld score = matcher.score(haystack.str);
            if (score > 0.0) {
                haystack.score *= score * WEIGHT_KMP;
                haystack.matched = true;
            }
        }
    }

    // Tokenized Matcher
    {
        TokenizedLevenshteinMatcher matcher(needle);
        for (auto &haystack : haystacks)
        {
            if (haystack.matched)
                continue;
            ld score = matcher.score(haystack.str);
            if (score > 0.0) {
                haystack.score *= score * WEIGHT_TOKEN;
                haystack.matched = true;
            }
        }
    }

    // Damerau-Levenshtein Matcher
    {
        DamerauLevenshteinMatcher matcher(needle);
        for (auto &haystack : haystacks)
        {
            if (haystack.matched)
                continue;
            ld score = matcher.score(haystack.str);
            if (score > 0.0) {
                haystack.score *= score * WEIGHT_DAMERAU;
                haystack.matched = true;
            }
        }
    }
}