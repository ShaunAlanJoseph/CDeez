#include "TokenizedMatcher.h"
#include "SequenceMatcher.h"
#include "utils/StringUtils.h"
#include <vector>

TokenizedLevenshteinMatcher::TokenizedLevenshteinMatcher(const std::string &needle)
    : MatcherBase(needle) {}

ld TokenizedLevenshteinMatcher::score(const std::string &haystack) const
{
    /*
    @brief Tokenizes the needle and haystack strings and checks for token sequence matches.
    @return A score of 1.0 if all needle tokens are found in sequence within the haystack tokens, otherwise 0.0.
    */
    std::vector<std::string> needleTokens = utils::tokenize(_needle),
                             haystackTokens = utils::tokenize(haystack);

    if (needleTokens.empty() || haystackTokens.empty())
        return 0.0;

    size_t n = needleTokens.size(), m = haystackTokens.size();

    auto needleIter = needleTokens.rbegin();
    for (size_t i = haystackTokens.size(); i-- > 0 && needleIter != needleTokens.rend();)
        if (SequenceMatcher(*needleIter).score(haystackTokens[i]) > 0.0)
            ++needleIter;

    return needleIter == needleTokens.rend() ? 1.0 : 0.0;
}
