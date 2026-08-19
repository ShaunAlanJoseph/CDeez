#include "utils/StringUtils.h"

#include <string>
#include <vector>

std::vector<std::string> utils::tokenize(const std::string& str,
                                         char delimiter) {
  std::vector<std::string> tokens(1);
  for (char c : str) {
    if (c == delimiter)
      tokens.emplace_back();
    else
      tokens.back() += c;
  }
  if (tokens.back().empty()) tokens.pop_back();
  return tokens;
}