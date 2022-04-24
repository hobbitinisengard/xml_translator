#pragma once
#include<string>
struct Locale
{
    std::string name;
    std::string code;
    std::string locale_str;
    Locale(const std::string& NAME, const std::string& CODE, const std::string& strr) noexcept
        :name(NAME), code(CODE), locale_str(strr)
    {
    };
};

