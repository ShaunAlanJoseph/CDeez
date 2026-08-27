#include <exception>
#include <iostream>
#include <optional>
#include <string>

#include "core/Processor.h"
#include "db/db.h"

namespace {
  constexpr const char *USAGE = "Usage:\n"
                                "  cdeez add <path>\n"
                                "  cdeez query <term>\n";
} // namespace

int main(int argc, char *argv[]) {
  if (argc != 3) {
    std::cerr << USAGE;
    return 2;
  }

  std::string command = argv[1];
  std::string arg = argv[2];

  if (command != "add" && command != "query") {
    std::cerr << USAGE;
    return 2;
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
