#pragma once
#include <vector>
#include "MatcherBase.h"

class KMPMatcher : public MatcherBase
{
private:
    std::vector<int> _lps;

    void buildLPS();

public:
    explicit KMPMatcher(const std::string &needle);

    ld score(const std::string &haystack) const override;
};
