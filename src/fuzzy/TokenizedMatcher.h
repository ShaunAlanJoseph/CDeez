#pragma once

#include <string>

#include "MatcherBase.h"

class TokenizedLevenshteinMatcher : public MatcherBase {
 public:
  explicit TokenizedLevenshteinMatcher(const std::string& needle);

  double score(const std::string& haystack) const override;
};