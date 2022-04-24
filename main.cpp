#include "imgui.h"
#include "imgui_impl_dx9.h"
#include "imgui_impl_win32.h"
#include <d3d9.h>
#include <tchar.h>
#include "imfilebrowser.h"
#include <boost/filesystem.hpp>
#include "boost/algorithm/string.hpp"
#include "XML_Translator.h"
#include <iostream>
#include "Locale.h"
#include <fcntl.h>
#include <io.h>
#include <stdio.h>
#include <span>


using namespace boost::algorithm;
// Data
static LPDIRECT3D9              g_pD3D = nullptr;
static LPDIRECT3DDEVICE9        g_pd3dDevice = nullptr;
static D3DPRESENT_PARAMETERS    g_d3dpp = {};

int main(int, char**);

// Forward declarations of helper functions
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void ResetDevice();

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

constexpr int auth_key_buffer_size = 40;
std::unique_ptr<cURL_wrapper> curl_wrapper;
constexpr auto program_conf_name = "config.cfg";

int input_locale_index = 0;
int output_locale_index = 0;
// commands table vars
constexpr int raw_commands_size = 1024 * 16;
static char text[raw_commands_size] = "DialogsResource.Reply:text";
const std::vector<Locale*> locales =
{
    new Locale("Russian", "ru_RU", "RU"),
    new Locale("Polish", "pl_PL", "PL"),
    new Locale("English", "en_GB", "EN"),
    new Locale("German", "de_DE", "DE"),
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
constexpr auto helptext = "Enter path to tags you want to translate. Example:\n\n\
<DialogsResource>\n\
    <Reply\n\
        name=\"Dlg_npc_002712\"\n\
        text=\"Hallo!\"\n\
        role=\"NPC\"/>\n\
    <string name=\"global_confirm\">Bevestig</string>\n\
</DialogsResource>\n\n\
To translate word Bevestig of in <string>, write: DialogsResource.string\n\
To translate Hallo! (text attribute of tag Reply), write: DialogsResource.Reply:text\n\
Every command has to be written in a new line.";
static void HelpMarker(const char* desc)
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
std::vector<std::string> Process_raw_commands(std::span<char> text)
{
    //auto end = std::find(text, text + raw_commands_size, '\0');
    auto end = std::find(text.begin(), text.end(), '\0');
    std::string raw_commands = std::string(text.begin(), end);
    trim(raw_commands);
    std::vector<std::string> output;
    boost::split(output, raw_commands, boost::is_any_of("\n"));
    return output;
}
void Save_configuration()
{
    
    std::ofstream ouf(program_conf_name, std::ofstream::trunc);
    ouf << input_locale_index << std::endl;
    ouf << output_locale_index << std::endl;
    ouf << text;
}
void Load_configuration()
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
// Main code
int main(int, char**)
{
    // Create application window
    //ImGui_ImplWin32_EnableDpiAwareness();
    WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, _T("XML Translator"), NULL };
    ::RegisterClassEx(&wc);
    HWND hwnd = ::CreateWindow(wc.lpszClassName, _T("XML Translator"), WS_OVERLAPPEDWINDOW, 100, 100, 1280, 800, NULL, NULL, wc.hInstance, NULL);

    // Initialize Direct3D
    if (!CreateDeviceD3D(hwnd))
    {
        CleanupDeviceD3D();
        ::UnregisterClass(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    // Show the window
    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsClassic();

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX9_Init(g_pd3dDevice);

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
    // - Read 'docs/FONTS.md' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    //io.Fonts->AddFontDefault();
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/ProggyTiny.ttf", 10.0f);
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
    //IM_ASSERT(font != NULL);

    // States
    bool refreshed_translate_window = false;
    bool show_demo_window = true;
    bool show_another_window = false;
    bool show_fullscreen = true;
    bool show_enter_auth_window = true;
    bool show_error_message = false;
    bool got_auth_key = false;
    bool try_read_auth_key = true;
    Xml_translator xml_parser;
    std::string path;
    const ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    // create a file browser instance
    ImGui::FileBrowser fileDialog;

    // (optional) set browser properties
    fileDialog.SetTitle("File browser");
    fileDialog.SetTypeFilters({ ".xml"});

    // Main loop
    bool done = false;
    Load_configuration();
    while (!done)
    {
        // Poll and handle messages (inputs, window resize, etc.)
        // See the WndProc() function below for our to dispatch events to the Win32 backend.
        MSG msg;
        while (::PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                done = true;
        }
        if (done)
            break;

        // Start the Dear ImGui frame
        ImGui_ImplDX9_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

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
                    catch(...)
                    {
                    }
                    try_read_auth_key = false;
                }

                if (!curl_wrapper)
                {
                    static char auth_key_buffer[auth_key_buffer_size] = "";
                    ImGui::SetNextWindowSize(ImVec2(700,70));
                    ImGui::Begin("Enter API auth key to deepl account", &show_enter_auth_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
                    ImGui::SetWindowFocus();
                    if (ImGui::InputText("##label3", auth_key_buffer, auth_key_buffer_size, ImGuiInputTextFlags_EnterReturnsTrue))
                    {
                        try
                        {
                            curl_wrapper = std::make_unique<cURL_wrapper>(auth_key_buffer);
                        }
                        catch(...)
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
                        input_locale_index = n;

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
                        output_locale_index = n;

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
                if (refreshed_translate_window) {
                    path = fileDialog.GetSelected().string();
                    std::cout << "Selected filename: " << path << std::endl;
                    auto commands = Process_raw_commands(std::span(text, raw_commands_size));

                    show_error_message = !xml_parser.translate(
                        path,
                        commands,
                        *locales[input_locale_index],
                        *locales[output_locale_index]
                    );

                    fileDialog.ClearSelected();
                    refreshed_translate_window = false;
                }
                else
                    refreshed_translate_window = true;
            }
        }
        if (show_error_message)
        {
            ImGui::Begin("Error", &show_another_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
            ImGui::Text("Translation aborted. There's probably an error in the syntax of your commands or you've selected wrong file.");
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

        // Rendering
        ImGui::EndFrame();
        g_pd3dDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
        g_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
        g_pd3dDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
        D3DCOLOR clear_col_dx = D3DCOLOR_RGBA((int)(clear_color.x*clear_color.w*255.0f), (int)(clear_color.y*clear_color.w*255.0f), (int)(clear_color.z*clear_color.w*255.0f), (int)(clear_color.w*255.0f));
        g_pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, clear_col_dx, 1.0f, 0);
        if (g_pd3dDevice->BeginScene() >= 0)
        {
            ImGui::Render();
            ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
            g_pd3dDevice->EndScene();
        }
        HRESULT result = g_pd3dDevice->Present(NULL, NULL, NULL, NULL);

        // Handle loss of D3D9 device
        if (result == D3DERR_DEVICELOST && g_pd3dDevice->TestCooperativeLevel() == D3DERR_DEVICENOTRESET)
            ResetDevice();
    }

    ImGui_ImplDX9_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClass(wc.lpszClassName, wc.hInstance);

    return 0;
}

// Helper functions

bool CreateDeviceD3D(HWND hWnd)
{
    if ((g_pD3D = Direct3DCreate9(D3D_SDK_VERSION)) == NULL)
        return false;

    // Create the D3DDevice
    ZeroMemory(&g_d3dpp, sizeof(g_d3dpp));
    g_d3dpp.Windowed = TRUE;
    g_d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    g_d3dpp.BackBufferFormat = D3DFMT_UNKNOWN; // Need to use an explicit format with alpha if needing per-pixel alpha composition.
    g_d3dpp.EnableAutoDepthStencil = TRUE;
    g_d3dpp.AutoDepthStencilFormat = D3DFMT_D16;
    g_d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_ONE;           // Present with vsync
    //g_d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;   // Present without vsync, maximum unthrottled framerate
    if (g_pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, D3DCREATE_HARDWARE_VERTEXPROCESSING, &g_d3dpp, &g_pd3dDevice) < 0)
        return false;

    return true;
}

void CleanupDeviceD3D()
{
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = NULL; }
    if (g_pD3D) { g_pD3D->Release(); g_pD3D = NULL; }
}

void ResetDevice()
{
    ImGui_ImplDX9_InvalidateDeviceObjects();
    HRESULT hr = g_pd3dDevice->Reset(&g_d3dpp);
    if (hr == D3DERR_INVALIDCALL)
        IM_ASSERT(0);
    ImGui_ImplDX9_CreateDeviceObjects();
}

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Win32 message handler
// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        if (g_pd3dDevice != NULL && wParam != SIZE_MINIMIZED)
        {
            g_d3dpp.BackBufferWidth = LOWORD(lParam);
            g_d3dpp.BackBufferHeight = HIWORD(lParam);
            ResetDevice();
        }
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProc(hWnd, msg, wParam, lParam);
}
