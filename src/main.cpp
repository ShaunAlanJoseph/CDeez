#include <exception>
#include <iostream>
#include <optional>
#include <string>

#include "core/Processor.h"
#include "db/db.h"
#include "shell/ShellInit.h"

namespace {
  constexpr const char *USAGE = "Usage:\n"
                                "  cdeez add <path>\n"
                                "  cdeez query <term>\n"
                                "  cdeez init <zsh|bash|fish>\n";
} // namespace

int main(int argc, char *argv[]) {
  if (argc != 3) {
    std::cerr << USAGE;
    return 2;
  }

  std::string command = argv[1];
  std::string arg = argv[2];

  if (command != "add" && command != "query" && command != "init") {
    std::cerr << USAGE;
    return 2;
  }

  // init needs no database, so it is handled before one is opened.
  if (command == "init") {
    std::optional<std::string> script = shell::initScript(arg);
    if (!script) {
      std::cerr << USAGE;
      return 2;
    }

    std::cout << *script;
    return 0;
  }

  try {
    DB db(DB::defaultPath());
    Processor processor(db);

    if (command == "add") {
      processor.add(arg);
      return 0;
    }

    std::optional<std::string> match = processor.resolve(arg);
    if (!match) {
      std::cerr << "cdeez: no match for '" << arg << "'" << std::endl;
      return 1;
    }

    std::cout << *match;
  } catch (const std::exception &e) {
    std::cerr << "cdeez: " << e.what() << std::endl;
    return 3;
  }

  return 0;
}
