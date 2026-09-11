#include <exception>
#include <filesystem>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

#include "Version.h"
#include "core/Processor.h"
#include "db/db.h"
#include "shell/ShellInit.h"
#include "utils/PathUtils.h"

namespace {
  constexpr const char *USAGE = "Usage:\n"
                                "  cdeez add <path>\n"
                                "  cdeez query [--exclude <path>] [--list]\n"
                                "              <term>...\n"
                                "  cdeez init <zsh|bash|fish>\n"
                                "  cdeez list\n"
                                "  cdeez import   (reads paths on stdin)\n"
                                "  cdeez tag <name> [path]\n"
                                "  cdeez untag <name>\n"
                                "  cdeez tags\n"
                                "  cdeez --help | --version\n";

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

  if (command == "--version" || command == "-V") {
    std::cout << "cdeez " << cdeez::VERSION << "\n";
    return 0;
  }

  if (command == "--help" || command == "-h") {
    std::cout << USAGE;
    return 0;
  }

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
  if (command == "list" && argc != 2)
    return usageError();
  if (command == "import" && argc != 2)
    return usageError();
  if (command == "tag" && (argc < 3 || argc > 4))
    return usageError();
  if (command == "untag" && argc != 3)
    return usageError();
  if (command == "tags" && argc != 2)
    return usageError();
  if (command != "add" && command != "query" && command != "list" &&
      command != "import" && command != "tag" && command != "untag" &&
      command != "tags")
    return usageError();

  try {
    DB db(DB::defaultPath());
    Processor processor(db);

    if (command == "add") {
      processor.add(argv[2]);
      return 0;
    }

    if (command == "tag") {
      std::string path = argc == 4 ? argv[3] : std::filesystem::current_path();
      processor.tag(argv[2], path);
      return 0;
    }

    if (command == "untag") {
      processor.untag(argv[2]);
      return 0;
    }

    if (command == "tags") {
      for (const auto &[name, path] : processor.tags())
        std::cout << name << "\t" << path << "\n";
      return 0;
    }

    if (command == "import") {
      Processor::ImportResult result = processor.import(std::cin);
      std::cerr << "cdeez: imported " << result.imported << " directories, "
                << "skipped " << result.skipped << std::endl;
      return 0;
    }

    if (command == "list") {
      for (const std::string &path : processor.list())
        std::cout << path << "\n";
      return 0;
    }

    std::string exclude;
    bool listAll = false;

    int first = 2;
    while (first < argc) {
      std::string flag = argv[first];

      if (flag == "--list") {
        listAll = true;
        ++first;
      } else if (flag == "--exclude") {
        if (first + 2 >= argc)
          return usageError();

        exclude = utils::normalizePath(argv[first + 1]);
        first += 2;
      } else if (flag == "--") {
        ++first;
        break;
      } else {
        break;
      }
    }

    if (first >= argc)
      return usageError();

    std::vector<std::string> keywords;
    for (int i = first; i < argc; ++i)
      keywords.push_back(argv[i]);

    if (listAll) {
      for (const std::string &path : processor.rank(keywords, exclude))
        std::cout << path << "\n";
      return 0;
    }

    std::optional<std::string> match = processor.resolve(keywords, exclude);
    if (!match) {
      std::cerr << "cdeez: no match for '" << joinKeywords(argc, argv, first)
                << "'" << std::endl;
      return 1;
    }

    std::cout << *match;
  } catch (const std::exception &e) {
    std::cerr << "cdeez: " << e.what() << std::endl;
    return 3;
  }

  return 0;
}
