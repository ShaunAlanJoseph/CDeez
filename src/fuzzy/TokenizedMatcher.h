#pragma once
#include "MatcherBase.h"

class TokenizedLevenshteinMatcher : public MatcherBase
{
public:
    explicit TokenizedLevenshteinMatcher(const std::string &needle);

    ld score(const std::string &haystack) const override;
};