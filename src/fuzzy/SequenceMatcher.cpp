#include "SequenceMatcher.h"

#include <cmath>
#include <cstddef>
#include <string>
#include <vector>

#include "MatcherBase.h"

SequenceMatcher::SequenceMatcher(const std::string& needle)
    : MatcherBase(needle) {}

double SequenceMatcher::score(const std::string& haystack) const {
  /*
  @brief Calculates a normalized L2-based clustering score.
  @return A score >0.0 and <=1.0 equal to the clustering score, otherwise 0.0.
  */
  if (haystack.size() < _needle.size() || _needle.empty()) return 0.0;
  std::vector<size_t> cluster_sizes;
  auto needleIter = _needle.rbegin();
  size_t prevMatch = haystack.size();
  for (size_t i = haystack.size(); i-- > 0 && needleIter != _needle.rend();) {
    if (*needleIter != haystack[i]) continue;
    if (i + 1 != prevMatch || cluster_sizes.empty())
      cluster_sizes.push_back(1);
    else
      ++cluster_sizes.back();
    prevMatch = i;
    ++needleIter;
  }

  if (needleIter != _needle.rend()) return 0.0;

  double score = 0.0;
  for (size_t size : cluster_sizes) score += static_cast<double>(size * size);
  score = std::sqrtl(score) / static_cast<double>(_needle.size());
  return score;
}