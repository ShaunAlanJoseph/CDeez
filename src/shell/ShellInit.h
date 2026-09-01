#pragma once

#include <optional>
#include <string>

namespace shell {
  std::optional<std::string> initScript(const std::string &shell);
} // namespace shell
