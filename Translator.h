#pragma once
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include "boost/algorithm/string.hpp"
#include "boost/asio.hpp"
#include "curl_wrapper.h"
#include <string>
#include <vector>
#include <fstream>
#include <string>
#include <set>
#include <exception>
#include <iostream>
#include "Locale.h"
#include <imgui.h>
namespace pt = boost::property_tree;
class Translator
{
private:
    pt::ptree tree;
public:
    void translate(const std::string& filepath,
        const std::vector<std::string>& commands,
        const Locale& input_locale,
        const Locale& output_locale);
    
private:
    void load_to_ptree(const std::string& filename, const std::string& locale_str);
    void output_to_file(const std::string& filename, const std::string& locale_str);
};

struct bad_command_exception : std::exception
{
    const char* what() const throw ()
    {
        return "Command syntax error";
    }
};

