#pragma once

#include <ctime>
#include <istream>
#include <optional>
#include <string>
#include <vector>

#include "db/db.h"

class Processor {
private:
  DB &_db;
  int _maxTotalAccess;

  double _computeBaseScore(const DB::PathEntry &entry, std::time_t now) const;

  bool _isExcluded(const std::string &path) const;

public:
  static constexpr int DEFAULT_MAX_TOTAL_ACCESS = 10000;

  explicit Processor(DB &db, int maxTotalAccess = DEFAULT_MAX_TOTAL_ACCESS);

  void add(const std::string &path);

  std::vector<std::string> rank(const std::string &query,
                                const std::string &exclude = "") const;

  std::optional<std::string> resolve(const std::string &query,
                                     const std::string &exclude = "") const;

  std::vector<std::string> list() const;

  struct ImportResult {
    int imported;
    int skipped;
  };

  ImportResult import(std::istream &input);
};
