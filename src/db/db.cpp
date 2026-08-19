#include "db.h"

#include <sqlite3.h>

#include <ctime>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include "utils/PathUtils.h"

namespace {
constexpr const char* DB_PATH = "~/.local/share/CDeez/db.sqlite3";
}

DB::DB() : _db(nullptr) {
  std::string expandedPath = utils::expandHome(DB_PATH);

  if (!utils::createParentDirectoriesIfNotExist(expandedPath)) {
    std::cerr << "Failed to create database directory." << std::endl;
    return;
  }

  if (sqlite3_open(expandedPath.c_str(), &_db) != SQLITE_OK) {
    std::cerr << "Failed to open database: " << sqlite3_errmsg(_db)
              << std::endl;
    sqlite3_close(_db);
    _db = nullptr;
  }
}

DB::~DB() {
  if (_db) sqlite3_close(_db);
}

bool DB::isOpen() const { return _db != nullptr; }

bool DB::ensureTable() {
  if (!isOpen()) return false;

  constexpr const char* CREATE_TABLE_QUERY =
      "CREATE TABLE IF NOT EXISTS paths ("
      "path TEXT PRIMARY KEY, "
      "access_count INTEGER NOT NULL, "
      "last_accessed INTEGER NOT NULL);";
  char* errMsg = nullptr;
  if (sqlite3_exec(_db, CREATE_TABLE_QUERY, nullptr, nullptr, &errMsg) !=
      SQLITE_OK) {
    std::cerr << "SQL error: " << errMsg << std::endl;
    sqlite3_free(errMsg);
    return false;
  }
  return true;
}

bool DB::upsertPath(const std::string& path, std::time_t access_time) {
  if (!isOpen()) return false;

  constexpr const char* UPSERT_PATH_QUERY =
      "INSERT INTO paths (path, access_count, last_accessed) "
      "VALUES (?, 1, ?) "
      "ON CONFLICT(path) DO UPDATE SET "
      "access_count = access_count + 1, "
      "last_accessed = excluded.last_accessed;";

  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(_db, UPSERT_PATH_QUERY, -1, &stmt, nullptr) !=
      SQLITE_OK) {
    std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(_db)
              << std::endl;
    return false;
  }

  sqlite3_bind_text(stmt, 1, path.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_int64(stmt, 2, static_cast<sqlite3_int64>(access_time));

  if (sqlite3_step(stmt) != SQLITE_DONE) {
    std::cerr << "Failed to execute statement: " << sqlite3_errmsg(_db)
              << std::endl;
    sqlite3_finalize(stmt);
    return false;
  }

  sqlite3_finalize(stmt);
  return true;
}

bool DB::removePath(const std::string& path) {
  if (!isOpen()) return false;

  constexpr const char* REMOVE_PATH_QUERY = "DELETE FROM paths WHERE path = ?;";

  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(_db, REMOVE_PATH_QUERY, -1, &stmt, nullptr) !=
      SQLITE_OK) {
    std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(_db)
              << std::endl;
    return false;
  }

  sqlite3_bind_text(stmt, 1, path.c_str(), -1, SQLITE_TRANSIENT);

  if (sqlite3_step(stmt) != SQLITE_DONE) {
    std::cerr << "Failed to execute statement: " << sqlite3_errmsg(_db)
              << std::endl;
    sqlite3_finalize(stmt);
    return false;
  }

  sqlite3_finalize(stmt);
  return true;
}

std::vector<DB::PathEntry> DB::getPaths() const {
  std::vector<PathEntry> result;
  if (!isOpen()) return result;

  constexpr const char* GET_PATHS_QUERY =
      "SELECT path, access_count, last_accessed FROM paths;";

  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(_db, GET_PATHS_QUERY, -1, &stmt, nullptr) !=
      SQLITE_OK) {
    std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(_db)
              << std::endl;
    return result;
  }

  int rc;
  while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
    PathEntry entry;
    entry.path = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
    entry.access_count = sqlite3_column_int(stmt, 1);
    entry.last_accessed =
        static_cast<std::time_t>(sqlite3_column_int64(stmt, 2));
    result.push_back(std::move(entry));
  }

  if (rc != SQLITE_DONE) {
    std::cerr << "Failed to read rows: " << sqlite3_errmsg(_db) << std::endl;
  }

  sqlite3_finalize(stmt);
  return result;
}