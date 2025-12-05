#define NOMINMAX
#include <windows.h>
#include <shlwapi.h>
#pragma comment(lib, "shlwapi.lib")

#include <d3d9.h>
#pragma comment(lib, "d3d9.lib")
#include <d3dx9tex.h>
#pragma comment(lib, "d3dx9.lib")

#include <MinHook.h>
#include <map>
#include <string>
#include <fstream>
#include <chrono>
#include <ctime>
#include <sstream>
#include <thread>
#include <algorithm>
#include <queue>
#include <mutex>
#include <atomic>
#include <vector>
#include <yaml-cpp/yaml.h>
#include <imgui.h>
#include <imgui_impl_win32.h>
#include "imgui_impl_dx9.h"

// =============================================================
// CONFIGURATION SYSTEM
// =============================================================
enum class GameID {
    Unknown,
    SkyFC,
    XanaduNext,
    YsSeven,
    YsOrigin,
    YsVI,
    YsFelghana
};

struct GameConfig {
    std::string gameName;
    std::string windowTitlePart;
    std::vector<std::string> yamlFiles;
    bool useFileHook;
    bool useRvaHook;
    uintptr_t bgmFuncRVA;
};

GameConfig g_Config;
GameID g_CurrentGame = GameID::Unknown;
static std::mutex g_LogMutex;

// =============================================================
// LOGGING
// =============================================================
std::string GetModDirectory() {
    char path[MAX_PATH];
    HMODULE hModule = NULL;
    GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCSTR)&GetModDirectory, &hModule);
    GetModuleFileNameA(hModule, path, MAX_PATH);
    PathRemoveFileSpecA(path);
    return std::string(path);
}

void Log(const std::string& message) {
    std::lock_guard<std::mutex> lock(g_LogMutex);
    std::string path = GetModDirectory() + "\\mod_log.txt";
    std::ofstream log_file(path, std::ios_base::app | std::ios_base::out);
    auto time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    char time_str[26];
    ctime_s(time_str, sizeof(time_str), &time);
    time_str[24] = '\0';
    log_file << "[" << time_str << "] " << message << std::endl;
}

void WCharToString(const WCHAR* wstr, char* buffer, size_t bufferSize) {
    if (!wstr || !buffer) return;
    WideCharToMultiByte(CP_UTF8, 0, wstr, -1, buffer, (int)bufferSize, NULL, NULL);
}

// =============================================================
// GAME DETECTION
// =============================================================
void DetectAndConfigure() {
    char exePath[MAX_PATH];
    GetModuleFileNameA(NULL, exePath, MAX_PATH);
    std::string exe = exePath;
    std::transform(exe.begin(), exe.end(), exe.begin(), ::tolower);

    g_Config.useFileHook = false;
    g_Config.useRvaHook = false;
    g_Config.bgmFuncRVA = 0;

    if (exe.find("xanadu.exe") != std::string::npos) {
        g_CurrentGame = GameID::XanaduNext;
        g_Config.gameName = "Xanadu Next";
        g_Config.windowTitlePart = "Xanadu";
        g_Config.yamlFiles.push_back("BgmMap_XanaduNext.yaml");
        g_Config.useRvaHook = true;
        g_Config.bgmFuncRVA = 0xC9B60;
    }
    else if (exe.find("ys7.exe") != std::string::npos) {
        g_CurrentGame = GameID::YsSeven;
        g_Config.gameName = "Ys Seven";
        g_Config.windowTitlePart = "Ys Seven";
        g_Config.yamlFiles.push_back("BgmMap_Ys7.yaml");
        g_Config.useFileHook = true;
    }
    else if (exe.find("yso_win.exe") != std::string::npos) {
        g_CurrentGame = GameID::YsOrigin;
        g_Config.gameName = "Ys Origin";
        g_Config.windowTitlePart = "Ys Origin";
        g_Config.yamlFiles.push_back("BgmMap_YsOrigin.yaml");
        g_Config.useFileHook = true;
    }
    else if (exe.find("ys6_win_dx9.exe") != std::string::npos) {
        g_CurrentGame = GameID::YsVI;
        g_Config.gameName = "Ys VI: The Ark of Napishtim";
        g_Config.windowTitlePart = "Ys VI";
        g_Config.yamlFiles.push_back("BgmMap_Ys6.yaml");
        g_Config.useFileHook = true;
    }
    else if (exe.find("ysf_win_dx9.exe") != std::string::npos) {
        g_CurrentGame = GameID::YsFelghana;
        g_Config.gameName = "Ys: The Oath in Felghana";
        g_Config.windowTitlePart = "Felghana";
        g_Config.yamlFiles.push_back("BgmMap_YsFelghana.yaml");
        g_Config.useFileHook = true;
    }
    else if (exe.find("ed6_win_dx9.exe") != std::string::npos) {
        g_CurrentGame = GameID::SkyFC;
        g_Config.gameName= "The Legend of Heroes: Trails in the Sky";
        g_Config.windowTitlePart = "Trails in the Sky";
        g_Config.yamlFiles.push_back("BgmMap_SkyFC.yaml");
        g_Config.useFileHook = true;
    }
    else {
        g_CurrentGame = GameID::Unknown;
        g_Config.gameName = "Unknown Game";
        g_Config.windowTitlePart = "";
        g_Config.yamlFiles.push_back("BgmMap.yaml");
        g_Config.useFileHook = true;
    }

    Log("DETECTED GAME: " + g_Config.gameName);
}

// =============================================================
// GLOBALS
// =============================================================
struct BgmInfo {
    std::string songName;
    std::string japaneseName;
    std::string disc;
    std::string track;
    std::string album;
    std::string rawFileName;
};

static std::map<std::string, BgmInfo> g_bgmMap;
static BgmInfo g_currentBgmInfo;
static std::map<std::string, std::chrono::steady_clock::time_point> g_songLastShown;
static std::string g_lastTriggeredFile = "";

static float g_toastTimer = 0.0f;
constexpr float TOAST_DURATION_SECONDS = 5.0f;
constexpr int COOLDOWN_HOURS = 5;
static float g_toastCurrentX = -10000.0f;

static float g_currentToastTotalWidth = 0.0f;
static float g_currentToastTotalHeight = 0.0f;
static float g_currentNoteIconWidth = 0.0f;
static float g_currentBoxWidth = 0.0f;
static float g_currentTextHeight = 0.0f;
static float g_currentLineHeight = 28.0f;

static bool g_imguiInitialized = false;
static HWND g_hWindow = nullptr;
static ImFont* g_pToastFont = nullptr;
static LPDIRECT3DTEXTURE9 g_pToastTexture = nullptr;
static float g_TextureWidth = 0.0f;
static float g_TextureHeight = 0.0f;

static std::thread g_workerThread;
static std::mutex g_bufferMutex;
static char g_bgmFilenameBuffer[MAX_PATH];
static std::atomic<bool> g_bWorkerThreadActive = true;
static std::vector<std::string> g_bgmQueue; // Stores pending files



// =============================================================
// YAML PARSER
// =============================================================
std::vector<std::string> SplitString(const std::string& s, char delimiter) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(s);
    while (std::getline(tokenStream, token, delimiter)) tokens.push_back(token);
    return tokens;
}

void LoadMapFile(const std::string& filename) {
    std::string modDir = GetModDirectory();
    std::string yamlPath = modDir + "\\assets\\" + filename;
    std::ifstream file(yamlPath);
    if (!file.is_open()) { Log("LoadMapFile: " + filename + " not found."); return; }

    try {
        YAML::Node config = YAML::Load(file);
        for (const auto& node : config) {
            std::string filepath = node.first.as<std::string>();
            std::string value = node.second.as<std::string>();
            BgmInfo info;
            info.rawFileName = filepath;
            std::vector<std::string> parts = SplitString(value, '|');

            if (parts.size() >= 1) info.songName = parts[0];
            if (parts.size() >= 2) info.japaneseName = parts[1];
            if (parts.size() >= 3) info.disc = parts[2];
            if (parts.size() >= 4) info.track = parts[3];
            if (parts.size() >= 5) info.album = parts[4];

            if (info.japaneseName == info.songName) info.japaneseName = "";
            if (info.japaneseName == " ") info.japaneseName = "";
            g_bgmMap[filepath] = info;
        }
        Log("Loaded: " + filename + " (" + std::to_string(g_bgmMap.size()) + " entries)");
    } catch (const YAML::Exception& e) { Log("YAML Error: " + std::string(e.what())); }
}

void LoadBgmMap() {
    g_bgmMap.clear();
    for (const auto& file : g_Config.yamlFiles) LoadMapFile(file);
    Log("Total entries: " + std::to_string(g_bgmMap.size()));
}

// =============================================================
// XANADU NEXT ID MAPPING
// =============================================================
std::string GetXanaduFilenameFromID(int id) {
    switch(id) {
        case 0: return "XANA000"; case 1: return "XANA001"; case 2: return "XANA002"; case 3: return "XANA003";
        case 4: return "XANA004"; case 5: return "XANA010"; case 6: return "XANA020"; case 7: return "XANA030";
        case 8: return "XANA040"; case 9: return "XANA050"; case 10: return "XANA060"; case 11: return "XANA070";
        case 12: return "XANA080"; case 13: return "XANA090"; case 14: return "XANA100"; case 15: return "XANA110";
        case 16: return "XANA200"; case 17: return "XANA210"; case 18: return "XANA300"; case 19: return "XANA310";
        case 20: return "XANA320"; case 21: return "XANA330"; case 22: return "XANA340"; case 23: return "XANA350";
        default: return "";
    }
}

// =============================================================
// GRAPHICS HOOKS (DX9)
// =============================================================
typedef HRESULT(WINAPI* PFN_ENDSCENE)(IDirect3DDevice9* pDevice);
static PFN_ENDSCENE g_pfnOriginalEndScene = nullptr;

typedef HRESULT(WINAPI* PFN_RESET)(IDirect3DDevice9* pDevice, D3DPRESENT_PARAMETERS* pPresentationParameters);
static PFN_RESET g_pfnOriginalReset = nullptr;

typedef IDirect3D9* (WINAPI* PFN_DIRECT3DCREATE9)(UINT SDKVersion);
static PFN_DIRECT3DCREATE9 g_pfnOriginalDirect3DCreate9 = nullptr;

typedef HRESULT(WINAPI* PFN_CREATEDEVICE)(IDirect3D9* pD3D, UINT Adapter, D3DDEVTYPE DeviceType, HWND hFocusWindow, DWORD BehaviorFlags, D3DPRESENT_PARAMETERS* pPresentationParameters, IDirect3DDevice9** ppReturnedDeviceInterface);
static PFN_CREATEDEVICE g_pfnOriginalCreateDevice = nullptr;

static bool g_bEndSceneHooked = false;

static WNDPROC g_pfnOriginalWndProc = NULL;
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT WINAPI WndProc(const HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam)) return TRUE;
    return CallWindowProc(g_pfnOriginalWndProc, hWnd, uMsg, wParam, lParam);
}

void LoadDdsTexture(IDirect3DDevice9* pDevice) {
    std::string texturePathStr = GetModDirectory() + "\\assets/bgm_info.dds";
    HRESULT hr = D3DXCreateTextureFromFileA(pDevice, texturePathStr.c_str(), &g_pToastTexture);
    if (SUCCEEDED(hr) && g_pToastTexture) {
        D3DSURFACE_DESC desc;
        g_pToastTexture->GetLevelDesc(0, &desc);
        g_TextureWidth = (float)desc.Width;
        g_TextureHeight = (float)desc.Height;
        Log("Texture loaded: " + std::to_string(g_TextureWidth) + "x" + std::to_string(g_TextureHeight));
    } else {
        Log("Failed to load bgm_info.dds");
    }
}

void InitImGui(IDirect3DDevice9* pDevice) {
    if (g_imguiInitialized) return;

    Log("InitImGui called.");

    g_pfnOriginalWndProc = (WNDPROC)SetWindowLongPtr(g_hWindow, GWLP_WNDPROC, (LONG_PTR)WndProc);
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;

    const std::string fontPathStr = GetModDirectory() + "\\assets/mod_font.otf";
    const std::string fontPathJapaneseStr = GetModDirectory() + "\\assets/mod_font_japanese.ttf";
    g_pToastFont = io.Fonts->AddFontFromFileTTF(fontPathStr.c_str(), 28.0f);
    if (g_pToastFont) {
        Log("mod_font.otf loaded successfully.");
        ImFontConfig config;
        config.MergeMode = true;
        const ImWchar* ranges = io.Fonts->GetGlyphRangesJapanese();
        io.Fonts->AddFontFromFileTTF(fontPathJapaneseStr.c_str(), 28.0f, &config, ranges);
        Log("Attempted to merge Japanese font.");
    } else {
        Log("Failed to load mod_font.otf!");
    }

    LoadDdsTexture(pDevice);
    ImGui_ImplWin32_Init(g_hWindow);
    ImGui_ImplDX9_Init(pDevice);

    Log("ImGui initialized successfully.");
    g_imguiInitialized = true;
}

HRESULT WINAPI My_EndScene(IDirect3DDevice9* pDevice) {
    InitImGui(pDevice);

    ImGui_ImplDX9_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    ImGuiIO& io = ImGui::GetIO();
    D3DVIEWPORT9 viewport;
    pDevice->GetViewport(&viewport);
    io.DisplaySize.x = (float)viewport.Width;
    io.DisplaySize.y = (float)viewport.Height;

    if (!g_pToastTexture) LoadDdsTexture(pDevice);

    const float UI_SCALE = 0.65f;
    const float SCREEN_PADDING = 10.0f;
    const float TEXT_PADDING_X = 20.0f * UI_SCALE;
    const float TEXT_PADDING_Y = 15.0f * UI_SCALE;
    const float ROUNDING = 8.0f * UI_SCALE;

    if (g_toastTimer > 0.0f && g_toastCurrentX == -10000.0f) {
        if (g_pToastFont) ImGui::PushFont(g_pToastFont);

        ImVec2 s1 = ImGui::CalcTextSize(g_currentBgmInfo.songName.c_str());
        g_currentLineHeight = s1.y > 0 ? s1.y : 28.0f;

        float w1 = s1.x;
        float w2 = ImGui::CalcTextSize(g_currentBgmInfo.japaneseName.c_str()).x;
        float w3 = ImGui::CalcTextSize(g_currentBgmInfo.album.c_str()).x;
        std::string discTrack = (!g_currentBgmInfo.disc.empty() || !g_currentBgmInfo.track.empty()) 
            ? "Disc " + g_currentBgmInfo.disc + ", Track " + g_currentBgmInfo.track : "";
        float w4 = ImGui::CalcTextSize(discTrack.c_str()).x;
        float text_width = std::max({w1, w2, w3, w4});

        int line_count = 0;
        if (!g_currentBgmInfo.songName.empty()) line_count++;
        if (!g_currentBgmInfo.japaneseName.empty()) line_count++;
        if (!discTrack.empty()) line_count++;
        if (!g_currentBgmInfo.album.empty()) line_count++;

        g_currentTextHeight = g_currentLineHeight * (float)line_count + (g_currentLineHeight * 0.2f * (line_count - 1));

        if (g_pToastFont) ImGui::PopFont();

        g_currentToastTotalHeight = g_currentTextHeight + (TEXT_PADDING_Y * 2.0f);
        g_currentNoteIconWidth = g_currentToastTotalHeight;
        g_currentBoxWidth = text_width + (TEXT_PADDING_X * 2.0f);
        g_currentToastTotalWidth = g_currentNoteIconWidth + g_currentBoxWidth;

        g_toastCurrentX = io.DisplaySize.x + SCREEN_PADDING;
    }

    if (g_toastCurrentX != -10000.0f) {
        float target = io.DisplaySize.x - g_currentToastTotalWidth - SCREEN_PADDING;
        float dt = io.DeltaTime;
        if (g_toastTimer > 0.0f) {
            if (g_toastCurrentX > target) g_toastCurrentX -= 1500.0f * dt;
            g_toastTimer -= dt;
        } else {
            if (g_toastCurrentX < io.DisplaySize.x + SCREEN_PADDING) g_toastCurrentX += 1500.0f * dt;
            else g_toastCurrentX = -10000.0f;
        }
    }

    if (g_toastCurrentX != -10000.0f) {
        ImDrawList* dl = ImGui::GetBackgroundDrawList();
        ImVec2 p_box(g_toastCurrentX + g_currentNoteIconWidth, SCREEN_PADDING);
        dl->AddRectFilled(p_box, ImVec2(p_box.x + g_currentBoxWidth, SCREEN_PADDING + g_currentToastTotalHeight), IM_COL32(0, 0, 0, 180), ROUNDING);

        if (g_pToastTexture) {
            dl->AddImage((void*)g_pToastTexture, 
                ImVec2(g_toastCurrentX, SCREEN_PADDING), 
                ImVec2(g_toastCurrentX + g_currentNoteIconWidth, SCREEN_PADDING + g_currentToastTotalHeight));
        }

        float current_y = SCREEN_PADDING + (g_currentToastTotalHeight - g_currentTextHeight) * 0.5f;

        if (g_pToastFont) ImGui::PushFont(g_pToastFont);

        auto DrawLine = [&](const std::string& s, ImU32 col) {
            if (!s.empty()) {
                dl->AddText(ImVec2(p_box.x + TEXT_PADDING_X, current_y), col, s.c_str());
                current_y += g_currentLineHeight * 1.2f;
            }
        };

        DrawLine(g_currentBgmInfo.songName, IM_COL32_WHITE);
        DrawLine(g_currentBgmInfo.japaneseName, IM_COL32(200, 200, 200, 255));
        std::string line3 = (!g_currentBgmInfo.disc.empty() || !g_currentBgmInfo.track.empty()) 
            ? "Disc " + g_currentBgmInfo.disc + ", Track " + g_currentBgmInfo.track : "";
        DrawLine(line3, IM_COL32(180, 180, 180, 255));
        DrawLine(g_currentBgmInfo.album, IM_COL32(180, 180, 180, 255));

        if (g_pToastFont) ImGui::PopFont();
    }

    ImGui::EndFrame();
    ImGui::Render();
    ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
    return g_pfnOriginalEndScene(pDevice);
}

HRESULT WINAPI Detour_Reset(IDirect3DDevice9* pDevice, D3DPRESENT_PARAMETERS* pPresentationParameters) {
    if (g_pToastTexture) { g_pToastTexture->Release(); g_pToastTexture = nullptr; }
    ImGui_ImplDX9_InvalidateDeviceObjects();
    return g_pfnOriginalReset(pDevice, pPresentationParameters);
}

// =============================================================
// FILE SYSTEM HOOKS
// =============================================================
bool IsTargetFile(const std::string& fname) {
    if (fname.length() < 4) return false;
    std::string ext = fname.substr(fname.length() - 4);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return (ext == ".ogg" || ext == ".wav");
}

bool IsTargetFileW(const std::wstring& fname) {
    if (fname.length() < 4) return false;
    std::wstring ext = fname.substr(fname.length() - 4);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::towlower);
    return (ext == L".ogg" || ext == L".wav");
}

typedef HANDLE(WINAPI* PFN_CREATEFILEW)(LPCWSTR, DWORD, DWORD, LPSECURITY_ATTRIBUTES, DWORD, DWORD, HANDLE);
static PFN_CREATEFILEW g_pfnOriginalCreateFileW = nullptr;

HANDLE WINAPI Detour_CreateFileW(LPCWSTR lpFileName, DWORD dwAccess, DWORD dwShare, LPSECURITY_ATTRIBUTES lpSec, DWORD dwDisp, DWORD dwFlags, HANDLE hTemplate) {
    if (lpFileName && IsTargetFileW(lpFileName)) {
        char buf[MAX_PATH];
        WCharToString(lpFileName, buf, MAX_PATH);

        // FIX: Queue it, don't drop it
        std::lock_guard<std::mutex> lock(g_bufferMutex);
        g_bgmQueue.push_back(std::string(buf));
    }
    return g_pfnOriginalCreateFileW(lpFileName, dwAccess, dwShare, lpSec, dwDisp, dwFlags, hTemplate);
}

typedef HANDLE(WINAPI* PFN_CREATEFILEA)(LPCSTR, DWORD, DWORD, LPSECURITY_ATTRIBUTES, DWORD, DWORD, HANDLE);
static PFN_CREATEFILEA g_pfnOriginalCreateFileA = nullptr;

HANDLE WINAPI Detour_CreateFileA(LPCSTR lpFileName, DWORD dwAccess, DWORD dwShare, LPSECURITY_ATTRIBUTES lpSec, DWORD dwDisp, DWORD dwFlags, HANDLE hTemplate) {
    if (lpFileName && IsTargetFile(lpFileName)) {
        // FIX: Queue it, don't drop it
        std::lock_guard<std::mutex> lock(g_bufferMutex);
        g_bgmQueue.push_back(std::string(lpFileName));
    }
    return g_pfnOriginalCreateFileA(lpFileName, dwAccess, dwShare, lpSec, dwDisp, dwFlags, hTemplate);
}

// =============================================================
// BGM PROCESSING
// =============================================================
void TriggerToast(const BgmInfo& info) {
    std::string songKey = info.songName;
    bool shouldShow = false;
    auto it = g_songLastShown.find(songKey);

    if (it == g_songLastShown.end()) {
        shouldShow = true;
    } else {
        auto now = std::chrono::steady_clock::now();
        auto hours = std::chrono::duration_cast<std::chrono::hours>(now - it->second).count();
        if (hours >= COOLDOWN_HOURS) shouldShow = true;
    }

    if (shouldShow) {
        g_currentBgmInfo = info;
        g_toastTimer = TOAST_DURATION_SECONDS;
        g_toastCurrentX = -10000.0f;
        g_songLastShown[songKey] = std::chrono::steady_clock::now();
    }
}

void ProcessBgmTrigger(const std::string& s_filename) {
    if (s_filename == g_lastTriggeredFile) return;

    g_lastTriggeredFile = s_filename;
    Log("Processing: " + s_filename);

    std::string normalizedInput = s_filename;
    std::replace(normalizedInput.begin(), normalizedInput.end(), '/', '\\');

    for (auto& entry : g_bgmMap) {
        std::string key = entry.first;
        std::replace(key.begin(), key.end(), '/', '\\');

        if (normalizedInput.length() >= key.length() &&
            normalizedInput.compare(normalizedInput.length() - key.length(), key.length(), key) == 0) {

            Log("MATCH FOUND for: " + key);
            BgmInfo info = entry.second;
            info.rawFileName = entry.first;
            TriggerToast(info);
            break;
        }
    }
}

void BgmWorkerThread()
{
    Log("BGM Worker Thread started.");
    while (g_bWorkerThreadActive) {

        // 1. GRAB DATA SAFELY
        std::vector<std::string> localQueue;
        {
            std::lock_guard<std::mutex> lock(g_bufferMutex);
            if (!g_bgmQueue.empty()) {
                localQueue = g_bgmQueue; // Copy everything
                g_bgmQueue.clear();      // Clear the global queue
            }
        }

        // 2. PROCESS DATA
        // Now we can take our time processing without blocking the game
        for (const auto& fname : localQueue) {
            ProcessBgmTrigger(fname);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    Log("BGM Worker Thread shutting down.");
}
// =============================================================
// XANADU NEXT RVA HOOK (NAKED ASSEMBLY)
// =============================================================
static void* g_pfnOriginalPlayBgm = nullptr;

void __stdcall ProcessXanaduBgmLogic(int songID) {
    std::string filename = GetXanaduFilenameFromID(songID);

    if (!filename.empty()) {
        if (filename == g_lastTriggeredFile) return;
        g_lastTriggeredFile = filename;

        Log("Music Changed: ID " + std::to_string(songID) + " -> " + filename);

        for (auto& entry : g_bgmMap) {
            if (entry.first.find(filename) != std::string::npos) {
                BgmInfo info = entry.second;
                info.rawFileName = entry.first;
                TriggerToast(info);
                break;
            }
        }
    }
}

#ifdef _M_IX86
void __declspec(naked) Detour_PlayBgm_ASM() {
    __asm {
        pushad
        pushfd
        push ecx
        call ProcessXanaduBgmLogic
        popfd
        popad
        jmp [g_pfnOriginalPlayBgm]
    }
}
#endif

// =============================================================
// HOOK INITIALIZATION
// =============================================================
HRESULT WINAPI Detour_CreateDevice(IDirect3D9* pD3D, UINT Adapter, D3DDEVTYPE DeviceType, HWND hFocusWindow, DWORD BehaviorFlags, D3DPRESENT_PARAMETERS* pPresentationParameters, IDirect3DDevice9** ppReturnedDeviceInterface) {
    HRESULT hr = g_pfnOriginalCreateDevice(pD3D, Adapter, DeviceType, hFocusWindow, BehaviorFlags, pPresentationParameters, ppReturnedDeviceInterface);
    if (SUCCEEDED(hr) && ppReturnedDeviceInterface && !g_bEndSceneHooked) {
        IDirect3DDevice9* pDevice = *ppReturnedDeviceInterface;
        void** pVTable = *(void***)pDevice;
        MH_CreateHook((LPVOID)pVTable[42], &My_EndScene, (LPVOID*)&g_pfnOriginalEndScene);
        MH_CreateHook((LPVOID)pVTable[16], &Detour_Reset, (LPVOID*)&g_pfnOriginalReset);
        MH_EnableHook(MH_ALL_HOOKS);
        g_bEndSceneHooked = true;
        Log("EndScene and Reset hooked.");
    }
    return hr;
}

IDirect3D9* WINAPI Detour_Direct3DCreate9(UINT SDKVersion) {
    IDirect3D9* pD3D = g_pfnOriginalDirect3DCreate9(SDKVersion);

    while (g_hWindow == NULL) {
        if (!g_Config.windowTitlePart.empty()) {
            EnumWindows([](HWND hwnd, LPARAM lParam) -> BOOL {
                char title[256];
                GetWindowTextA(hwnd, title, 256);
                if (std::string(title).find(g_Config.windowTitlePart) != std::string::npos) {
                    g_hWindow = hwnd;
                    return FALSE;
                }
                return TRUE;
            }, 0);
        }
        if (!g_hWindow) {
            Log("Searching for " + g_Config.gameName + " window...");
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    }
    Log(g_Config.gameName + " window found!");

    LoadBgmMap();

    if (pD3D) {
        void** pVTable = *(void***)pD3D;
        MH_CreateHook((LPVOID)pVTable[16], &Detour_CreateDevice, (LPVOID*)&g_pfnOriginalCreateDevice);
        MH_EnableHook(MH_ALL_HOOKS);
        Log("CreateDevice hooked.");
    }

    if (g_Config.useFileHook) {
        g_bWorkerThreadActive = true;
        g_workerThread = std::thread(BgmWorkerThread);

        HMODULE hKernel32 = GetModuleHandleA("kernel32.dll");
        if (hKernel32) {
            void* pCreateFileW = (void*)GetProcAddress(hKernel32, "CreateFileW");
            if (pCreateFileW) {
                MH_CreateHook(pCreateFileW, &Detour_CreateFileW, (LPVOID*)&g_pfnOriginalCreateFileW);
                Log("Hooked CreateFileW");
            }

            void* pCreateFileA = (void*)GetProcAddress(hKernel32, "CreateFileA");
            if (pCreateFileA) {
                MH_CreateHook(pCreateFileA, &Detour_CreateFileA, (LPVOID*)&g_pfnOriginalCreateFileA);
                Log("Hooked CreateFileA");
            }
            MH_EnableHook(MH_ALL_HOOKS);
        }
    }

#ifdef _M_IX86
    if (g_Config.useRvaHook && g_Config.bgmFuncRVA != 0) {
        uintptr_t baseAddr = (uintptr_t)GetModuleHandle(NULL);
        uintptr_t targetAddr = baseAddr + g_Config.bgmFuncRVA;

        if (MH_CreateHook((LPVOID)targetAddr, &Detour_PlayBgm_ASM, (LPVOID*)&g_pfnOriginalPlayBgm) == MH_OK) {
            MH_EnableHook((LPVOID)targetAddr);
            Log("BGM RVA Hook Successful at: 0x" + std::to_string(targetAddr));
        } else {
            Log("FATAL: Failed to hook BGM function at 0x" + std::to_string(targetAddr));
        }
    }
#endif

    MH_RemoveHook((LPVOID)g_pfnOriginalDirect3DCreate9);
    return pD3D;
}

void InitializeMod() {
    DetectAndConfigure();

    if (MH_Initialize() != MH_OK) {
        Log("MH_Initialize failed!");
        return;
    }
    Log("MH_Initialize successful.");

    HMODULE hD3D9 = GetModuleHandleA("d3d9.dll");
    while (!hD3D9) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        hD3D9 = GetModuleHandleA("d3d9.dll");
    }

    void* pDirect3DCreate9 = (void*)GetProcAddress(hD3D9, "Direct3DCreate9");
    if (pDirect3DCreate9) {
        MH_CreateHook(pDirect3DCreate9, &Detour_Direct3DCreate9, (LPVOID*)&g_pfnOriginalDirect3DCreate9);
        MH_EnableHook(pDirect3DCreate9);
        Log("Hooked Direct3DCreate9.");
    }
}

// =============================================================
// DLL ENTRY POINT
// =============================================================
BOOL WINAPI DllMain(HMODULE hModule, DWORD fdwReason, LPVOID lpReserved) {
    if (fdwReason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);
        InitializeMod();
    } else if (fdwReason == DLL_PROCESS_DETACH){
    }
    return TRUE;
}
