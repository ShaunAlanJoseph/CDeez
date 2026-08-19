#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "MatcherBase.h"

class KMPMatcher : public MatcherBase {
 private:
  std::vector<size_t> _lps;

  void buildLPS();

 public:
  explicit KMPMatcher(const std::string& needle);

  double score(const std::string& haystack) const override;
};
