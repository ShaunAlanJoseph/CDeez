#pragma once

#include <ctime>
#include <optional>
#include <string>

#include "db/db.h"

class Processor {
private:
  DB &_db;

  double _computeBaseScore(const DB::PathEntry &entry, std::time_t now) const;

  bool _isExcluded(const std::string &path) const;

public:
  explicit Processor(DB &db);

  void add(const std::string &path);

  std::optional<std::string> resolve(const std::string &query) const;
};
