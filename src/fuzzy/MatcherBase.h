#pragma once

#include <string>

class MatcherBase {
 protected:
  std::string _needle;

 public:
  explicit MatcherBase(const std::string& needle);
  virtual ~MatcherBase() = default;

  virtual double score(const std::string& haystack) const = 0;
};
