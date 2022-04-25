#pragma once
#include <curlpp/cURLpp.hpp>
#include <curlpp/Easy.hpp>
#include <curlpp/Options.hpp>
#include <curlpp/Exception.hpp>
#include "boost/filesystem.hpp"
#include "Locale.h"
#include <vector>
#include <sstream>

class cURL_wrapper
{
private:
    const inline static std::string key_ini_filename = "key.cfg";
    const static int allowed_key_size_free { 39 };
    const static int allowed_key_size_pro { 36 };
    static bool Is_auth_key_pro() noexcept;
    void initialize_curl();
public:
    inline static std::string auth_key = "";
    cURL_wrapper();
    cURL_wrapper(const char* AUTH_KEY);
    cURL_wrapper(const std::string& auth_key);
    ~cURL_wrapper();
    static std::string run_deepl_request(const std::string& text, const std::string& target_lang, const std::string& input_lang);
};

struct out_of_quota_exception : std::exception
{
    const char* what() const throw ()
    {
        return "User out of quota";
    }
};
