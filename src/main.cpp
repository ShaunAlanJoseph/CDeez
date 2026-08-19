#include <iostream>
#include <string>

#include "core/Processor.h"

int main(int argc, char* argv[]) {
  if (argc > 2) {
    std::cerr << "Usage: cdeez <path>";
    return 1;
  }

  std::string path;
  if (argc == 1)
    path = "~";
  else
    path = argv[1];
  Processor processor;
  if (!processor.handlePath(path)) {
    std::cerr << "Failed to handle path: " << path << std::endl;
    return 1;
  }

  return 0;
}