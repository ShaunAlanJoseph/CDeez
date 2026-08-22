#pragma once

#include <string>

namespace utils {
  std::string expandHome(const std::string &path) noexcept;

  void createParentDirectories(const std::string &path);

  bool dirExists(const std::string &path) noexcept;

  std::string normalizePath(const std::string &path);
} // namespace utils
