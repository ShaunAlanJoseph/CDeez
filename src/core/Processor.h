#pragma once

#include <ctime>
#include <string>

#include "db/db.h"

class Processor {
 private:
  DB _db;

  double _computeBaseScore(const DB::PathEntry& entry, std::time_t now) const;

 public:
  Processor();

  bool handlePath(const std::string& path);
};