#pragma once

#include <string>

#include "MatcherBase.h"

class SuffixMatcher : public MatcherBase {
 public:
  explicit SuffixMatcher(const std::string& needle);

  double score(const std::string& haystack) const override;
};