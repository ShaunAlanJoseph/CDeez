#include "Processor.h"

#include <unistd.h>

#include <algorithm>
#include <ctime>
#include <iostream>
#include <string>
#include <vector>

#include "db/db.h"
#include "fuzzy/OrderMatches.h"
#include "utils/PathUtils.h"

namespace {
constexpr double LAST_HOUR = 3600.0;
constexpr double LAST_DAY = 24 * LAST_HOUR;
constexpr double LAST_WEEK = 7 * LAST_DAY;
constexpr double LAST_MONTH = 30 * LAST_DAY;

constexpr double LAST_HOUR_MULTIPLIER = 4.0;
constexpr double LAST_DAY_MULTIPLIER = 2.0;
constexpr double LAST_WEEK_MULTIPLIER = 1;
constexpr double LAST_MONTH_MULTIPLIER = 0.5;
constexpr double OLDER_MULTIPLIER = 0.25;
}  // namespace

Processor::Processor() : _db() {
  if (!_db.isOpen())
    std::cerr << "Failed to open database." << std::endl;
  else if (!_db.ensureTable())
    std::cerr << "Failed to ensure database table." << std::endl;
}

double Processor::_computeBaseScore(const DB::PathEntry& entry,
                                    std::time_t now) const {
  double score = static_cast<double>(entry.access_count);
  double age = static_cast<double>(now - entry.last_accessed);
  if (age <= LAST_HOUR)
    score *= LAST_HOUR_MULTIPLIER;
  else if (age <= LAST_DAY)
    score *= LAST_DAY_MULTIPLIER;
  else if (age <= LAST_WEEK)
    score *= LAST_WEEK_MULTIPLIER;
  else if (age <= LAST_MONTH)
    score *= LAST_MONTH_MULTIPLIER;
  else
    score *= OLDER_MULTIPLIER;
  return score;
}

bool Processor::handlePath(const std::string& path) {
  if (path[0] == '~' || path == "-") {
    std::cout << path;
    return true;
  }

  if (utils::dirExists(path)) {
    std::string fullPath = utils::absolutePath(path);
    _db.upsertPath(fullPath, std::time(nullptr));
    std::cout << fullPath;
    return true;
  }

  auto paths = _db.getPaths();
  if (paths.empty()) {
    std::cerr << "No known paths in database." << std::endl;
    return false;
  }

  std::vector<MatchResult> results;
  std::time_t now = std::time(nullptr);
  for (const auto& entry : paths)
    results.emplace_back(entry.path, _computeBaseScore(entry, now), false);

  scoreMatches(path, results);

  std::sort(results.begin(), results.end(),
            [](const MatchResult& a, const MatchResult& b) {
              return a.score > b.score;
            });

  std::string bestPath;
  double bestScore = 0.0L;
  for (const auto& result : results) {
    std::cerr << result.str << " -> " << result.score << " " << result.matched
              << "\n";
    if (result.matched && result.score > bestScore) {
      bestScore = result.score;
      bestPath = result.str;
    }
  }

  if (bestPath.empty()) {
    std::cerr << "No matching path found." << std::endl;
    return false;
  }

  if (!utils::dirExists(bestPath)) {
    std::cerr << "Failed to change directory to: " << bestPath << std::endl;
    _db.removePath(bestPath);
    return false;
  }

  _db.upsertPath(bestPath, now);
  std::cout << bestPath;
  return true;
}