#pragma once
#include <string>

using ld = long double;

class MatcherBase
{
protected:
    std::string _needle;

public:
    explicit MatcherBase(const std::string &needle);
    virtual ~MatcherBase() = default;

    virtual ld score(const std::string &haystack) const = 0;
};
