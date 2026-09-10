#include "Processor.h"

#include <algorithm>
#include <cmath>
#include <ctime>
#include <optional>
#include <sstream>
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

Processor::Processor(DB &db, int maxTotalAccess)
    : _db(db), _maxTotalAccess(maxTotalAccess) {}

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

  int total = _db.totalAccessCount();
  if (total > _maxTotalAccess)
    _db.ageAll(static_cast<double>(_maxTotalAccess) / total);
}

std::vector<std::string> Processor::rank(const std::string &query,
                                         const std::string &exclude) const {
  std::vector<MatchResult> results;
  std::time_t now = std::time(nullptr);
  for (const auto &entry : _db.getPaths())
    results.emplace_back(entry.path, _computeBaseScore(entry, now), false);

  scoreMatches(query, results);

  std::sort(results.begin(), results.end(),
            [](const MatchResult &a, const MatchResult &b) {
              return a.score > b.score;
            });

  std::vector<std::string> matches;
  for (const auto &result : results) {
    if (!result.matched || result.str == exclude)
      continue;
    if (!utils::dirExists(result.str))
      continue;

    matches.push_back(result.str);
  }
  return matches;
}

std::optional<std::string>
Processor::resolve(const std::string &query, const std::string &exclude) const {
  std::vector<std::string> matches = rank(query, exclude);
  if (matches.empty())
    return std::nullopt;

  return matches.front();
}

std::vector<std::string> Processor::list() const {
  std::vector<MatchResult> results;
  std::time_t now = std::time(nullptr);
  for (const auto &entry : _db.getPaths())
    results.emplace_back(entry.path, _computeBaseScore(entry, now), false);

  std::sort(results.begin(), results.end(),
            [](const MatchResult &a, const MatchResult &b) {
              return a.score > b.score;
            });

  std::vector<std::string> paths;
  for (const auto &result : results)
    paths.push_back(result.str);
  return paths;
}

Processor::ImportResult Processor::import(std::istream &input) {
  /*
  @brief Records directories read from an input stream.
  @param input One directory per line, each optionally preceded by a score,
  as produced by `zoxide query --list --score`.
  @return How many were imported and how many were skipped.
  */
  ImportResult result{0, 0};
  std::time_t now = std::time(nullptr);

  std::string line;
  while (std::getline(input, line)) {
    std::istringstream fields(line);

    double score = 0.0;
    std::string path;
    if (fields >> score && std::getline(fields >> std::ws, path)) {
    } else {
      fields.clear();
      fields.str(line);
      std::getline(fields >> std::ws, path);
      score = 1.0;
    }

    while (!path.empty() &&
           std::isspace(static_cast<unsigned char>(path.back())))
      path.pop_back();

    if (path.empty())
      continue;

    std::string fullPath = utils::normalizePath(path);
    if (!utils::dirExists(fullPath) || _isExcluded(fullPath)) {
      ++result.skipped;
      continue;
    }

    int count = std::max(1, static_cast<int>(std::lround(score)));
    _db.addPath(fullPath, count, now);
    ++result.imported;
  }

  return result;
}
