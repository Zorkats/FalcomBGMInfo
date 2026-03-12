#define NOMINMAX
#include <windows.h>
#include <shlwapi.h>
#pragma comment(lib, "shlwapi.lib")
#include <psapi.h>
#pragma comment(lib, "psapi.lib")
#include "universal_proxy_x86.h"

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
    Unknown, XanaduNext, YsSeven, YsOrigin, YsVI, YsFelghana, SkyFC, SkySC, Sky3rd
};

struct GameConfig {
    std::string gameName;
    std::string windowTitlePart;
    std::vector<std::string> yamlFiles;
    bool useFileHook;
    bool useRvaHook;
    uintptr_t bgmFuncRVA;
    std::string signature;
};

struct ModConfig {
    bool ignoreCooldown = false;
    int showHotkey = VK_F2;
    int menuHotkey = VK_F1;
    float toastDuration = 5.0f;
    int cooldownHours = 5;
    bool showJapanese = true;
    float uiScale = 0.65f;
};

struct BgmInfo {
    std::string songName, japaneseName, disc, track, album, rawFileName;
};

// =============================================================
// GLOBALS
// =============================================================
GameConfig g_Config;
ModConfig g_ModConfig;
GameID g_CurrentGame = GameID::Unknown;
static std::mutex g_LogMutex;
static std::string g_modDirectory = "";
static bool g_IsGog = false;

static std::map<std::string, BgmInfo> g_bgmMap;
static BgmInfo g_currentBgmInfo;
static std::map<std::string, std::chrono::steady_clock::time_point> g_songLastShown;
static std::string g_lastTriggeredFile = "";
static std::mutex g_BgmMutex;

static float g_toastTimer = 0.0f;
static float g_toastCurrentX = -10000.0f;
static float g_lastDisplayWidth = 0.0f;
static float g_lastDisplayHeight = 0.0f;

static bool g_imguiInitialized = false;
static bool g_showMenu = false;
static bool g_isRemapping = false;
static bool g_bEndSceneHooked = false;
static HWND g_hWindow = nullptr;
static ImFont* g_pMenuFont = nullptr;
static ImFont* g_pToastFont = nullptr;
static LPDIRECT3DTEXTURE9 g_pToastTexture = nullptr;
static float g_TextureWidth = 0.0f;
static float g_TextureHeight = 0.0f;

static std::thread g_workerThread;
static std::mutex g_bufferMutex;
static char g_bgmFilenameBuffer[MAX_PATH];
static std::atomic<bool> g_bNewBgmAvailable = false;
static std::atomic<bool> g_bWorkerThreadActive = true;

static WNDPROC g_pfnOriginalWndProc = NULL;
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// =============================================================
// UTILS
// =============================================================
std::string ResolveModDirectory() {
    char path[MAX_PATH];
    HMODULE hModule = NULL;
    GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCSTR)&ResolveModDirectory, &hModule);
    GetModuleFileNameA(hModule, path, MAX_PATH);
    PathRemoveFileSpecA(path);
    return std::string(path);
}

void Log(const std::string& message) {
    std::string dir = g_modDirectory;
    if (dir.empty()) dir = ResolveModDirectory();
    std::lock_guard<std::mutex> lock(g_LogMutex);
    std::string path = dir + "\\mod_log.txt";
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

void SaveConfig() {
    std::string path = g_modDirectory + "\\mod_config.yaml";
    try {
        YAML::Emitter out;
        out << YAML::BeginMap;
        out << YAML::Key << "IgnoreCooldown" << YAML::Value << g_ModConfig.ignoreCooldown;
        out << YAML::Key << "ShowHotkey" << YAML::Value << g_ModConfig.showHotkey;
        out << YAML::Key << "MenuHotkey" << YAML::Value << g_ModConfig.menuHotkey;
        out << YAML::Key << "ToastDuration" << YAML::Value << g_ModConfig.toastDuration;
        out << YAML::Key << "CooldownHours" << YAML::Value << g_ModConfig.cooldownHours;
        out << YAML::Key << "ShowJapanese" << YAML::Value << g_ModConfig.showJapanese;
        out << YAML::Key << "UIScale" << YAML::Value << g_ModConfig.uiScale;
        out << YAML::EndMap;
        std::ofstream fout(path);
        fout << out.c_str();
    } catch (...) {}
}

void LoadConfig() {
    std::string path = g_modDirectory + "\\mod_config.yaml";
    std::ifstream fin(path);
    if (!fin.is_open()) {
        SaveConfig();
        return;
    }
    try {
        YAML::Node config = YAML::Load(fin);
        if (config["IgnoreCooldown"]) g_ModConfig.ignoreCooldown = config["IgnoreCooldown"].as<bool>();
        if (config["AlwaysShow"]) g_ModConfig.ignoreCooldown = config["AlwaysShow"].as<bool>();
        if (config["ShowHotkey"]) g_ModConfig.showHotkey = config["ShowHotkey"].as<int>();
        if (config["MenuHotkey"]) g_ModConfig.menuHotkey = config["MenuHotkey"].as<int>();
        if (config["ToastDuration"]) g_ModConfig.toastDuration = config["ToastDuration"].as<float>();
        if (config["CooldownHours"]) g_ModConfig.cooldownHours = config["CooldownHours"].as<int>();
        if (config["ShowJapanese"]) g_ModConfig.showJapanese = config["ShowJapanese"].as<bool>();
        if (config["UIScale"]) g_ModConfig.uiScale = config["UIScale"].as<float>();
    } catch (...) {}
}

std::string GetKeyName(int vkCode) {
    if (vkCode == 0) return "None";
    switch (vkCode) {
        case VK_LBUTTON: return "Left Mouse"; case VK_RBUTTON: return "Right Mouse";
        case VK_MBUTTON: return "Middle Mouse"; case VK_BACK: return "Backspace";
        case VK_TAB: return "Tab"; case VK_RETURN: return "Enter";
        case VK_ESCAPE: return "Esc"; case VK_SPACE: return "Space";
    }
    char name[64];
    UINT scanCode = MapVirtualKeyA(vkCode, MAPVK_VK_TO_VSC);
    LONG lParamValue = (scanCode << 16);
    if ((vkCode >= 33 && vkCode <= 46) || (vkCode >= 91 && vkCode <= 93) || (vkCode >= 106 && vkCode <= 111) || (vkCode >= 144 && vkCode <= 145))
        lParamValue |= (1 << 24);
    if (GetKeyNameTextA(lParamValue, name, sizeof(name)) > 0) return std::string(name);
    std::stringstream ss; ss << "Key: 0x" << std::hex << vkCode; return ss.str();
}

// =============================================================
// INPUT BLOCKING
// =============================================================
typedef SHORT(WINAPI* PFN_GETASYNCKEYSTATE)(int);
static PFN_GETASYNCKEYSTATE g_pfnOriginalGetAsyncKeyState = nullptr;
SHORT WINAPI Detour_GetAsyncKeyState(int vKey) {
    if (g_showMenu && !g_isRemapping) return 0;
    return g_pfnOriginalGetAsyncKeyState(vKey);
}

typedef SHORT(WINAPI* PFN_GETKEYSTATE)(int);
static PFN_GETKEYSTATE g_pfnOriginalGetKeyState = nullptr;
SHORT WINAPI Detour_GetKeyState(int nVirtKey) {
    if (g_showMenu && !g_isRemapping) return 0;
    return g_pfnOriginalGetKeyState(nVirtKey);
}

typedef BOOL(WINAPI* PFN_GETKEYBOARDSTATE)(PBYTE);
static PFN_GETKEYBOARDSTATE g_pfnOriginalGetKeyboardState = nullptr;
BOOL WINAPI Detour_GetKeyboardState(PBYTE lpKeyState) {
    if (g_showMenu && !g_isRemapping && lpKeyState) { memset(lpKeyState, 0, 256); return TRUE; }
    return g_pfnOriginalGetKeyboardState(lpKeyState);
}

typedef BOOL(WINAPI* PFN_SETCURSORPOS)(int, int);
static PFN_SETCURSORPOS g_pfnOriginalSetCursorPos = nullptr;
BOOL WINAPI Detour_SetCursorPos(int X, int Y) {
    if (g_showMenu && !g_isRemapping) return TRUE;
    return g_pfnOriginalSetCursorPos(X, Y);
}

typedef BOOL(WINAPI* PFN_CLIPCURSOR)(const RECT*);
static PFN_CLIPCURSOR g_pfnOriginalClipCursor = nullptr;
BOOL WINAPI Detour_ClipCursor(const RECT* lpRect) {
    if (g_showMenu && !g_isRemapping) return TRUE;
    return g_pfnOriginalClipCursor(lpRect);
}

// =============================================================
// MEMORY SCANNER
// =============================================================
namespace MemoryScanner {
    uintptr_t Scan(const std::string& pattern) {
        if (pattern.empty()) return 0;
        
        HMODULE hModule = GetModuleHandle(NULL);
        MODULEINFO mi;
        if (!GetModuleInformation(GetCurrentProcess(), hModule, &mi, sizeof(mi))) return 0;
        
        uint8_t* baseAddr = (uint8_t*)mi.lpBaseOfDll;
        size_t imageSize = mi.SizeOfImage;
        
        std::vector<int> bytes;
        std::stringstream ss(pattern);
        std::string word;
        while (ss >> word) {
            if (word == "?" || word == "??") bytes.push_back(-1);
            else bytes.push_back(std::stoi(word, nullptr, 16));
        }
        if (bytes.empty()) return 0;
        
        uint8_t* currentAddr = baseAddr;
        while (currentAddr < baseAddr + imageSize) {
            MEMORY_BASIC_INFORMATION mbi;
            if (!VirtualQuery(currentAddr, &mbi, sizeof(mbi))) break;
            
            if (mbi.State == MEM_COMMIT && 
                (mbi.Protect == PAGE_EXECUTE_READ || mbi.Protect == PAGE_EXECUTE_READWRITE || mbi.Protect == PAGE_READONLY || mbi.Protect == PAGE_READWRITE)) {
                
                uint8_t* regionStart = (uint8_t*)mbi.BaseAddress;
                size_t regionSize = mbi.RegionSize;
                
                if (regionStart < baseAddr) {
                    size_t diff = baseAddr - regionStart;
                    if (regionSize > diff) { regionSize -= diff; regionStart = baseAddr; } else regionSize = 0;
                }
                if (regionStart + regionSize > baseAddr + imageSize) {
                    regionSize = (baseAddr + imageSize) - regionStart;
                }

                if (regionSize >= bytes.size()) {
                    for (size_t i = 0; i <= regionSize - bytes.size(); i++) {
                        bool found = true;
                        for (size_t j = 0; j < bytes.size(); j++) {
                            if (bytes[j] != -1 && regionStart[i + j] != (uint8_t)bytes[j]) {
                                found = false;
                                break;
                            }
                        }
                        if (found) return (uintptr_t)(regionStart + i);
                    }
                }
            }
            currentAddr = (uint8_t*)mbi.BaseAddress + mbi.RegionSize;
        }
        return 0;
    }
}

// Global thread-local guard to prevent recursion crashes in fopen/Log
static thread_local bool g_InHookExecution = false;

bool __stdcall TryExtract(DWORD addr) {
    if (addr < 0x10000 || addr > 0x7FFFFFFF) return false;
    
    char* p = (char*)addr;
    __try {
        if (p[0] < 32 || p[0] > 126) return false;
        size_t l = 0;
        while (l < MAX_PATH && p[l] != 0) l++;
        if (l < 4) return false;
        
        if (tolower(p[l-1]) == 'g' && tolower(p[l-2]) == 'g' && tolower(p[l-3]) == 'o' && p[l-4] == '.') {
            if (g_bufferMutex.try_lock()) {
                if (!g_bNewBgmAvailable) {
                    strcpy_s(g_bgmFilenameBuffer, MAX_PATH, p);
                    g_bNewBgmAvailable = true;
                }
                g_bufferMutex.unlock();
            }
            return true;
        }
    } __except(EXCEPTION_EXECUTE_HANDLER) { return false; }
    return false;
}

// =============================================================
// CRT HOOKS
// =============================================================
typedef FILE* (__cdecl* PFN_FOPEN)(const char*, const char*);
static PFN_FOPEN g_pfnOriginalFopenUCRT = nullptr;
static PFN_FOPEN g_pfnOriginalFopenMSVCRT = nullptr;

FILE* __cdecl Detour_Fopen_UCRT(const char* filename, const char* mode) {
    if (g_InHookExecution) return g_pfnOriginalFopenUCRT(filename, mode);
    g_InHookExecution = true;
    if (filename) TryExtract((DWORD)filename);
    FILE* res = g_pfnOriginalFopenUCRT(filename, mode);
    g_InHookExecution = false;
    return res;
}

FILE* __cdecl Detour_Fopen_MSVCRT(const char* filename, const char* mode) {
    if (g_InHookExecution) return g_pfnOriginalFopenMSVCRT(filename, mode);
    g_InHookExecution = true;
    if (filename) TryExtract((DWORD)filename);
    FILE* res = g_pfnOriginalFopenMSVCRT(filename, mode);
    g_InHookExecution = false;
    return res;
}

// =============================================================
// GAME DETECTION
// =============================================================
void DetectAndConfigure() {
    char exePath[MAX_PATH];
    GetModuleFileNameA(NULL, exePath, MAX_PATH);
    std::string exe = exePath;
    std::transform(exe.begin(), exe.end(), exe.begin(), ::tolower);

    HANDLE hFile = CreateFileA(exePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    DWORD size = 0;
    if (hFile != INVALID_HANDLE_VALUE) {
        size = GetFileSize(hFile, NULL);
        CloseHandle(hFile);
    }

    if (exe.find("xanadu.exe") != std::string::npos) {
        g_CurrentGame = GameID::XanaduNext;
        g_Config.gameName = "Xanadu Next";
        g_Config.yamlFiles.push_back("BgmMap_XanaduNext.yaml");
        g_Config.useRvaHook = true;
        if (size == 1259008) { // GOG
            g_IsGog = true;
            g_Config.bgmFuncRVA = 0xDB990;
            g_Config.signature = ""; 
        } else { // Steam
            g_IsGog = false;
            g_Config.bgmFuncRVA = 0xC9B60;
            g_Config.signature = ""; 
        }
    }
    else if (exe.find("ys7.exe") != std::string::npos) {
        g_CurrentGame = GameID::YsSeven;
        g_Config.gameName = "Ys Seven";
        g_Config.yamlFiles.push_back("BgmMap_Ys7.yaml");
        g_Config.useRvaHook = true;
        g_Config.useFileHook = false;
        if (size == 2300928) { // GOG
            g_IsGog = true;
            g_Config.bgmFuncRVA = 0xEC6E0;
            g_Config.signature = "";
        } else if (size == 2429296) { // Steam
            g_IsGog = false;
            g_Config.bgmFuncRVA = 0x1445E0;
            g_Config.signature = "";
        }
    }
    else if (exe.find("yso_win.exe") != std::string::npos) {
        g_CurrentGame = GameID::YsOrigin;
        g_Config.gameName = "Ys Origin";
        g_Config.yamlFiles.push_back("BgmMap_YsOrigin.yaml");
        g_Config.useFileHook = true;
    }
    else if (exe.find("ys6_win_dx9.exe") != std::string::npos) {
        g_CurrentGame = GameID::YsVI;
        g_Config.gameName = "Ys VI";
        g_Config.yamlFiles.push_back("BgmMap_Ys6.yaml");
        g_Config.useFileHook = true;
    }
    else if (exe.find("ysf_win_dx9.exe") != std::string::npos) {
        g_CurrentGame = GameID::YsFelghana;
        g_Config.gameName = "Ys: Felghana";
        g_Config.yamlFiles.push_back("BgmMap_YsFelghana.yaml");
        g_Config.useFileHook = true;
    }
    else if (exe.find("ed6_win") != std::string::npos) {
        if (exe.find("win2") != std::string::npos) {
            g_CurrentGame = GameID::SkySC;
            g_Config.gameName = "Trails SC";
            g_Config.yamlFiles.push_back("BgmMap_SkySC.yaml");
        }
        else if (exe.find("win3") != std::string::npos) {
            g_CurrentGame = GameID::Sky3rd;
            g_Config.gameName = "Trails 3rd";
            g_Config.yamlFiles.push_back("BgmMap_Sky3rd.yaml");
        }
        else {
            g_CurrentGame = GameID::SkyFC;
            g_Config.gameName = "Trails FC";
            g_Config.yamlFiles.push_back("BgmMap_SkyFC.yaml");
        }
        g_Config.useFileHook = true;
    }
    else {
        g_CurrentGame = GameID::Unknown;
        g_Config.gameName = "Unknown Game";
        g_Config.yamlFiles.push_back("BgmMap.yaml");
        g_Config.useFileHook = true;
    }
    Log("Detected: " + g_Config.gameName);
}

// =============================================================
// INPUT & WNDPROC
// =============================================================
LRESULT WINAPI WndProc(const HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (uMsg == WM_KEYDOWN) {
        if (wParam == g_ModConfig.menuHotkey) {
            g_showMenu = !g_showMenu; ImGuiIO& io = ImGui::GetIO();
            if (g_showMenu) {
                io.ConfigFlags &= ~ImGuiConfigFlags_NoMouse;
                io.ConfigFlags &= ~ImGuiConfigFlags_NoKeyboard;
                io.MouseDrawCursor = true;
            } else {
                io.ConfigFlags |= ImGuiConfigFlags_NoMouse;
                io.ConfigFlags |= ImGuiConfigFlags_NoKeyboard;
                io.MouseDrawCursor = false;
                SaveConfig();
            }
            return 0;
        }
        if (wParam == g_ModConfig.showHotkey && !g_showMenu) {
            g_toastTimer = g_ModConfig.toastDuration;
            g_toastCurrentX = -10000.0f;
            return 0;
        }
    }
    
    if (g_showMenu) {
        ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam);
        if (!g_isRemapping) {
            switch (uMsg) {
                case WM_LBUTTONDOWN: case WM_LBUTTONUP: case WM_LBUTTONDBLCLK:
                case WM_RBUTTONDOWN: case WM_RBUTTONUP: case WM_RBUTTONDBLCLK:
                case WM_MBUTTONDOWN: case WM_MBUTTONUP: case WM_MBUTTONDBLCLK:
                case WM_XBUTTONDOWN: case WM_XBUTTONUP: case WM_XBUTTONDBLCLK:
                case WM_MOUSEWHEEL:  case WM_MOUSEHWHEEL: case WM_MOUSEMOVE:
                case WM_INPUT:
                case WM_KEYDOWN: case WM_KEYUP: case WM_SYSKEYDOWN: case WM_SYSKEYUP:
                    return 1;
            }
            return TRUE;
        }
    }
    
    ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam);
    return CallWindowProc(g_pfnOriginalWndProc, hWnd, uMsg, wParam, lParam);
}

// =============================================================
// GRAPHICS HOOKS
// =============================================================
typedef HRESULT(WINAPI* PFN_ENDSCENE)(IDirect3DDevice9* pDevice);
static PFN_ENDSCENE g_pfnOriginalEndScene = nullptr;
typedef HRESULT(WINAPI* PFN_RESET)(IDirect3DDevice9* pDevice, D3DPRESENT_PARAMETERS* pPresentationParameters);
static PFN_RESET g_pfnOriginalReset = nullptr;
typedef IDirect3D9* (WINAPI* PFN_DIRECT3DCREATE9)(UINT SDKVersion);
static PFN_DIRECT3DCREATE9 g_pfnOriginalDirect3DCreate9 = nullptr;
typedef HRESULT(WINAPI* PFN_CREATEDEVICE)(IDirect3D9* pD3D, UINT Adapter, D3DDEVTYPE DeviceType, HWND hFocusWindow, DWORD BehaviorFlags, D3DPRESENT_PARAMETERS* pPresentationParameters, IDirect3DDevice9** ppReturnedDeviceInterface);
static PFN_CREATEDEVICE g_pfnOriginalCreateDevice = nullptr;

typedef HANDLE(WINAPI* PFN_CREATEFILEW)(LPCWSTR, DWORD, DWORD, LPSECURITY_ATTRIBUTES, DWORD, DWORD, HANDLE);
typedef HANDLE(WINAPI* PFN_CREATEFILEA)(LPCSTR, DWORD, DWORD, LPSECURITY_ATTRIBUTES, DWORD, DWORD, HANDLE);
static PFN_CREATEFILEW g_pfnOriginalCreateFileW = nullptr;
static PFN_CREATEFILEA g_pfnOriginalCreateFileA = nullptr;
HANDLE WINAPI Detour_CreateFileW(LPCWSTR lpFileName, DWORD dwAccess, DWORD dwShare, LPSECURITY_ATTRIBUTES lpSec, DWORD dwDisp, DWORD dwFlags, HANDLE hTemplate);
HANDLE WINAPI Detour_CreateFileA(LPCSTR lpFileName, DWORD dwAccess, DWORD dwShare, LPSECURITY_ATTRIBUTES lpSec, DWORD dwDisp, DWORD dwFlags, HANDLE hTemplate);
void BgmWorkerThread();

void LoadDdsTexture(IDirect3DDevice9* pDevice) {
    std::string path = g_modDirectory + "\\assets/bgm_info.dds";
    if (SUCCEEDED(D3DXCreateTextureFromFileA(pDevice, path.c_str(), &g_pToastTexture))) {
        D3DSURFACE_DESC desc;
        g_pToastTexture->GetLevelDesc(0, &desc);
        g_TextureWidth = (float)desc.Width;
        g_TextureHeight = (float)desc.Height;
    }
}

void InitImGui(IDirect3DDevice9* pDevice) {
    if (g_imguiInitialized) return;
    D3DDEVICE_CREATION_PARAMETERS params;
    if (SUCCEEDED(pDevice->GetCreationParameters(&params))) g_hWindow = params.hFocusWindow;
    if (!g_hWindow) return;
    
    g_pfnOriginalWndProc = (WNDPROC)SetWindowLongPtr(g_hWindow, GWLP_WNDPROC, (LONG_PTR)WndProc);
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.ConfigFlags |= ImGuiConfigFlags_NoMouse;
    io.ConfigFlags |= ImGuiConfigFlags_NoKeyboard;
    
    std::string f1 = g_modDirectory + "\\assets/mod_font.otf";
    std::string f2 = g_modDirectory + "\\assets/mod_font_japanese.ttf";
    const ImWchar* r = io.Fonts->GetGlyphRangesJapanese();
    
    g_pMenuFont = io.Fonts->AddFontFromFileTTF(f1.c_str(), 18.0f);
    if (g_pMenuFont) { ImFontConfig cfg1; cfg1.MergeMode = true; io.Fonts->AddFontFromFileTTF(f2.c_str(), 18.0f, &cfg1, r); }
    g_pToastFont = io.Fonts->AddFontFromFileTTF(f1.c_str(), 28.0f);
    if (g_pToastFont) { ImFontConfig cfg2; cfg2.MergeMode = true; io.Fonts->AddFontFromFileTTF(f2.c_str(), 28.0f, &cfg2, r); }
    
    LoadDdsTexture(pDevice);
    ImGui_ImplWin32_Init(g_hWindow);
    ImGui_ImplDX9_Init(pDevice);
    g_imguiInitialized = true;
    
    if (g_Config.useFileHook) {
        g_bWorkerThreadActive = true;
        g_workerThread = std::thread(BgmWorkerThread);
        HMODULE hK32 = GetModuleHandleA("kernel32.dll");
        MH_CreateHook(GetProcAddress(hK32, "CreateFileW"), &Detour_CreateFileW, (LPVOID*)&g_pfnOriginalCreateFileW);
        MH_CreateHook(GetProcAddress(hK32, "CreateFileA"), &Detour_CreateFileA, (LPVOID*)&g_pfnOriginalCreateFileA);
        MH_EnableHook(MH_ALL_HOOKS);
    }
}

HRESULT WINAPI My_EndScene(IDirect3DDevice9* pDevice) {
    IDirect3DSurface9* pCurrentRT = nullptr;
    IDirect3DSurface9* pBackBuffer = nullptr;
    bool isBackBuffer = false;
    if (SUCCEEDED(pDevice->GetRenderTarget(0, &pCurrentRT)) && pCurrentRT) {
        if (SUCCEEDED(pDevice->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &pBackBuffer)) && pBackBuffer) {
            isBackBuffer = (pCurrentRT == pBackBuffer);
            pBackBuffer->Release();
        }
        pCurrentRT->Release();
    }
    if (!isBackBuffer) return g_pfnOriginalEndScene(pDevice);

    InitImGui(pDevice);
    if (!g_pToastTexture && g_imguiInitialized) LoadDdsTexture(pDevice);
    
    ImGui_ImplDX9_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
    ImGuiIO& io = ImGui::GetIO();
    g_isRemapping = false;
    
    if (g_showMenu) {
        ImGui::SetNextWindowSize(ImVec2(400, 300), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Falcom BGM Info Settings", &g_showMenu)) {
            ImGui::Checkbox("Ignore Cooldown (Always Show)", &g_ModConfig.ignoreCooldown);
            ImGui::Checkbox("Show Japanese Name", &g_ModConfig.showJapanese);
            ImGui::SliderFloat("Toast Duration", &g_ModConfig.toastDuration, 1.0f, 20.0f, "%.1f seconds");
            ImGui::SliderFloat("UI Scale", &g_ModConfig.uiScale, 0.3f, 2.0f, "%.2f");
            int h = g_ModConfig.cooldownHours;
            if (ImGui::SliderInt("Cooldown (Hours)", &h, 0, 24)) g_ModConfig.cooldownHours = h;
            ImGui::Separator();
            ImGui::Text("Hotkeys:");
            auto DrawRemapper = [&](const char* label, int& key) {
                std::string kn = GetKeyName(key);
                char buf[128]; sprintf_s(buf, "%s###%s_btn", kn.c_str(), label);
                if (ImGui::Button(buf, ImVec2(140, 0))) ImGui::OpenPopup(label);
                ImGui::SameLine(); ImGui::Text("%s", label);
                if (ImGui::BeginPopup(label)) {
                    g_isRemapping = true;
                    ImGui::Text("Press any key...");
                    for (int i = 1; i < 256; i++) {
                        if (GetAsyncKeyState(i) & 0x8000) {
                            if (i != VK_LBUTTON && i != VK_RBUTTON) {
                                key = i; ImGui::CloseCurrentPopup(); g_isRemapping = false; break;
                            }
                        }
                    }
                    if (ImGui::Button("Cancel")) { ImGui::CloseCurrentPopup(); g_isRemapping = false; }
                    ImGui::EndPopup();
                }
            };
            DrawRemapper("Menu Hotkey", g_ModConfig.menuHotkey);
            DrawRemapper("Show Info Hotkey", g_ModConfig.showHotkey);
            if (ImGui::Button("Reset Cooldowns")) { std::lock_guard<std::mutex> lock(g_BgmMutex); g_songLastShown.clear(); }
            if (ImGui::Button("Save Configuration")) SaveConfig();
        }
        ImGui::End();
        if (!g_showMenu) {
            io.ConfigFlags |= ImGuiConfigFlags_NoMouse | ImGuiConfigFlags_NoKeyboard;
            io.MouseDrawCursor = false; SaveConfig();
        }
    }
    
    D3DVIEWPORT9 vp; pDevice->GetViewport(&vp);
    io.DisplaySize = ImVec2((float)vp.Width, (float)vp.Height);
    if (vp.Width != g_lastDisplayWidth || vp.Height != g_lastDisplayHeight) {
        g_lastDisplayWidth = (float)vp.Width; g_lastDisplayHeight = (float)vp.Height;
        std::lock_guard<std::mutex> lock(g_BgmMutex);
        if (g_toastCurrentX != -10000.0f && g_toastTimer > 0.0f) g_toastCurrentX = -10000.0f;
    }
    
    const float UI = g_ModConfig.uiScale, SP = 10.0f, TX = 20.0f * UI, TY = 15.0f * UI, RO = 8.0f * UI;
    {
        std::lock_guard<std::mutex> lock(g_BgmMutex);
        if ((g_toastTimer > 0.0f || g_toastCurrentX != -10000.0f) && !g_currentBgmInfo.songName.empty()) {
            bool fP = false;
            if (g_pToastFont && g_pToastFont->IsLoaded()) { ImGui::PushFont(g_pToastFont); fP = true; }
            ImVec2 s1 = ImGui::CalcTextSize(g_currentBgmInfo.songName.c_str());
            float LH = s1.y > 0 ? s1.y : (28.0f * UI);
            std::string dt = (!g_currentBgmInfo.disc.empty() || !g_currentBgmInfo.track.empty()) ? "Disc " + g_currentBgmInfo.disc + ", Track " + g_currentBgmInfo.track : "";
            ImVec2 s2 = (g_ModConfig.showJapanese && !g_currentBgmInfo.japaneseName.empty()) ? ImGui::CalcTextSize(g_currentBgmInfo.japaneseName.c_str()) : ImVec2(0,0);
            float TW = std::max({s1.x, s2.x, ImGui::CalcTextSize(dt.c_str()).x, ImGui::CalcTextSize(g_currentBgmInfo.album.c_str()).x});
            int LC = 0; if (!g_currentBgmInfo.songName.empty()) LC++; if (g_ModConfig.showJapanese && !g_currentBgmInfo.japaneseName.empty()) LC++; if (!dt.empty()) LC++; if (!g_currentBgmInfo.album.empty()) LC++;
            float TH = LH * (float)LC + (LH * 0.2f * (LC - 1)), TTH = TH + (TY * 2.0f), NIW = TTH, BW = TW + (TX * 2.0f), TTW = NIW + BW;
            float target = io.DisplaySize.x - TTW - SP; if (target < SP) target = SP;
            if (g_toastCurrentX == -10000.0f) g_toastCurrentX = io.DisplaySize.x + SP;
            float delta = io.DeltaTime;
            if (g_toastTimer > 0.0f) {
                if (g_toastCurrentX > target) g_toastCurrentX -= 1500.0f * delta;
                if (g_toastCurrentX < target) g_toastCurrentX = target;
                g_toastTimer -= delta;
            } else {
                if (g_toastCurrentX < io.DisplaySize.x + SP) g_toastCurrentX += 1500.0f * delta;
                else g_toastCurrentX = -10000.0f;
            }
            if (g_toastCurrentX != -10000.0f) {
                ImDrawList* dl = ImGui::GetForegroundDrawList();
                ImVec2 p_box(g_toastCurrentX + NIW, SP);
                dl->AddRectFilled(p_box, ImVec2(p_box.x + BW, SP + TTH), IM_COL32(0, 0, 0, 140), RO);
                if (g_pToastTexture) dl->AddImage((void*)g_pToastTexture, ImVec2(g_toastCurrentX, SP), ImVec2(g_toastCurrentX + NIW, SP + TTH));
                float cY = SP + (TTH - TH) * 0.5f;
                auto DrawLine = [&](const std::string& s, ImU32 col) {
                    if (!s.empty()) { dl->AddText(ImVec2(p_box.x + TX, cY), col, s.c_str()); cY += LH * 1.2f; }
                };
                DrawLine(g_currentBgmInfo.songName, IM_COL32_WHITE);
                if (g_ModConfig.showJapanese) DrawLine(g_currentBgmInfo.japaneseName, IM_COL32(200, 200, 200, 255));
                DrawLine(dt, IM_COL32(180, 180, 180, 255));
                DrawLine(g_currentBgmInfo.album, IM_COL32(180, 180, 180, 255));
            }
            if (fP) ImGui::PopFont();
        }
    }
    ImGui::EndFrame();
    ImGui::Render();
    IDirect3DStateBlock9* pSB = nullptr;
    if (SUCCEEDED(pDevice->CreateStateBlock(D3DSBT_ALL, &pSB)) && pSB) pSB->Capture();
    pDevice->SetRenderState(D3DRS_SRGBWRITEENABLE, FALSE);
    ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
    if (pSB) { pSB->Apply(); pSB->Release(); }
    return g_pfnOriginalEndScene(pDevice);
}

HRESULT WINAPI Detour_Reset(IDirect3DDevice9* pDevice, D3DPRESENT_PARAMETERS* pPresentationParameters) {
    if (g_pToastTexture) { g_pToastTexture->Release(); g_pToastTexture = nullptr; }
    ImGui_ImplDX9_InvalidateDeviceObjects();
    HRESULT hr = g_pfnOriginalReset(pDevice, pPresentationParameters);
    if (SUCCEEDED(hr)) ImGui_ImplDX9_CreateDeviceObjects();
    return hr;
}

// =============================================================
// BGM PROCESSING
// =============================================================
bool IsTarget(const std::string& fn) {
    if (fn.length() < 4) return false;
    std::string ex = fn.substr(fn.length() - 4);
    std::transform(ex.begin(), ex.end(), ex.begin(), ::tolower);
    return (ex == ".ogg" || ex == ".wav");
}

bool IsTargetW(const std::wstring& fn) {
    if (fn.length() < 4) return false;
    std::wstring ex = fn.substr(fn.length() - 4);
    std::transform(ex.begin(), ex.end(), ex.begin(), ::towlower);
    return (ex == L".ogg" || ex == L".wav");
}

HANDLE WINAPI Detour_CreateFileW(LPCWSTR f, DWORD a, DWORD s, LPSECURITY_ATTRIBUTES sec, DWORD d, DWORD fl, HANDLE t) {
    if (f && IsTargetW(f)) {
        if (g_bufferMutex.try_lock()) {
            if (!g_bNewBgmAvailable) { WCharToString(f, g_bgmFilenameBuffer, MAX_PATH); g_bNewBgmAvailable = true; }
            g_bufferMutex.unlock();
        }
    }
    return g_pfnOriginalCreateFileW(f, a, s, sec, d, fl, t);
}

HANDLE WINAPI Detour_CreateFileA(LPCSTR f, DWORD a, DWORD s, LPSECURITY_ATTRIBUTES sec, DWORD d, DWORD fl, HANDLE t) {
    if (f && IsTarget(f)) {
        if (g_bufferMutex.try_lock()) {
            if (!g_bNewBgmAvailable) { strcpy_s(g_bgmFilenameBuffer, MAX_PATH, f); g_bNewBgmAvailable = true; }
            g_bufferMutex.unlock();
        }
    }
    return g_pfnOriginalCreateFileA(f, a, s, sec, d, fl, t);
}

void ProcessBgmTrigger(const std::string& fn) {
    if (fn == g_lastTriggeredFile) return;
    g_lastTriggeredFile = fn;
    
    std::string input = fn;
    std::replace(input.begin(), input.end(), '/', '\\');
    for (auto& e : g_bgmMap) {
        std::string k = e.first;
        std::replace(k.begin(), k.end(), '/', '\\');
        if (input.length() >= k.length() && input.compare(input.length() - k.length(), k.length(), k) == 0) {
            g_currentBgmInfo = e.second;
            bool show = g_ModConfig.ignoreCooldown || (g_songLastShown.find(e.second.songName) == g_songLastShown.end()) || 
                        (std::chrono::duration_cast<std::chrono::hours>(std::chrono::steady_clock::now() - g_songLastShown[e.second.songName]).count() >= g_ModConfig.cooldownHours);
            if (show) {
                g_toastTimer = g_ModConfig.toastDuration;
                g_songLastShown[e.second.songName] = std::chrono::steady_clock::now();
                g_toastCurrentX = -10000.0f;
            }
            break;
        }
    }
}

void BgmWorkerThread() {
    while (g_bWorkerThreadActive) {
        if (g_bNewBgmAvailable) {
            std::string f;
            { std::lock_guard<std::mutex> l(g_bufferMutex); f = g_bgmFilenameBuffer; g_bNewBgmAvailable = false; }
            ProcessBgmTrigger(f);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

// =============================================================
// XANADU & YS7 HOOKS (RVA)
// =============================================================
static void* g_pfnOriginalPlayBgm = nullptr;
static void* g_pfnOriginalInternalFopen = nullptr;

std::string GetXanaduFilename(int id) {
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

void __stdcall ProcessXanaduLogic(int id_ecx, int id_stack) {
    int id = g_IsGog ? id_stack : id_ecx;
    std::string fn = GetXanaduFilename(id);
    if (!fn.empty()) {
        for (auto& e : g_bgmMap) {
            if (e.first.find(fn) != std::string::npos) {
                std::lock_guard<std::mutex> lock(g_BgmMutex);
                g_currentBgmInfo = e.second;
                g_toastTimer = g_ModConfig.toastDuration;
                g_toastCurrentX = -10000.0f;
                break;
            }
        }
    }
}

void __declspec(naked) Detour_Xanadu() {
    __asm {
        pushad
        pushfd
        mov eax, [esp + 40] // Arg 1 from original stack (id_stack)
        push eax
        push ecx // id_ecx
        call ProcessXanaduLogic
        popfd
        popad
        jmp [g_pfnOriginalPlayBgm]
    }
}

void __declspec(naked) Detour_Fopen() {
    __asm {
        pushad
        pushfd
        mov eax, [esp + 40] // Filename pointer from stack
        push eax
        call TryExtract
        test al, al
        jnz _done
        push ecx
        call TryExtract
    _done:
        popfd
        popad
        jmp [g_pfnOriginalInternalFopen]
    }
}

void LoadBgmMap() {
    g_bgmMap.clear();
    for (const auto& fn : g_Config.yamlFiles) {
        std::string p = g_modDirectory + "\\assets\\" + fn;
        std::ifstream f(p);
        if (!f.is_open()) continue;
        try {
            YAML::Node cfg = YAML::Load(f);
            for (const auto& n : cfg) {
                std::string path = n.first.as<std::string>();
                std::string val = n.second.as<std::string>();
                BgmInfo i; i.rawFileName = path;
                std::vector<std::string> pts; std::stringstream ss(val); std::string t;
                while (std::getline(ss, t, '|')) pts.push_back(t);
                if (pts.size() >= 1) i.songName = pts[0];
                if (pts.size() >= 2) i.japaneseName = pts[1];
                if (pts.size() >= 3) i.disc = pts[2];
                if (pts.size() >= 4) i.track = pts[3];
                if (pts.size() >= 5) i.album = pts[4];
                if (i.japaneseName == i.songName) i.japaneseName = "";
                g_bgmMap[path] = i;
            }
        } catch (...) {}
    }
}

HRESULT WINAPI Detour_CreateDevice(IDirect3D9* pD3D, UINT a, D3DDEVTYPE dt, HWND hw, DWORD b, D3DPRESENT_PARAMETERS* pp, IDirect3DDevice9** r) {
    HRESULT hr = g_pfnOriginalCreateDevice(pD3D, a, dt, hw, b, pp, r);
    if (SUCCEEDED(hr) && r && !g_bEndSceneHooked) {
        g_hWindow = hw ? hw : (pp ? pp->hDeviceWindow : NULL);
        void** vt = *(void***)*r;
        MH_CreateHook((LPVOID)vt[42], &My_EndScene, (LPVOID*)&g_pfnOriginalEndScene);
        MH_CreateHook((LPVOID)vt[16], &Detour_Reset, (LPVOID*)&g_pfnOriginalReset);
        MH_EnableHook(MH_ALL_HOOKS);
        g_bEndSceneHooked = true;
    }
    return hr;
}

IDirect3D9* WINAPI Detour_D3DCreate9(UINT v) {
    IDirect3D9* p = g_pfnOriginalDirect3DCreate9(v);
    LoadBgmMap();
    if (p) {
        void** vt = *(void***)p;
        MH_CreateHook((LPVOID)vt[16], &Detour_CreateDevice, (LPVOID*)&g_pfnOriginalCreateDevice);
        MH_EnableHook(MH_ALL_HOOKS);
    }
    uintptr_t target = 0;
    if (g_Config.useRvaHook) target = (uintptr_t)GetModuleHandle(NULL) + g_Config.bgmFuncRVA;
    if (target) {
        if (g_CurrentGame != GameID::XanaduNext) { g_bWorkerThreadActive = true; g_workerThread = std::thread(BgmWorkerThread); }
        void* detourFunc = (g_CurrentGame == GameID::XanaduNext) ? (void*)Detour_Xanadu : (void*)Detour_Fopen;
        void** originalFunc = (g_CurrentGame == GameID::XanaduNext) ? &g_pfnOriginalPlayBgm : &g_pfnOriginalInternalFopen;
        if (MH_CreateHook((LPVOID)target, detourFunc, originalFunc) == MH_OK) MH_EnableHook((LPVOID)target);
    }
    MH_RemoveHook((LPVOID)g_pfnOriginalDirect3DCreate9);
    return p;
}

void InitializeHooks() {
    MH_Initialize();
    HMODULE d3d = GetModuleHandleA("d3d9.dll");
    while (!d3d) { std::this_thread::sleep_for(std::chrono::milliseconds(100)); d3d = GetModuleHandleA("d3d9.dll"); }
    void* pC = (void*)GetProcAddress(d3d, "Direct3DCreate9");
    if (pC) { MH_CreateHook(pC, &Detour_D3DCreate9, (LPVOID*)&g_pfnOriginalDirect3DCreate9); MH_EnableHook(pC); }
    HMODULE hU32 = GetModuleHandleA("user32.dll");
    if (hU32) {
        MH_CreateHook(GetProcAddress(hU32, "SetCursorPos"), &Detour_SetCursorPos, (LPVOID*)&g_pfnOriginalSetCursorPos);
        MH_CreateHook(GetProcAddress(hU32, "ClipCursor"), &Detour_ClipCursor, (LPVOID*)&g_pfnOriginalClipCursor);
        MH_CreateHook(GetProcAddress(hU32, "GetAsyncKeyState"), &Detour_GetAsyncKeyState, (LPVOID*)&g_pfnOriginalGetAsyncKeyState);
        MH_CreateHook(GetProcAddress(hU32, "GetKeyState"), &Detour_GetKeyState, (LPVOID*)&g_pfnOriginalGetKeyState);
        MH_CreateHook(GetProcAddress(hU32, "GetKeyboardState"), &Detour_GetKeyboardState, (LPVOID*)&g_pfnOriginalGetKeyboardState);
    }
    
    auto HookX = [&](const char* dllName) {
        HMODULE h = GetModuleHandleA(dllName);
        if (h) {
            void* p = GetProcAddress(h, "XInputGetState");
            if (p) { if (MH_CreateHook(p, &Detour_XInputGetState, (LPVOID*)&g_pfnOriginalXInputGetState) == MH_OK) MH_EnableHook(p); }
        }
    };
    HookX("xinput1_4.dll"); HookX("xinput1_3.dll"); HookX("xinput9_1_0.dll");

    HMODULE hUcrt = GetModuleHandleA("ucrtbase.dll");
    if (hUcrt) {
        void* p = GetProcAddress(hUcrt, "fopen");
        if (p) { if (MH_CreateHook(p, &Detour_Fopen_UCRT, (LPVOID*)&g_pfnOriginalFopenUCRT) == MH_OK) MH_EnableHook(p); }
    }
    HMODULE hMsvcrt = GetModuleHandleA("msvcrt.dll");
    if (hMsvcrt) {
        void* p = GetProcAddress(hMsvcrt, "fopen");
        if (p) { if (MH_CreateHook(p, &Detour_Fopen_MSVCRT, (LPVOID*)&g_pfnOriginalFopenMSVCRT) == MH_OK) MH_EnableHook(p); }
    }
    MH_EnableHook(MH_ALL_HOOKS);
}

BOOL WINAPI DllMain(HMODULE h, DWORD r, LPVOID res) {
    if (r == DLL_PROCESS_ATTACH) {
        g_modDirectory = ResolveModDirectory();
        DisableThreadLibraryCalls(h);
        DetectProxyType(h);
        DetectAndConfigure();
        LoadConfig();
        std::thread(InitializeHooks).detach();
    } else if (r == DLL_PROCESS_DETACH) g_bWorkerThreadActive = false;
    return TRUE;
}
