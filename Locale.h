#pragma once
#include<string>
struct Locale
{
    std::string name;
    std::string code;
    std::string locale_str;
    Locale(const std::string& NAME, const std::string& LOCALE_CODE, const std::string& DEEPL_STR) noexcept
        :name(NAME), code(LOCALE_CODE), locale_str(DEEPL_STR)
    {
    };
};

