#pragma once
#include <string>
#include <vector>

namespace utils {
std::vector<std::string> tokenize(const std::string& str, char delimiter = '/');
}