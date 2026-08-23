#include "utils/PathUtils.h"

#include <cstdlib>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <system_error>

std::string utils::expandHome(const std::string &path) noexcept {
  if (path != "~" && !path.starts_with("~/"))
    return path;

  const char *home = std::getenv("HOME");
  return std::string(home ? home : "") + path.substr(1);
}

std::string utils::xdgDataHome() noexcept {
  const char *dataHome = std::getenv("XDG_DATA_HOME");
  if (dataHome && dataHome[0] == '/')
    return dataHome;

  return expandHome("~/.local/share");
}

void utils::createParentDirectories(const std::string &path) {
  std::filesystem::path fsPath(path);
  auto parentPath = fsPath.parent_path();

  std::error_code ec;
  std::filesystem::create_directories(parentPath, ec);
  if (ec)
    throw std::runtime_error("Failed to create directories: " + ec.message());
}

bool utils::dirExists(const std::string &path) noexcept {
  return std::filesystem::is_directory(std::filesystem::path(path));
}

std::string utils::normalizePath(const std::string &path) {
  std::error_code ec;
  std::string res =
      std::filesystem::absolute(expandHome(path), ec).lexically_normal();
  if (ec)
    throw std::runtime_error("Failed to normalize path: " + ec.message());
  if (res.size() > 1 && res.back() == '/')
    res.pop_back();
  return res;
}
