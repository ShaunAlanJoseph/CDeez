#include "SuffixMatcher.h"

SuffixMatcher::SuffixMatcher(const std::string &needle) : MatcherBase(needle) {}

ld SuffixMatcher::score(const std::string &haystack) const
{
    /*
    @brief Checks if the needle is a suffix of the haystack.
    @return A score of 1.0 if the needle is found as a suffix in the haystack, otherwise 0.0.
    */
    if (haystack.size() < _needle.size())
        return 0.0;

    for (size_t i = 0; i < _needle.size(); i++)
        if (haystack[haystack.size() - i - 1] != _needle[_needle.size() - i - 1])
            return 0.0;

    return 1.0;
}
