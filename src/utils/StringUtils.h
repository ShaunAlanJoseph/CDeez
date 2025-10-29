#pragma once
#include <string>
#include <vector>

namespace utils
{
    std::vector<std::string> tokenize(const std::string &str, char delimiter = '/')
    {
        std::vector<std::string> tokens(1);
        for (char c : str)
        {
            if (c == delimiter)
                tokens.emplace_back();
            else
                tokens.back() += c;
        }
        if (tokens.back().empty())
            tokens.pop_back();
        return tokens;
    }
}