#include "utils/StringUtils.h"

#include <cstddef>
#include <string>
#include <vector>

std::vector<std::string> utils::tokenize(const std::string &str,
                                         char delimiter) {
  std::vector<std::string> tokens(1);
  for (char c : str) {
    if (c == delimiter)
      tokens.emplace_back();
    else
      tokens.back() += c;
  }
  if (tokens.back().empty())
    tokens.pop_back();
  return tokens;
}

std::string utils::baseName(const std::string &path) noexcept {
  size_t pos = path.find_last_of('/');
  return pos == std::string::npos ? path : path.substr(pos + 1);
}
