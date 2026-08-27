#pragma once

#include <unistd.h>

#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <string>

namespace testing {
  // Sets an environment variable for the duration of a scope and restores the
  // previous value, including the case where it was not set at all. Tests using
  // this touch global state, so they cannot run in parallel with each other.
  class EnvGuard {
  private:
    std::string _name;
    std::string _old;
    bool _wasSet;

  public:
    EnvGuard(const std::string &name, const char *value) : _name(name) {
      const char *old = std::getenv(name.c_str());
      _wasSet = old != nullptr;
      if (_wasSet)
        _old = old;

      if (value)
        setenv(name.c_str(), value, 1);
      else
        unsetenv(name.c_str());
    }

    ~EnvGuard() {
      if (_wasSet)
        setenv(_name.c_str(), _old.c_str(), 1);
      else
        unsetenv(_name.c_str());
    }

    EnvGuard(const EnvGuard &) = delete;
    EnvGuard &operator=(const EnvGuard &) = delete;
  };

  // A uniquely named directory under the system temp directory, removed when
  // the scope ends.
  class TempDir {
  private:
    std::filesystem::path _path;

  public:
    TempDir() {
      _path = std::filesystem::temp_directory_path() /
              ("cdeez_test_" + std::to_string(::getpid()) + "_" +
               std::to_string(reinterpret_cast<uintptr_t>(this)));
      std::filesystem::create_directories(_path);
    }

    ~TempDir() {
      std::error_code ec;
      std::filesystem::remove_all(_path, ec);
    }

    const std::filesystem::path &path() const { return _path; }

    std::string operator/(const std::string &child) const {
      return (_path / child).string();
    }

    std::string makeDir(const std::string &child) const {
      std::filesystem::create_directories(_path / child);
      return (_path / child).string();
    }

    TempDir(const TempDir &) = delete;
    TempDir &operator=(const TempDir &) = delete;
  };
} // namespace testing
