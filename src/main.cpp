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
                                "  cdeez query <term>...\n"
                                "  cdeez init <zsh|bash|fish>\n"
                                "  cdeez list\n";

  int usageError() {
    std::cerr << USAGE;
    return 2;
  }

  // Keywords join into one "/"-separated query.
  std::string joinKeywords(int argc, char *argv[], int first) {
    std::string joined = argv[first];
    for (int i = first + 1; i < argc; ++i)
      joined += "/" + std::string(argv[i]);
    return joined;
  }
} // namespace

int main(int argc, char *argv[]) {
  if (argc < 2)
    return usageError();

  std::string command = argv[1];

  if (command == "init") {
    if (argc != 3)
      return usageError();

    std::optional<std::string> script = shell::initScript(argv[2]);
    if (!script)
      return usageError();

    std::cout << *script;
    return 0;
  }

  if (command == "add" && argc != 3)
    return usageError();
  if (command == "query" && argc < 3)
    return usageError();
  if (command == "list" && argc != 2)
    return usageError();
  if (command != "add" && command != "query" && command != "list")
    return usageError();

  try {
    DB db(DB::defaultPath());
    Processor processor(db);

    if (command == "add") {
      processor.add(argv[2]);
      return 0;
    }

    if (command == "list") {
      for (const std::string &path : processor.list())
        std::cout << path << "\n";
      return 0;
    }

    std::string query = joinKeywords(argc, argv, 2);
    std::optional<std::string> match = processor.resolve(query);
    if (!match) {
      std::cerr << "cdeez: no match for '" << query << "'" << std::endl;
      return 1;
    }

    std::cout << *match;
  } catch (const std::exception &e) {
    std::cerr << "cdeez: " << e.what() << std::endl;
    return 3;
  }

  return 0;
}
