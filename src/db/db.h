#pragma once
#include <sqlite3.h>
#include <string>
#include <vector>
#include <ctime>

class DB
{
private:
    sqlite3 *_db;

public:
    struct PathEntry
    {
        std::string path;
        int access_count;
        std::time_t last_accessed;
    };

    explicit DB();
    ~DB();

    bool isOpen() const;

    bool ensureTable();
    bool upsertPath(const std::string &path, std::time_t access_time);
    bool removePath(const std::string &path);
    std::vector<PathEntry> getPaths() const;
};