#include "curl_wrapper.h"
/// searches local cfg file for auth_key
cURL_wrapper::cURL_wrapper()
{
    boost::filesystem::path myfile(key_ini_filename);
    
    if (boost::filesystem::exists(myfile))
    {
        std::ifstream inf(key_ini_filename);
        std::getline(inf, auth_key);
        this->initialize_curl();
    }
    else
    {
        throw std::exception("data.cfg doesn't exist");
    }
}
void cURL_wrapper::initialize_curl()
{
    if (!(auth_key.size() == allowed_key_size_free) && !(auth_key.size() == allowed_key_size_pro))
        throw std::invalid_argument("wrong size of auth_key");
    if (auth_key.size() == allowed_key_size_free && auth_key[37] != 'f' && auth_key[38] != 'x')
        throw std::invalid_argument("free_auth_key doesn't end with 'fx'");

    cURLpp::initialize(CURL_GLOBAL_ALL);
}

/// saves auth_key in local file
cURL_wrapper::cURL_wrapper(const char* AUTH_KEY)
{
    std::string s(AUTH_KEY);
    cURL_wrapper::cURL_wrapper(s);
}
cURL_wrapper::cURL_wrapper(const std::string& AUTH_KEY)
{
    auth_key = AUTH_KEY;
    this->initialize_curl();
    std::ofstream ouf(key_ini_filename,std::ofstream::trunc);
    ouf << auth_key;
}
cURL_wrapper::~cURL_wrapper()
{
    cURLpp::terminate();
}
bool cURL_wrapper::Is_auth_key_pro() noexcept
{
    return cURL_wrapper::auth_key.size() == cURL_wrapper::allowed_key_size_pro;
}
std::string cURL_wrapper::run_request(const std::string& text, const std::string& target_lang, const std::string& input_lang)
{
    std::list<std::string> header{ "Authorization: DeepL-Auth-Key " + cURL_wrapper::auth_key };

    std::string postmessage = "text=" + text;
    postmessage += "&target_lang=" + target_lang;
    postmessage += "&source_lang=" + input_lang;
    
    auto request = cURLpp::Easy();
    request.setOpt(new cURLpp::Options::BufferSize(5600));
    request.setOpt(new cURLpp::Options::HttpHeader(header));
    if(Is_auth_key_pro())
        request.setOpt(new cURLpp::Options::Url("https://api.deepl.com/v2/translate"));
    else
        request.setOpt(new cURLpp::Options::Url("https://api-free.deepl.com/v2/translate"));
    //request.setOpt(new cURLpp::Options::NoProgress(1));
    request.setOpt(new curlpp::options::PostFields(postmessage));

    request.perform();
    //std::cout << text << std::endl;
    std::stringstream result;
    //request.setOpt(new cURLpp::Options::WriteStream(&result));
    result << request;
    std::string str;
    getline(result, str);
    //text:\" \"a
    // sprawdŸ to: "message":"Quota Exceeded"
    const size_t begin = str.find("text\":\"") + 7;
    const size_t end = str.size() - 4;
    if (begin >= end)
        return "";
    try {
        str = str.substr(begin, end - begin);
        return str;
    }
    catch (...)
    {
        return "";
    }
    
}
