#pragma once
#include <boost/filesystem.hpp>
#include "boost/algorithm/string.hpp"
#include "Translator.h"
#include <iostream>
#include "Locale.h"
#include <fcntl.h>
#include <io.h>
#include <stdio.h>
#include <span>
#include "imgui.h"
#include "imgui_impl_dx9.h"
#include "imgui_impl_win32.h"
#include "imfilebrowser.h"
class Application
{
private:
    // States
    bool refreshed_translate_window = false;
    bool show_demo_window = true;
    bool show_another_window = false;
    bool show_fullscreen = true;
    bool show_enter_auth_window = true;
    bool show_error_message = false;
    bool got_auth_key = false;
    bool try_read_auth_key = true;

    Translator xml_parser;
    ImGui::FileBrowser fileDialog;
    static const int auth_key_buffer_size = 40;
    char auth_key_buffer[auth_key_buffer_size] = "";
    std::unique_ptr<cURL_wrapper> curl_wrapper;
    inline static const std::string program_conf_name = "config.cfg";

    int input_locale_index = 1;
    int output_locale_index = 2;

    std::string error_message_string;

    static const int raw_commands_size = 1024 * 16;
    char text[raw_commands_size];
    const std::vector<Locale*> locales =
    {
        new Locale("Polish", "pl_PL", "PL"),
        new Locale("English", "en_GB", "EN"),
        new Locale("German", "de_DE", "DE"),
        new Locale("Russian", "ru_RU", "RU"),
        new Locale("Spanish", "es_ES", "ES"),
        new Locale("Chinese simpl", "zh_CN", "ZH"),
        new Locale("Bulgarian", "bg_BG", "BG"),
        new Locale("Czech", "cs_CZ", "CS"),
        new Locale("Danish", "da_DK", "DA"),
        new Locale("Dutch", "nl_NL", "NL"),
        new Locale("Estonian", "et_EE", "ET"),
        new Locale("Finnish", "fi_FI", "FI"),
        new Locale("French", "fr_FR", "FR"),
        new Locale("Greek", "el_GR", "EL"),
        new Locale("Hungarian", "hu_HU", "HU"),
        new Locale("Italian", "it_IT", "IT"),
        new Locale("Japanese", "ja_JP", "JA"),
        new Locale("Latvian", "lv_LV", "LV"),
        new Locale("Lithuanian", "lt_LT", "LT"),
        new Locale("Portuguese", "pt_PT", "PT"),
        new Locale("Romanian", "ro_RO", "RO"),
        new Locale("Slovak", "sk_SK", "SK"),
        new Locale("Slovenian", "sl_SL", "SL"),
        new Locale("Swedish", "sv_SE", "SV"),
    };
    
public:
    Application();
    static void HelpMarker(const char* desc);
    std::vector<std::string> Process_raw_commands(std::span<char> text);
    void Save_configuration();
    void Load_configuration();
    void Loop();
};
