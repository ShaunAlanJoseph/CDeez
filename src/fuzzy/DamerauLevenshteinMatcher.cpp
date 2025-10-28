#include "DamerauLevenshteinMatcher.h"

DamerauLevenshteinMatcher::DamerauLevenshteinMatcher(const std::string &needle)
    : MatcherBase(needle) {}

ld DamerauLevenshteinMatcher::score(const std::string &haystack) const
{
    /*
    @brief Computes the Damerau-Levenshtein distance between the needle and haystack.
    @return A similarity score between 0.0 and 1.0
    */
    if (_needle.empty())
        return 0.0;

    size_t n = _needle.size();
    size_t m = haystack.size();

    std::vector<std::vector<int>> dist(n + 1, std::vector<int>(m + 1, 0));

    for (size_t i = 0; i <= n; ++i)
        dist[i][0] = i;

    for (size_t j = 0; j <= m; ++j)
        dist[0][j] = j;

    for (size_t i = 1; i <= n; ++i)
    {
        for (size_t j = 1; j <= m; ++j)
        {
            dist[i][j] = std::min({
                dist[i - 1][j] + 1,                                      // Deletion
                dist[i][j - 1] + 1,                                      // Insertion
                dist[i - 1][j - 1] + (_needle[i - 1] != haystack[j - 1]) // Substitution
            });

            if (i > 1 && j > 1 && _needle[i - 1] == haystack[j - 2] && _needle[i - 2] == haystack[j - 1])
                dist[i][j] = std::min(dist[i][j], dist[i - 2][j - 2] + 1); // Transposition
        }
    }

    int editDistance = dist[n][m];
    ld maxLen = static_cast<ld>(std::max(n, m));
    return 1.0 - (static_cast<ld>(editDistance) / maxLen);
}