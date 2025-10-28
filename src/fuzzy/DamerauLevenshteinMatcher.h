#pragma once
#include "MatcherBase.h"

class DamerauLevenshteinMatcher : public MatcherBase
{
public:
    explicit DamerauLevenshteinMatcher(const std::string &needle);

    ld score(const std::string &haystack) const override;
};