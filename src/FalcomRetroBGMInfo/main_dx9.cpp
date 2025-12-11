#define NOMINMAX
#include <windows.h>
#include <shlwapi.h>
#pragma comment(lib, "shlwapi.lib")
#include "universal_proxy.h"

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
    XanaduNext,
    YsSeven,
    YsOrigin,
    YsVI,
    YsFelghana,
    SkyFC,
    SkySC,
    Sky3rd
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
        g_Config.windowTitlePart = "Ys7";
        g_Config.yamlFiles.push_back("BgmMap_Ys7.yaml");
        g_Config.useRvaHook = true;
        g_Config.bgmFuncRVA = 0x1445E0;  // InternalFopen
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
    else if (exe.find("ed6_win_dx9.exe") != std::string::npos || exe.find("ed6_win.exe") != std::string::npos) {
        g_CurrentGame = GameID::SkyFC;
        g_Config.gameName = "Trails in the Sky FC";
        g_Config.windowTitlePart = "Trails in the Sky";
        g_Config.yamlFiles.push_back("BgmMap_SkyFC.yaml");
        g_Config.useFileHook = true;
    }
    else if (exe.find("ed6_win2_dx9.exe") != std::string::npos || exe.find("ed6_win2.exe") != std::string::npos) {
        g_CurrentGame = GameID::SkySC;
        g_Config.gameName = "Trails in the Sky SC";
        g_Config.windowTitlePart = "Trails in the Sky";
        g_Config.yamlFiles.push_back("BgmMap_SkySC.yaml");
        g_Config.useFileHook = true;
    }
    else if (exe.find("ed6_win3_dx9.exe") != std::string::npos || exe.find("ed6_win3.exe") != std::string::npos) {
        g_CurrentGame = GameID::Sky3rd;
        g_Config.gameName = "Trails in the Sky the 3rd";
        g_Config.windowTitlePart = "Trails in the Sky";
        g_Config.yamlFiles.push_back("BgmMap_Sky3rd.yaml");
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
static float g_lastDisplayWidth = 0.0f;
static float g_lastDisplayHeight = 0.0f;

static bool g_imguiInitialized = false;
static HWND g_hWindow = nullptr;
static ImFont* g_pToastFont = nullptr;
static LPDIRECT3DTEXTURE9 g_pToastTexture = nullptr;
static float g_TextureWidth = 0.0f;
static float g_TextureHeight = 0.0f;

static std::thread g_workerThread;
static std::mutex g_bufferMutex;
static char g_bgmFilenameBuffer[MAX_PATH];
static std::atomic<bool> g_bNewBgmAvailable = false;
static std::atomic<bool> g_bWorkerThreadActive = true;

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

// Forward declarations for functions defined later
typedef HANDLE(WINAPI* PFN_CREATEFILEW)(LPCWSTR, DWORD, DWORD, LPSECURITY_ATTRIBUTES, DWORD, DWORD, HANDLE);
typedef HANDLE(WINAPI* PFN_CREATEFILEA)(LPCSTR, DWORD, DWORD, LPSECURITY_ATTRIBUTES, DWORD, DWORD, HANDLE);
static PFN_CREATEFILEW g_pfnOriginalCreateFileW = nullptr;
static PFN_CREATEFILEA g_pfnOriginalCreateFileA = nullptr;
HANDLE WINAPI Detour_CreateFileW(LPCWSTR lpFileName, DWORD dwAccess, DWORD dwShare, LPSECURITY_ATTRIBUTES lpSec, DWORD dwDisp, DWORD dwFlags, HANDLE hTemplate);
HANDLE WINAPI Detour_CreateFileA(LPCSTR lpFileName, DWORD dwAccess, DWORD dwShare, LPSECURITY_ATTRIBUTES lpSec, DWORD dwDisp, DWORD dwFlags, HANDLE hTemplate);
void BgmWorkerThread();

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

    Log("InitImGui starting...");

    // Ensure we have a valid window handle
    if (!g_hWindow) {
        // Try to get window from device creation parameters
        D3DDEVICE_CREATION_PARAMETERS params;
        if (SUCCEEDED(pDevice->GetCreationParameters(&params))) {
            g_hWindow = params.hFocusWindow;
            Log("Window from GetCreationParameters: " + std::to_string((uintptr_t)g_hWindow));
        }
    }

    if (!g_hWindow) {
        Log("ERROR: No valid window handle, skipping initialization");
        return;
    }

    Log("Using window handle: " + std::to_string((uintptr_t)g_hWindow));

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

    // Set up file hooks now that the game has fully initialized
    // This avoids interfering with initial audio loading
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
}

HRESULT WINAPI My_EndScene(IDirect3DDevice9* pDevice) {
    static int frameCount = 0;
    frameCount++;

    // Log first few frames to confirm EndScene is being called
    if (frameCount <= 3) {
        Log("EndScene called, frame " + std::to_string(frameCount));
    }

    InitImGui(pDevice);

    ImGui_ImplDX9_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    ImGuiIO& io = ImGui::GetIO();

    // Get actual render target dimensions (more reliable than viewport)
    float displayWidth = 0.0f;
    float displayHeight = 0.0f;

    IDirect3DSurface9* pBackBuffer = nullptr;
    if (SUCCEEDED(pDevice->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &pBackBuffer))) {
        D3DSURFACE_DESC desc;
        if (SUCCEEDED(pBackBuffer->GetDesc(&desc))) {
            displayWidth = (float)desc.Width;
            displayHeight = (float)desc.Height;
        }
        pBackBuffer->Release();
    }

    // Fallback to viewport if backbuffer query failed
    if (displayWidth <= 0.0f || displayHeight <= 0.0f) {
        D3DVIEWPORT9 viewport;
        pDevice->GetViewport(&viewport);
        displayWidth = (float)viewport.Width;
        displayHeight = (float)viewport.Height;
    }

    io.DisplaySize.x = displayWidth;
    io.DisplaySize.y = displayHeight;

    // Detect resolution change and force layout recalculation
    bool resolutionChanged = (displayWidth != g_lastDisplayWidth || displayHeight != g_lastDisplayHeight);
    if (resolutionChanged) {
        g_lastDisplayWidth = displayWidth;
        g_lastDisplayHeight = displayHeight;
        // Force layout recalculation by resetting position
        if (g_toastCurrentX != -10000.0f && g_toastTimer > 0.0f) {
            g_toastCurrentX = -10000.0f;
        }
    }

    if (!g_pToastTexture) LoadDdsTexture(pDevice);

    const float UI_SCALE = 0.65f;
    const float SCREEN_PADDING = 10.0f;
    const float TEXT_PADDING_X = 20.0f * UI_SCALE;
    const float TEXT_PADDING_Y = 15.0f * UI_SCALE;
    const float ROUNDING = 8.0f * UI_SCALE;

    // Calculate layout every frame (not cached) to handle resolution changes and font issues
    bool needsLayoutCalc = (g_toastTimer > 0.0f || g_toastCurrentX != -10000.0f);
    bool needsInitialPosition = (g_toastTimer > 0.0f && g_toastCurrentX == -10000.0f);

    if (needsLayoutCalc) {
        bool fontPushed = false;
        if (g_pToastFont && g_pToastFont->IsLoaded()) {
            ImGui::PushFont(g_pToastFont);
            fontPushed = true;
        }

        ImVec2 s1 = ImGui::CalcTextSize(g_currentBgmInfo.songName.c_str());
        g_currentLineHeight = s1.y > 0 ? s1.y : 28.0f;

        float w1 = s1.x;
        float w2 = ImGui::CalcTextSize(g_currentBgmInfo.japaneseName.c_str()).x;
        float w3 = ImGui::CalcTextSize(g_currentBgmInfo.album.c_str()).x;
        std::string discTrack = (!g_currentBgmInfo.disc.empty() || !g_currentBgmInfo.track.empty())
            ? "Disc " + g_currentBgmInfo.disc + ", Track " + g_currentBgmInfo.track : "";
        float w4 = ImGui::CalcTextSize(discTrack.c_str()).x;
        float text_width = std::max({w1, w2, w3, w4});

        // Minimum width fallback (estimate ~8px per character if font fails)
        if (text_width < 50.0f) {
            size_t maxLen = std::max({
                g_currentBgmInfo.songName.length(),
                g_currentBgmInfo.japaneseName.length(),
                g_currentBgmInfo.album.length(),
                discTrack.length()
            });
            text_width = std::max(text_width, (float)maxLen * 8.0f);
        }

        int line_count = 0;
        if (!g_currentBgmInfo.songName.empty()) line_count++;
        if (!g_currentBgmInfo.japaneseName.empty()) line_count++;
        if (!discTrack.empty()) line_count++;
        if (!g_currentBgmInfo.album.empty()) line_count++;

        g_currentTextHeight = g_currentLineHeight * (float)line_count + (g_currentLineHeight * 0.2f * (line_count - 1));

        if (fontPushed) ImGui::PopFont();

        g_currentToastTotalHeight = g_currentTextHeight + (TEXT_PADDING_Y * 2.0f);
        g_currentNoteIconWidth = g_currentToastTotalHeight;
        g_currentBoxWidth = text_width + (TEXT_PADDING_X * 2.0f);
        g_currentToastTotalWidth = g_currentNoteIconWidth + g_currentBoxWidth;

        if (needsInitialPosition) {
            g_toastCurrentX = io.DisplaySize.x + SCREEN_PADDING;
        }
    }

    if (g_toastCurrentX != -10000.0f) {
        // Clamp toast width to reasonable maximum (80% of screen width)
        float maxToastWidth = io.DisplaySize.x * 0.8f;
        float effectiveWidth = std::min(g_currentToastTotalWidth, maxToastWidth);

        float target = io.DisplaySize.x - effectiveWidth - SCREEN_PADDING;
        // Safety: ensure target is always positive
        if (target < SCREEN_PADDING) target = SCREEN_PADDING;

        float dt = io.DeltaTime;
        if (g_toastTimer > 0.0f) {
            if (g_toastCurrentX > target) g_toastCurrentX -= 1500.0f * dt;
            if (g_toastCurrentX < target) g_toastCurrentX = target; // Clamp to target
            g_toastTimer -= dt;
        } else {
            if (g_toastCurrentX < io.DisplaySize.x + SCREEN_PADDING) g_toastCurrentX += 1500.0f * dt;
            else g_toastCurrentX = -10000.0f;
        }
    }

    if (g_toastCurrentX != -10000.0f) {
        ImDrawList* dl = ImGui::GetForegroundDrawList();
        ImVec2 p_box(g_toastCurrentX + g_currentNoteIconWidth, SCREEN_PADDING);
        dl->AddRectFilled(p_box, ImVec2(p_box.x + g_currentBoxWidth, SCREEN_PADDING + g_currentToastTotalHeight), IM_COL32(0, 0, 0, 180), ROUNDING);

        if (g_pToastTexture) {
            dl->AddImage((void*)g_pToastTexture,
                ImVec2(g_toastCurrentX, SCREEN_PADDING),
                ImVec2(g_toastCurrentX + g_currentNoteIconWidth, SCREEN_PADDING + g_currentToastTotalHeight));
        }

        float current_y = SCREEN_PADDING + (g_currentToastTotalHeight - g_currentTextHeight) * 0.5f;

        bool fontPushed = false;
        if (g_pToastFont && g_pToastFont->IsLoaded()) {
            ImGui::PushFont(g_pToastFont);
            fontPushed = true;
        }

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

        if (fontPushed) ImGui::PopFont();
    }

    // Debug: Log rendering state periodically
    static int renderLogCounter = 0;
    if (g_toastTimer > 0.0f && renderLogCounter++ % 60 == 0) {
        Log("Rendering toast: X=" + std::to_string(g_toastCurrentX) +
            " Timer=" + std::to_string(g_toastTimer) +
            " Display=" + std::to_string(io.DisplaySize.x) + "x" + std::to_string(io.DisplaySize.y));
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

HANDLE WINAPI Detour_CreateFileW(LPCWSTR lpFileName, DWORD dwAccess, DWORD dwShare, LPSECURITY_ATTRIBUTES lpSec, DWORD dwDisp, DWORD dwFlags, HANDLE hTemplate) {
    if (lpFileName && IsTargetFileW(lpFileName)) {
        if (g_bufferMutex.try_lock()) {
            if (!g_bNewBgmAvailable) {
                WCharToString(lpFileName, g_bgmFilenameBuffer, MAX_PATH);
                g_bNewBgmAvailable = true;
            }
            g_bufferMutex.unlock();
        }
    }
    return g_pfnOriginalCreateFileW(lpFileName, dwAccess, dwShare, lpSec, dwDisp, dwFlags, hTemplate);
}

HANDLE WINAPI Detour_CreateFileA(LPCSTR lpFileName, DWORD dwAccess, DWORD dwShare, LPSECURITY_ATTRIBUTES lpSec, DWORD dwDisp, DWORD dwFlags, HANDLE hTemplate) {
    if (lpFileName && IsTargetFile(lpFileName)) {
        if (g_bufferMutex.try_lock()) {
            if (!g_bNewBgmAvailable) {
                strcpy_s(g_bgmFilenameBuffer, MAX_PATH, lpFileName);
                g_bNewBgmAvailable = true;
            }
            g_bufferMutex.unlock();
        }
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
        Log("TOAST TRIGGERED: " + info.songName + " | ImGui initialized: " + (g_imguiInitialized ? "YES" : "NO"));
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

void BgmWorkerThread() {
    Log("BGM Worker Thread started.");
    while (g_bWorkerThreadActive) {
        if (g_bNewBgmAvailable) {
            std::string fname;
            {
                std::lock_guard<std::mutex> lock(g_bufferMutex);
                fname = g_bgmFilenameBuffer;
                g_bNewBgmAvailable = false;
            }
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
void __declspec(naked) Detour_PlayBgm_Xanadu() {
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

// =============================================================
// YS7 / GENERIC FOPEN RVA HOOK (NAKED ASSEMBLY)
// =============================================================
static void* g_pfnOriginalInternalFopen = nullptr;

// Try to extract .ogg filename from a memory address
bool __stdcall TryExtractOgg(DWORD candidateAddr) {
    // 1. Basic pointer validation
    if (candidateAddr < 0x10000 || candidateAddr > 0x7FFFFFFF) return false;
    if (IsBadReadPtr((void*)candidateAddr, 4)) return false;

    // 2. Check first char (must be printable text, not control char)
    char* pStr = (char*)candidateAddr;
    if (pStr[0] < 32 || pStr[0] > 126) return false;

    // 3. Scan length (limit to MAX_PATH to prevent hangs)
    size_t len = 0;
    while (len < MAX_PATH) {
        if (IsBadReadPtr(&pStr[len], 1)) return false;
        if (pStr[len] == 0) break;
        len++;
    }

    if (len < 4 || len >= MAX_PATH) return false;

    // 4. Check extension for .ogg (Case insensitive)
    if (tolower(pStr[len - 1]) == 'g' &&
        tolower(pStr[len - 2]) == 'g' &&
        tolower(pStr[len - 3]) == 'o' &&
        pStr[len - 4] == '.')
    {
        // FOUND IT!
        if (g_bufferMutex.try_lock()) {
            if (!g_bNewBgmAvailable) {
                strcpy_s(g_bgmFilenameBuffer, MAX_PATH, pStr);
                g_bNewBgmAvailable = true;
            }
            g_bufferMutex.unlock();
        }
        return true;
    }

    return false;
}

void __declspec(naked) Detour_InternalFopen() {
    __asm {
        // 1. Save ALL registers
        pushad
        pushfd

        // 2. SEARCH FOR THE FILENAME
        // Check ECX (Fastcall/Thiscall)
        push ecx
        call TryExtractOgg
        test al, al
        jnz _found

        // Check Stack Argument 1 (ESP+40 after pushad/pushfd)
        mov eax, [esp + 40]
        push eax
        call TryExtractOgg
        test al, al
        jnz _found

        // Check Stack Argument 2
        mov eax, [esp + 44]
        push eax
        call TryExtractOgg

    _found:
        // 3. Restore ALL registers
        popfd
        popad

        // 4. Jump to original
        jmp [g_pfnOriginalInternalFopen]
    }
}
#endif

// =============================================================
// HOOK INITIALIZATION
// =============================================================
HRESULT WINAPI Detour_CreateDevice(IDirect3D9* pD3D, UINT Adapter, D3DDEVTYPE DeviceType, HWND hFocusWindow, DWORD BehaviorFlags, D3DPRESENT_PARAMETERS* pPresentationParameters, IDirect3DDevice9** ppReturnedDeviceInterface) {
    HRESULT hr = g_pfnOriginalCreateDevice(pD3D, Adapter, DeviceType, hFocusWindow, BehaviorFlags, pPresentationParameters, ppReturnedDeviceInterface);
    if (SUCCEEDED(hr) && ppReturnedDeviceInterface && !g_bEndSceneHooked) {
        // Capture window handle from CreateDevice call
        if (hFocusWindow) {
            g_hWindow = hFocusWindow;
            Log("Window captured from CreateDevice: " + std::to_string((uintptr_t)hFocusWindow));
        } else if (pPresentationParameters && pPresentationParameters->hDeviceWindow) {
            g_hWindow = pPresentationParameters->hDeviceWindow;
            Log("Window captured from PresentationParameters: " + std::to_string((uintptr_t)g_hWindow));
        }

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

    Log("Direct3DCreate9 called, setting up hooks...");

    LoadBgmMap();

    if (pD3D) {
        void** pVTable = *(void***)pD3D;
        MH_CreateHook((LPVOID)pVTable[16], &Detour_CreateDevice, (LPVOID*)&g_pfnOriginalCreateDevice);
        MH_EnableHook(MH_ALL_HOOKS);
        Log("CreateDevice hooked.");
    }

    // File hooks will be set up later in InitImGui to avoid interfering with game audio init

#ifdef _M_IX86
    if (g_Config.useRvaHook && g_Config.bgmFuncRVA != 0) {
        uintptr_t baseAddr = (uintptr_t)GetModuleHandle(NULL);
        uintptr_t targetAddr = baseAddr + g_Config.bgmFuncRVA;

        // Start worker thread for RVA hooks that use the buffer
        if (g_CurrentGame != GameID::XanaduNext) {
            g_bWorkerThreadActive = true;
            g_workerThread = std::thread(BgmWorkerThread);
        }

        // Use appropriate hook for each game type
        if (g_CurrentGame == GameID::XanaduNext) {
            // Xanadu Next: song ID in ECX
            if (MH_CreateHook((LPVOID)targetAddr, &Detour_PlayBgm_Xanadu, (LPVOID*)&g_pfnOriginalPlayBgm) == MH_OK) {
                MH_EnableHook((LPVOID)targetAddr);
                Log("Xanadu Next BGM Hook Successful at: 0x" + std::to_string(targetAddr));
            } else {
                Log("FATAL: Failed to hook Xanadu Next BGM function");
            }
        } else {
            // Ys7 and similar: filename string in ECX or stack
            if (MH_CreateHook((LPVOID)targetAddr, &Detour_InternalFopen, (LPVOID*)&g_pfnOriginalInternalFopen) == MH_OK) {
                MH_EnableHook((LPVOID)targetAddr);
                Log("InternalFopen Hook Successful at: 0x" + std::to_string(targetAddr));
            } else {
                Log("FATAL: Failed to hook InternalFopen function");
            }
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
        DetectProxyType(hModule);
        InitializeMod();
    }
    else if (fdwReason == DLL_PROCESS_DETACH) {
        // Minimal cleanup only - full cleanup causes hanging in some games
        // (Ys: The Oath in Felghana, Xanadu Next, etc.)
        g_bWorkerThreadActive = false;
        // Don't join thread or release resources - let OS handle it
    }

    return TRUE;
}