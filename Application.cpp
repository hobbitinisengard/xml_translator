#include "Application.h"

using namespace boost::algorithm;

constexpr auto helptext = "Warning - Input document has to be UTF-8 encoded!\nEnter path to tags you want to translate. Example:\n\n\
<?xml version=\"1.0\" encoding=\"utf-8\"?>\n\
<DialogsResource>\n\
    <Reply\n\
        name=\"Dlg_npc_002712\"\n\
        text=\"Hallo!\"\n\
        role=\"NPC\"/>\n\
    <info name=\"global_confirm\">Bevestig</info>\n\
</DialogsResource>\n\n\
To translate word Bevestig in <info> tag, write: DialogsResource.info\n\
To translate Hallo! (text attribute of tag Reply), write: DialogsResource.Reply:text\n\
Every command has to be written in a new line.";

Application::Application()
{
    fileDialog.SetTitle("File browser");
    fileDialog.SetTypeFilters({ ".xml" });
    Load_configuration();
}

void Application::HelpMarker(const char* desc)
{
    ImGui::TextDisabled("(HELP)");
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(desc);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}
std::vector<std::string> Application::Process_raw_commands(std::span<char> text)
{
    //auto end = std::find(text, text + raw_commands_size, '\0');
    auto end = std::find(text.begin(), text.end(), '\0');
    std::string raw_commands = std::string(text.begin(), end);
    trim(raw_commands);
    std::vector<std::string> output;
    boost::split(output, raw_commands, boost::is_any_of("\n"));
    return output;
}
void Application::Save_configuration()
{

    std::ofstream ouf(program_conf_name, std::ofstream::trunc);
    ouf << input_locale_index << std::endl;
    ouf << output_locale_index << std::endl;
    ouf << text;
}
void Application::Load_configuration()
{
    if (!boost::filesystem::exists(program_conf_name))
        Save_configuration();

    std::ifstream inf(program_conf_name);
    std::string line;
    std::getline(inf, line);
    input_locale_index = std::stoi(line);
    std::getline(inf, line);
    output_locale_index = std::stoi(line);
    std::getline(inf, line);
    strcpy(text, line.c_str());
}

void Application::Loop()
{
    // XML translator fullscreen 
        //-----------------
    
    static bool use_work_area = true;
    static ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoCollapse;

    // We demonstrate using the full viewport area or the work area (without menu-bars, task-bars etc.)
    // Based on your use case you may want one of the other.
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(use_work_area ? viewport->WorkPos : viewport->Pos);
    ImGui::SetNextWindowSize(use_work_area ? viewport->WorkSize : viewport->Size);
    static int combo_flags = ImGuiComboFlags_PopupAlignLeft | ImGuiComboFlags_HeightLargest;
    if (ImGui::Begin("Example: Fullscreen window", &show_fullscreen, flags))
    {
        if (curl_wrapper)
            if (ImGui::Button(("Auth-key: " + curl_wrapper->auth_key).c_str())) {
                curl_wrapper.reset();
                got_auth_key = false;
                try_read_auth_key = false;
            }

        if (!got_auth_key)
        {
            // 
            if (try_read_auth_key)
            {
                try {
                    curl_wrapper = std::make_unique<cURL_wrapper>();
                }
                catch (...)
                {
                }
                try_read_auth_key = false;
            }

            if (!curl_wrapper)
            {
                ImGui::SetNextWindowSize(ImVec2(700, 70));
                ImGui::Begin("Enter API auth key to deepl account", &show_enter_auth_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
                ImGui::SetWindowFocus();
                if (ImGui::InputText("##label3", auth_key_buffer, auth_key_buffer_size, ImGuiInputTextFlags_EnterReturnsTrue))
                {
                    try
                    {
                        curl_wrapper = std::make_unique<cURL_wrapper>(auth_key_buffer);
                    }
                    catch (...)
                    {
                        // if provided auth_key is bad
                    }
                }
                ImGui::SetWindowFocus();
                ImGui::End();
            }
        }
        //ImGui::SameLine();
        const char* input_combo_preview_value = locales[input_locale_index]->name.c_str();  // Pass in the preview value visible before opening the combo (it could be anything)
        ImGui::Text("Input: ");
        ImGui::SameLine();
        if (ImGui::BeginCombo("##label", input_combo_preview_value, combo_flags))
        {
            for (int n = 0; n < locales.size(); ++n)
            {
                const bool is_selected = (input_locale_index == n);
                if (ImGui::Selectable(locales[n]->name.c_str(), is_selected))
                {
                    if (n == output_locale_index) {
                        std::swap(output_locale_index, input_locale_index);
                    }
                    else
                        input_locale_index = n;
                }


                // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                if (is_selected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
        // combobox

        const char* output_combo_preview_value = locales[output_locale_index]->name.c_str();  // Pass in the preview value visible before opening the combo (it could be anything)
        ImGui::Text("Output:");
        ImGui::SameLine();
        if (ImGui::BeginCombo("##label2", output_combo_preview_value, combo_flags))
        {
            for (int n = 0; n < locales.size(); ++n)
            {
                const bool is_selected = (output_locale_index == n);
                if (ImGui::Selectable(locales[n]->name.c_str(), is_selected))
                {
                    if (n == input_locale_index) {
                        std::swap(output_locale_index, input_locale_index);
                    }
                    else
                        output_locale_index = n;
                }
                // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                if (is_selected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }

        // open and read XML file
            // Note: we are using a fixed-sized buffer for simplicity here. See ImGuiInputTextFlags_CallbackResize
            // and the code in misc/cpp/imgui_stdlib.h for how to setup InputText() for dynamically resizing strings.

        ImGui::Text("Command field");
        ImGui::SameLine();
        HelpMarker(helptext);
        ImGui::InputTextMultiline("##source", text, IM_ARRAYSIZE(text),
            ImVec2(-FLT_MIN, ImGui::GetTextLineHeight() * 16), ImGuiInputTextFlags_AllowTabInput);

        if (ImGui::Button("Translate!"))
        {
            fileDialog.Open();
            Save_configuration();
        }
        fileDialog.Display();
        if (fileDialog.HasSelected())
        {
            static bool translating_window = true;
            ImGui::SetNextWindowSize(ImVec2(700, 200));
            //ImGui::SetNextWindowPos(ImVec2(0, 0));
            ImGui::SetWindowFocus();
            ImGui::Begin("Translating...", &translating_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
            ImGui::Text("Translating in progress.. \nOutput file will be available in the same directory as original file. ");
            ImGui::SetWindowFocus();
            ImGui::End();
            if (refreshed_translate_window)
            {
                std::string path = fileDialog.GetSelected().string();
                std::cout << "Selected filename: " << path << std::endl;
                auto commands = Process_raw_commands(std::span(text, raw_commands_size));
                try {
                    xml_parser.translate(
                        path,
                        commands,
                        *locales[input_locale_index],
                        *locales[output_locale_index]
                    );
                }
                catch (std::exception& e)
                {
                    show_error_message = true;
                    error_message_string = e.what();
                }

                fileDialog.ClearSelected();
                refreshed_translate_window = false;
            }
            else
                refreshed_translate_window = true;
        }
    }
    if (show_error_message)
    {
        ImGui::Begin("Translation aborted. ", &show_another_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
        ImGui::Text(error_message_string.c_str());
        ImGui::SetWindowFocus();
        if (ImGui::Button("OK"))
        {
            show_error_message = false;
            show_another_window = false;
        }
        ImGui::End();
    }
    ImGui::End();

    // demo window for feature reference
      /* if (show_demo_window)
           ImGui::ShowDemoWindow(&show_demo_window);*/
}
