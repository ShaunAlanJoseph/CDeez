#pragma once

#include <string>

#include "MatcherBase.h"

class DamerauLevenshteinMatcher : public MatcherBase {
 public:
  explicit DamerauLevenshteinMatcher(const std::string& needle);

  double score(const std::string& haystack) const override;
};