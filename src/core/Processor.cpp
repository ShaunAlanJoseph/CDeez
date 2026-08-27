#include "Processor.h"

#include <ctime>
#include <optional>
#include <stdexcept>
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
} // namespace

double Processor::_computeBaseScore(const DB::PathEntry &entry,
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

Processor::Processor(DB &db) : _db(db) {}

bool Processor::_isExcluded(const std::string &path) const {
  return path == "/" || path == utils::normalizePath("~");
}

void Processor::add(const std::string &path) {
  std::string fullPath = utils::normalizePath(path);

  if (!utils::dirExists(fullPath))
    throw std::runtime_error("Not a directory: " + fullPath);

  if (_isExcluded(fullPath))
    return;

  _db.upsertPath(fullPath, std::time(nullptr));
}

std::optional<std::string> Processor::resolve(const std::string &query) const {
  std::vector<MatchResult> results;
  std::time_t now = std::time(nullptr);
  for (const auto &entry : _db.getPaths())
    results.emplace_back(entry.path, _computeBaseScore(entry, now), false);

  scoreMatches(query, results);

  std::string bestPath;
  double bestScore = 0.0;
  for (const auto &result : results) {
    if (result.matched && result.score > bestScore &&
        utils::dirExists(result.str)) {
      bestPath = result.str;
      bestScore = result.score;
    }
  }

  if (bestPath.empty())
    return std::nullopt;

  return bestPath;
}
