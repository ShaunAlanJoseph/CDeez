#pragma once
#include <string>

#include "MatcherBase.h"

class SequenceMatcher : public MatcherBase {
 public:
  explicit SequenceMatcher(const std::string& needle);

  double score(const std::string& haystack) const override;
};