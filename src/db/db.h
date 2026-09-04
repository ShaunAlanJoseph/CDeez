#pragma once

#include <sqlite3.h>

#include <ctime>
#include <string>
#include <vector>

class DB {
private:
  sqlite3 *_db;

  void configure();
  void ensureTable();

public:
  struct PathEntry {
    std::string path;
    int access_count;
    std::time_t last_accessed;
  };

  static std::string defaultPath();

  explicit DB(const std::string &path);
  ~DB();

  void upsertPath(const std::string &path, std::time_t access_time);
  void removePath(const std::string &path);
  std::vector<PathEntry> getPaths() const;
  int totalAccessCount() const;
  void ageAll(double factor);
};
