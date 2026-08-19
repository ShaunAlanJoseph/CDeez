#pragma once

#include <string>
#include <vector>

struct MatchResult {
  std::string str;
  double score;
  bool matched;
};

void scoreMatches(const std::string& needle,
                  std::vector<MatchResult>& haystacks);