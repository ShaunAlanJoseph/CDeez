#pragma once
#include "MatcherBase.h"

class SequenceMatcher : public MatcherBase
{
public:
    explicit SequenceMatcher(const std::string &needle);

    ld score(const std::string &haystack) const override;
};