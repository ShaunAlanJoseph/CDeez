#pragma once
#include "MatcherBase.h"

class SuffixMatcher : public MatcherBase
{
public:
    explicit SuffixMatcher(const std::string &needle);

    ld score(const std::string &haystack) const override;
};