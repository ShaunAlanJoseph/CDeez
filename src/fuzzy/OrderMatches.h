#pragma once
#include <string>
#include <vector>

using ld = long double;

struct MatchResult
{
    std::string str;
    ld score;
    bool matched;
};

void scoreMatches(const std::string &needle, std::vector<MatchResult> &haystacks);