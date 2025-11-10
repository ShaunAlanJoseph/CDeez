#include <iostream>
#include <filesystem>

namespace utils
{
    std::string expandHome(const std::string &path)
    {
        if (path.empty() || path[0] != '~')
            return path;

        const char *home = std::getenv("HOME");
        return std::string(home ? home : "") + path.substr(1);
    }

    bool createParentDirectoriesIfNotExist(const std::string &path)
    {
        std::filesystem::path fsPath(path);
        auto parentPath = fsPath.parent_path();
        if (parentPath.empty())
            return true;

        std::error_code ec;
        std::filesystem::create_directories(parentPath, ec);
        if (ec)
        {
            std::cerr << "Failed to create directories: " << ec.message() << std::endl;
            return false;
        }
        return true;
    }
}