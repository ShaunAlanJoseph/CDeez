#include "KMPMatcher.h"

#include <cstddef>
#include <string>

#include "MatcherBase.h"

KMPMatcher::KMPMatcher(const std::string& needle) : MatcherBase(needle) {
  buildLPS();
}

void KMPMatcher::buildLPS() {
  _lps.assign(_needle.size(), 0);
  size_t len = 0;
  for (size_t i = 1; i < _needle.size();) {
    if (_needle[i] == _needle[len])
      _lps[i++] = ++len;
    else if (len != 0)
      len = _lps[len - 1];
    else
      _lps[i++] = 0;
  }
}

double KMPMatcher::score(const std::string& haystack) const {
  /*
  @brief Uses the Knuth-Morris-Pratt algorithm to find the needle in the
  haystack.
  @return A score of 1.0 if the needle is found in the haystack, otherwise 0.0.
  */
  size_t i = 0, j = 0;
  while (i < haystack.size()) {
    if (_needle[j] == haystack[i]) ++i, ++j;

    if (j == _needle.size())
      return 1.0;  // match found
    else if (i < haystack.size() && _needle[j] != haystack[i]) {
      if (j != 0)
        j = _lps[j - 1];
      else
        ++i;
    }
  }
  return 0.0;  // no match
}
