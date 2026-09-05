#pragma once
#include <string>
#include <vector>

namespace utils {
  std::vector<std::string> tokenize(const std::string &str,
                                    char delimiter = '/');

  std::string baseName(const std::string &path) noexcept;

  std::string toLower(const std::string &str) noexcept;
} // namespace utils
