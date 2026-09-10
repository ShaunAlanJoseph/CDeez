#include "db.h"

#include <sqlite3.h>

#include <ctime>
#include <stdexcept>
#include <string>
#include <vector>

#include "utils/PathUtils.h"
#include "utils/ScopeGuard.h"

namespace {
  constexpr const char *DB_SUBPATH = "/cdeez/db.sqlite3";

  constexpr int BUSY_TIMEOUT_MS = 3000;
} // namespace

std::string DB::defaultPath() { return utils::xdgDataHome() + DB_SUBPATH; }

DB::DB(const std::string &path) : _db(nullptr) {
  utils::createParentDirectories(path);

  if (sqlite3_open(path.c_str(), &_db) != SQLITE_OK) {
    std::string errorMsg = sqlite3_errmsg(_db);
    sqlite3_close(_db);
    throw std::runtime_error("Failed to open database: " + errorMsg);
  }

  try {
    configure();
    ensureTable();
  } catch (...) {
    sqlite3_close(_db);
    throw;
  }
}

DB::~DB() { sqlite3_close(_db); }

void DB::configure() {
  // Another shell may be writing; wait rather than fail.
  sqlite3_busy_timeout(_db, BUSY_TIMEOUT_MS);

  constexpr const char *WAL_QUERY = "PRAGMA journal_mode = WAL;";
  char *errMsg = nullptr;
  utils::ScopeGuard errMsgGuard([&]() { sqlite3_free(errMsg); });
  if (sqlite3_exec(_db, WAL_QUERY, nullptr, nullptr, &errMsg) != SQLITE_OK)
    throw std::runtime_error("Failed to enable WAL: " + std::string(errMsg));
}

void DB::ensureTable() {
  constexpr const char *CREATE_TABLE_QUERY =
      "CREATE TABLE IF NOT EXISTS paths ("
      "path TEXT PRIMARY KEY, "
      "access_count INTEGER NOT NULL, "
      "last_accessed INTEGER NOT NULL);";
  char *errMsg = nullptr;
  utils::ScopeGuard errMsgGuard([&]() { sqlite3_free(errMsg); });
  if (sqlite3_exec(_db, CREATE_TABLE_QUERY, nullptr, nullptr, &errMsg) !=
      SQLITE_OK)
    throw std::runtime_error("Failed to create table: " + std::string(errMsg));
}

void DB::upsertPath(const std::string &path, std::time_t access_time) {
  addPath(path, 1, access_time);
}

void DB::addPath(const std::string &path, int access_count,
                 std::time_t access_time) {
  constexpr const char *ADD_PATH_QUERY =
      "INSERT INTO paths (path, access_count, last_accessed) "
      "VALUES (?, ?, ?) "
      "ON CONFLICT(path) DO UPDATE SET "
      "access_count = access_count + excluded.access_count, "
      "last_accessed = MAX(last_accessed, excluded.last_accessed);";

  sqlite3_stmt *stmt = nullptr;
  utils::ScopeGuard stmtGuard([&]() { sqlite3_finalize(stmt); });
  if (sqlite3_prepare_v2(_db, ADD_PATH_QUERY, -1, &stmt, nullptr) != SQLITE_OK)
    throw std::runtime_error("Failed to prepare statement: " +
                             std::string(sqlite3_errmsg(_db)));

  if (sqlite3_bind_text(stmt, 1, path.c_str(), -1, SQLITE_TRANSIENT) !=
          SQLITE_OK ||
      sqlite3_bind_int(stmt, 2, access_count) != SQLITE_OK ||
      sqlite3_bind_int64(stmt, 3, static_cast<sqlite3_int64>(access_time)) !=
          SQLITE_OK)
    throw std::runtime_error("Failed to bind parameter: " +
                             std::string(sqlite3_errmsg(_db)));

  if (sqlite3_step(stmt) != SQLITE_DONE)
    throw std::runtime_error("Failed to execute statement: " +
                             std::string(sqlite3_errmsg(_db)));
}

void DB::removePath(const std::string &path) {
  constexpr const char *REMOVE_PATH_QUERY = "DELETE FROM paths WHERE path = ?;";

  sqlite3_stmt *stmt = nullptr;
  utils::ScopeGuard stmtGuard([&]() { sqlite3_finalize(stmt); });
  if (sqlite3_prepare_v2(_db, REMOVE_PATH_QUERY, -1, &stmt, nullptr) !=
      SQLITE_OK)
    throw std::runtime_error("Failed to prepare statement: " +
                             std::string(sqlite3_errmsg(_db)));

  if (sqlite3_bind_text(stmt, 1, path.c_str(), -1, SQLITE_TRANSIENT) !=
      SQLITE_OK)
    throw std::runtime_error("Failed to bind parameter: " +
                             std::string(sqlite3_errmsg(_db)));

  if (sqlite3_step(stmt) != SQLITE_DONE)
    throw std::runtime_error("Failed to execute statement: " +
                             std::string(sqlite3_errmsg(_db)));
}

std::vector<DB::PathEntry> DB::getPaths() const {
  std::vector<PathEntry> result;
  constexpr const char *GET_PATHS_QUERY =
      "SELECT path, access_count, last_accessed FROM paths;";

  sqlite3_stmt *stmt = nullptr;
  utils::ScopeGuard stmtGuard([&]() { sqlite3_finalize(stmt); });
  if (sqlite3_prepare_v2(_db, GET_PATHS_QUERY, -1, &stmt, nullptr) != SQLITE_OK)
    throw std::runtime_error("Failed to prepare statement: " +
                             std::string(sqlite3_errmsg(_db)));
  int rc;
  while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
    PathEntry entry;
    entry.path = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
    entry.access_count = sqlite3_column_int(stmt, 1);
    entry.last_accessed =
        static_cast<std::time_t>(sqlite3_column_int64(stmt, 2));
    result.push_back(entry);
  }

  if (rc != SQLITE_DONE)
    throw std::runtime_error("Failed to read rows: " +
                             std::string(sqlite3_errmsg(_db)));

  return result;
}

int DB::totalAccessCount() const {
  constexpr const char *TOTAL_QUERY =
      "SELECT COALESCE(SUM(access_count), 0) FROM paths;";

  sqlite3_stmt *stmt = nullptr;
  utils::ScopeGuard stmtGuard([&]() { sqlite3_finalize(stmt); });
  if (sqlite3_prepare_v2(_db, TOTAL_QUERY, -1, &stmt, nullptr) != SQLITE_OK)
    throw std::runtime_error("Failed to prepare statement: " +
                             std::string(sqlite3_errmsg(_db)));

  if (sqlite3_step(stmt) != SQLITE_ROW)
    throw std::runtime_error("Failed to read total: " +
                             std::string(sqlite3_errmsg(_db)));

  return sqlite3_column_int(stmt, 0);
}

void DB::ageAll(double factor) {
  // Without CAST, SQLite stores the product as a REAL.
  constexpr const char *AGE_QUERY =
      "BEGIN;"
      "UPDATE paths SET access_count = CAST(access_count * ?1 AS INTEGER);"
      "DELETE FROM paths WHERE access_count < 1;"
      "COMMIT;";

  sqlite3_stmt *stmt = nullptr;
  utils::ScopeGuard stmtGuard([&]() { sqlite3_finalize(stmt); });
  const char *tail = AGE_QUERY;
  while (*tail) {
    if (sqlite3_prepare_v2(_db, tail, -1, &stmt, &tail) != SQLITE_OK)
      throw std::runtime_error("Failed to prepare statement: " +
                               std::string(sqlite3_errmsg(_db)));
    if (!stmt)
      break;

    if (sqlite3_bind_parameter_count(stmt) > 0 &&
        sqlite3_bind_double(stmt, 1, factor) != SQLITE_OK)
      throw std::runtime_error("Failed to bind parameter: " +
                               std::string(sqlite3_errmsg(_db)));

    if (sqlite3_step(stmt) != SQLITE_DONE)
      throw std::runtime_error("Failed to execute statement: " +
                               std::string(sqlite3_errmsg(_db)));

    sqlite3_finalize(stmt);
    stmt = nullptr;
  }
}
