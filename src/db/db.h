#pragma once

#include <sqlite3.h>

#include <ctime>
#include <optional>
#include <string>
#include <utility>
#include <vector>

class DB {
private:
  sqlite3 *_db;

  void configure();
  void migrate();
  int schemaVersion() const;
  void exec(const char *query);

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
  void addPath(const std::string &path, int access_count,
               std::time_t access_time);
  void removePath(const std::string &path);
  std::vector<PathEntry> getPaths() const;
  int totalAccessCount() const;
  void setTag(const std::string &tag, const std::string &path);
  void removeTag(const std::string &tag);
  std::optional<std::string> getTag(const std::string &tag) const;
  std::vector<std::pair<std::string, std::string>> getTags() const;
  void ageAll(double factor);
};
