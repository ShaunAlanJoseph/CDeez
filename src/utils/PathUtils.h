#pragma once

#include <string>

namespace utils {
std::string expandHome(const std::string& path);

bool createParentDirectoriesIfNotExist(const std::string& path);

bool dirExists(const std::string& path);

std::string absolutePath(const std::string& path);
}  // namespace utils