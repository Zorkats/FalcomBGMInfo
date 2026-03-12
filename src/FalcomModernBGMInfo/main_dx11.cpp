#define NOMINMAX
#include <windows.h>
#include <shlwapi.h>
#pragma comment(lib, "shlwapi.lib")
#include <psapi.h>
#include "universal_proxy_x64.h"
#include <d3d11.h>
#pragma comment(lib, "d3d11.lib")
#include <DDSTextureLoader.h>
#include <DirectXTex.h>

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
#include <imgui_impl_dx11.h>

// =============================================================
// CONFIGURATION SYSTEM
// =============================================================
enum class GameID {
    Unknown,
    SkyRemake,
    CS1, CS2, CS3, CS4, Reverie,
    Ys8, Ys9, Celceta, Nayuta
};

struct GameConfig {
    std::string gameName;
    std::string windowTitlePart;
    std::vector<std::string> yamlFiles;
    bool useOpus;
    bool useWav;
    uintptr_t soundManagerRVA;
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

GameConfig g_Config;
ModConfig g_ModConfig;
GameID g_CurrentGame = GameID::Unknown;
static std::mutex g_LogMutex;

// =============================================================
// LOGGING & UTILS
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

void SaveConfig() {
    std::string path = GetModDirectory() + "\\mod_config.yaml";
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
        Log("Config saved to: " + path);
    } catch (const std::exception& e) {
        Log("Failed to save config: " + std::string(e.what()));
    }
}

void LoadConfig() {
    std::string path = GetModDirectory() + "\\mod_config.yaml";
    std::ifstream fin(path);
    if (!fin.is_open()) {
        Log("Config file not found, using defaults.");
        SaveConfig();
        return;
    }
    try {
        YAML::Node config = YAML::Load(fin);
        if (config["IgnoreCooldown"]) g_ModConfig.ignoreCooldown = config["IgnoreCooldown"].as<bool>();
        if (config["AlwaysShow"]) g_ModConfig.ignoreCooldown = config["AlwaysShow"].as<bool>(); // Legacy support
        if (config["ShowHotkey"]) g_ModConfig.showHotkey = config["ShowHotkey"].as<int>();
        if (config["MenuHotkey"]) g_ModConfig.menuHotkey = config["MenuHotkey"].as<int>();
        if (config["ToastDuration"]) g_ModConfig.toastDuration = config["ToastDuration"].as<float>();
        if (config["CooldownHours"]) g_ModConfig.cooldownHours = config["CooldownHours"].as<int>();
        if (config["ShowJapanese"]) g_ModConfig.showJapanese = config["ShowJapanese"].as<bool>();
        if (config["UIScale"]) g_ModConfig.uiScale = config["UIScale"].as<float>();
        Log("Config loaded from: " + path);
    } catch (const std::exception& e) {
        Log("Failed to load config: " + std::string(e.what()));
    }
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
// GAME DETECTION
// =============================================================
void DetectAndConfigure() {
    char exePath[MAX_PATH];
    GetModuleFileNameA(NULL, exePath, MAX_PATH);
    std::string exe = exePath;
    std::transform(exe.begin(), exe.end(), exe.begin(), ::tolower);

    g_Config.useOpus = false;
    g_Config.useWav = false;
    g_Config.soundManagerRVA = 0;

    if (exe.find("sora_1st.exe") != std::string::npos) g_CurrentGame = GameID::SkyRemake;
    else if (exe.find("ed8.exe") != std::string::npos) g_CurrentGame = GameID::CS1;
    else if (exe.find("ed8jp.exe") != std::string::npos) g_CurrentGame = GameID::CS1;
    else if (exe.find("ed8_2_pc_us.exe") != std::string::npos) g_CurrentGame = GameID::CS2;
    else if (exe.find("ed8_2_pc_jp.exe") != std::string::npos) g_CurrentGame = GameID::CS2;
    else if (exe.find("ed8_3_pc.exe") != std::string::npos) g_CurrentGame = GameID::CS3;
    else if (exe.find("ed8_3_pc_jp.exe") != std::string::npos) g_CurrentGame = GameID::CS3;
    else if (exe.find("ed8_4_pc.exe") != std::string::npos) g_CurrentGame = GameID::CS4;
    else if (exe.find("ed8_4_pc_jp.exe") != std::string::npos) g_CurrentGame = GameID::CS4;
    else if (exe.find("hnk.exe") != std::string::npos) g_CurrentGame = GameID::Reverie;
    else if (exe.find("ys8.exe") != std::string::npos) g_CurrentGame = GameID::Ys8;
    else if (exe.find("ys9.exe") != std::string::npos) g_CurrentGame = GameID::Ys9;
    else if (exe.find("ys9_jp.exe") != std::string::npos) g_CurrentGame = GameID::Ys9;
    else if (exe.find("ysc_dx11.exe") != std::string::npos) g_CurrentGame = GameID::Celceta;
    else if (exe.find("nys.exe") != std::string::npos) g_CurrentGame = GameID::Nayuta;

    switch (g_CurrentGame) {
        case GameID::CS1:
            g_Config.gameName = "Trails of Cold Steel I";
            g_Config.windowTitlePart = "Cold Steel";
            g_Config.yamlFiles.push_back("BgmMap_CS1.yaml");
            g_Config.useWav = true;
            break;
        case GameID::CS2:
            g_Config.gameName = "Trails of Cold Steel II";
            g_Config.windowTitlePart = "Cold Steel II";
            g_Config.yamlFiles.push_back("BgmMap_CS2.yaml");
            g_Config.useWav = true;
            break;
        case GameID::CS3:
            g_Config.gameName = "Trails of Cold Steel III";
            g_Config.windowTitlePart = "Cold Steel III";
            g_Config.yamlFiles.push_back("BgmMap_CS3.yaml");
            g_Config.useOpus = true;
            break;
        case GameID::CS4:
            g_Config.gameName = "Trails of Cold Steel IV";
            g_Config.windowTitlePart = "Cold Steel IV";
            g_Config.yamlFiles.push_back("BgmMap_Opus.yaml");
            g_Config.yamlFiles.push_back("BgmMap_Wav.yaml");
            g_Config.useOpus = true; g_Config.useWav = true;
            break;
        case GameID::Reverie:
            g_Config.gameName = "Trails into Reverie";
            g_Config.windowTitlePart = "Reverie";
            g_Config.yamlFiles.push_back("BgmMap_Reverie.yaml");
            g_Config.useOpus = true;
            break;
        case GameID::Ys8:
            g_Config.gameName = "Ys VIII";
            g_Config.windowTitlePart = "Lacrimosa of DANA";
            g_Config.yamlFiles.push_back("BgmMap_Ys8.yaml");
            g_Config.useOpus = true;
            break;
        case GameID::Ys9:
            g_Config.gameName = "Ys IX";
            g_Config.windowTitlePart = "Monstrum Nox";
            g_Config.yamlFiles.push_back("BgmMap_Ys9.yaml");
            g_Config.useOpus = true;
            break;
        case GameID::Celceta:
            g_Config.gameName = "Ys: Memories of Celceta";
            g_Config.windowTitlePart = "Celceta";
            g_Config.yamlFiles.push_back("BgmMap_Celceta.yaml");
            g_Config.useWav = true; g_Config.useOpus = true;
            break;
        case GameID::Nayuta:
            g_Config.gameName = "Nayuta no Kiseki";
            g_Config.windowTitlePart = "Boundless Trails";
            g_Config.yamlFiles.push_back("BgmMap_Nayuta.yaml");
            g_Config.useOpus = true;
            break;
        case GameID::SkyRemake:
            g_Config.gameName = "Trails in the Sky (Remake)";
            g_Config.windowTitlePart = "Trails in the Sky";
            g_Config.yamlFiles.push_back("BgmMap_Sky.yaml");
            g_Config.soundManagerRVA = 0x57B440;
            break;
        default:
            g_Config.gameName = "Unknown Game";
            g_Config.windowTitlePart = "";
            g_Config.yamlFiles.push_back("BgmMap.yaml");
            g_Config.useOpus = true; g_Config.useWav = true;
            break;
    }
    Log("DETECTED GAME: " + g_Config.gameName);
}

// =============================================================
// GLOBALS
// =============================================================
struct BgmInfo {
    std::string songName;
    std::string japaneseName;
    std::string version;
    std::string disc;
    std::string track;
    std::string album;
    std::string rawFileName;
};

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
static HWND g_hWindow = nullptr;
static ID3D11Device* g_pd3dDevice = nullptr;
static ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
static ID3D11RenderTargetView* g_pd3dRenderTargetView = nullptr;
static ImFont* g_pMenuFont = nullptr;
static ImFont* g_pToastFont = nullptr;
static ID3D11ShaderResourceView* g_pToastTexture = nullptr;
static ID3D11Resource* g_pToastResource = nullptr;
static float g_TextureWidth = 0.0f;
static float g_TextureHeight = 0.0f;
static std::thread g_workerThread;
static std::mutex g_bufferMutex;
static char g_bgmFilenameBuffer[MAX_PATH];
static std::atomic<bool> g_bNewBgmAvailable = false;
static std::atomic<bool> g_bWorkerThreadActive = true;

// =============================================================
// INPUT BLOCKING DETOURS
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
    if (g_showMenu && !g_isRemapping && lpKeyState) {
        memset(lpKeyState, 0, 256);
        return TRUE;
    }
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

// Aggressive Message Blocking
typedef BOOL(WINAPI* PFN_PEEKMESSAGEA)(LPMSG, HWND, UINT, UINT, UINT);
typedef BOOL(WINAPI* PFN_PEEKMESSAGEW)(LPMSG, HWND, UINT, UINT, UINT);
typedef BOOL(WINAPI* PFN_GETMESSAGEA)(LPMSG, HWND, UINT, UINT);
typedef BOOL(WINAPI* PFN_GETMESSAGEW)(LPMSG, HWND, UINT, UINT);
static PFN_PEEKMESSAGEA g_pfnOriginalPeekMessageA = nullptr;
static PFN_PEEKMESSAGEW g_pfnOriginalPeekMessageW = nullptr;
static PFN_GETMESSAGEA g_pfnOriginalGetMessageA = nullptr;
static PFN_GETMESSAGEW g_pfnOriginalGetMessageW = nullptr;

BOOL WINAPI Detour_PeekMessageA(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax, UINT wRemoveMsg) {
    BOOL res = g_pfnOriginalPeekMessageA(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax, wRemoveMsg);
    if (res && g_showMenu && !g_isRemapping && lpMsg) {
        if (lpMsg->message == WM_KEYDOWN || lpMsg->message == WM_SYSKEYDOWN) {
            if (lpMsg->wParam == (WPARAM)g_ModConfig.menuHotkey) return res;
        }
        if ((lpMsg->message >= WM_MOUSEFIRST && lpMsg->message <= WM_MOUSELAST) || (lpMsg->message >= WM_KEYFIRST && lpMsg->message <= WM_KEYLAST) || lpMsg->message == WM_INPUT || lpMsg->message == WM_CHAR) {
            lpMsg->message = WM_NULL;
        }
    }
    return res;
}
BOOL WINAPI Detour_PeekMessageW(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax, UINT wRemoveMsg) {
    BOOL res = g_pfnOriginalPeekMessageW(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax, wRemoveMsg);
    if (res && g_showMenu && !g_isRemapping && lpMsg) {
        if (lpMsg->message == WM_KEYDOWN || lpMsg->message == WM_SYSKEYDOWN) {
            if (lpMsg->wParam == (WPARAM)g_ModConfig.menuHotkey) return res;
        }
        if ((lpMsg->message >= WM_MOUSEFIRST && lpMsg->message <= WM_MOUSELAST) || (lpMsg->message >= WM_KEYFIRST && lpMsg->message <= WM_KEYLAST) || lpMsg->message == WM_INPUT || lpMsg->message == WM_CHAR) {
            lpMsg->message = WM_NULL;
        }
    }
    return res;
}
BOOL WINAPI Detour_GetMessageA(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax) {
    BOOL res = g_pfnOriginalGetMessageA(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax);
    if (res && g_showMenu && !g_isRemapping && lpMsg) {
        if (lpMsg->message == WM_KEYDOWN || lpMsg->message == WM_SYSKEYDOWN) {
            if (lpMsg->wParam == (WPARAM)g_ModConfig.menuHotkey) return res;
        }
        if ((lpMsg->message >= WM_MOUSEFIRST && lpMsg->message <= WM_MOUSELAST) || (lpMsg->message >= WM_KEYFIRST && lpMsg->message <= WM_KEYLAST) || lpMsg->message == WM_INPUT || lpMsg->message == WM_CHAR) {
            lpMsg->message = WM_NULL;
        }
    }
    return res;
}
BOOL WINAPI Detour_GetMessageW(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax) {
    BOOL res = g_pfnOriginalGetMessageW(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax);
    if (res && g_showMenu && !g_isRemapping && lpMsg) {
        if (lpMsg->message == WM_KEYDOWN || lpMsg->message == WM_SYSKEYDOWN) {
            if (lpMsg->wParam == (WPARAM)g_ModConfig.menuHotkey) return res;
        }
        if ((lpMsg->message >= WM_MOUSEFIRST && lpMsg->message <= WM_MOUSELAST) || (lpMsg->message >= WM_KEYFIRST && lpMsg->message <= WM_KEYLAST) || lpMsg->message == WM_INPUT || lpMsg->message == WM_CHAR) {
            lpMsg->message = WM_NULL;
        }
    }
    return res;
}

// =============================================================
// D3D11 STATE SAVER
// =============================================================
struct D3D11StateSaver {
    bool m_saved;
    D3D_FEATURE_LEVEL m_featureLevel;
    ID3D11DeviceContext* m_pContext;
    D3D11_PRIMITIVE_TOPOLOGY m_primitiveTopology;
    ID3D11InputLayout* m_pInputLayout;
    ID3D11BlendState* m_pBlendState;
    float m_blendFactor[4];
    UINT m_sampleMask;
    ID3D11DepthStencilState* m_pDepthStencilState;
    UINT m_stencilRef;
    ID3D11RasterizerState* m_pRasterizerState;
    ID3D11ShaderResourceView* m_pPSShaderResource;
    ID3D11SamplerState* m_pPSSampler;
    ID3D11PixelShader* m_pPS;
    ID3D11VertexShader* m_pVS;
    ID3D11GeometryShader* m_pGS;
    UINT m_numViewports;
    D3D11_VIEWPORT m_viewports[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE];
    ID3D11RenderTargetView* m_pRenderTargets[D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT];
    ID3D11DepthStencilView* m_pDepthStencilView;

    explicit D3D11StateSaver(ID3D11DeviceContext* pContext)
        : m_saved(false)
        , m_featureLevel(D3D_FEATURE_LEVEL_11_0)
        , m_pContext(pContext)
        , m_primitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED)
        , m_pInputLayout(nullptr)
        , m_pBlendState(nullptr)
        , m_sampleMask(0)
        , m_pDepthStencilState(nullptr)
        , m_stencilRef(0)
        , m_pRasterizerState(nullptr)
        , m_pPSShaderResource(nullptr)
        , m_pPSSampler(nullptr)
        , m_pPS(nullptr)
        , m_pVS(nullptr)
        , m_pGS(nullptr)
        , m_numViewports(0)
        , m_pDepthStencilView(nullptr)
    {
        memset(m_blendFactor, 0, sizeof(m_blendFactor));
        memset(m_viewports, 0, sizeof(m_viewports));
        for (int i = 0; i < D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
            m_pRenderTargets[i] = nullptr;

        if (m_pContext) saveCurrentState();
    }

    ~D3D11StateSaver() {
        restoreSavedState();
        releaseSavedState();
    }

    void saveCurrentState() {
        if (!m_pContext) return;
        m_pContext->IAGetPrimitiveTopology(&m_primitiveTopology);
        m_pContext->IAGetInputLayout(&m_pInputLayout);
        m_pContext->OMGetBlendState(&m_pBlendState, m_blendFactor, &m_sampleMask);
        m_pContext->OMGetDepthStencilState(&m_pDepthStencilState, &m_stencilRef);
        m_pContext->RSGetState(&m_pRasterizerState);
        m_numViewports = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
        m_pContext->RSGetViewports(&m_numViewports, m_viewports);
        m_pContext->PSGetShaderResources(0, 1, &m_pPSShaderResource);
        m_pContext->PSGetSamplers(0, 1, &m_pPSSampler);
        m_pContext->PSGetShader(&m_pPS, NULL, NULL);
        m_pContext->VSGetShader(&m_pVS, NULL, NULL);
        m_pContext->GSGetShader(&m_pGS, NULL, NULL);
        m_pContext->OMGetRenderTargets(D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT, m_pRenderTargets, &m_pDepthStencilView);
        m_saved = true;
    }

    void restoreSavedState() {
        if (!m_saved || !m_pContext) return;
        m_pContext->IASetPrimitiveTopology(m_primitiveTopology);
        m_pContext->IASetInputLayout(m_pInputLayout);
        m_pContext->OMSetBlendState(m_pBlendState, m_blendFactor, m_sampleMask);
        m_pContext->OMSetDepthStencilState(m_pDepthStencilState, m_stencilRef);
        m_pContext->RSSetState(m_pRasterizerState);
        m_pContext->RSSetViewports(m_numViewports, m_viewports);
        m_pContext->PSSetShaderResources(0, 1, &m_pPSShaderResource);
        m_pContext->PSSetSamplers(0, 1, &m_pPSSampler);
        m_pContext->PSSetShader(m_pPS, NULL, NULL);
        m_pContext->VSSetShader(m_pVS, NULL, NULL);
        m_pContext->GSSetShader(m_pGS, NULL, NULL);
        m_pContext->OMSetRenderTargets(D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT, m_pRenderTargets, m_pDepthStencilView);
    }

    void releaseSavedState() {
        if (m_pInputLayout) m_pInputLayout->Release();
        if (m_pBlendState) m_pBlendState->Release();
        if (m_pDepthStencilState) m_pDepthStencilState->Release();
        if (m_pRasterizerState) m_pRasterizerState->Release();
        if (m_pPSShaderResource) m_pPSShaderResource->Release();
        if (m_pPSSampler) m_pPSSampler->Release();
        if (m_pPS) m_pPS->Release();
        if (m_pVS) m_pVS->Release();
        if (m_pGS) m_pGS->Release();
        if (m_pDepthStencilView) m_pDepthStencilView->Release();
        for (int i = 0; i < D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
            if (m_pRenderTargets[i]) m_pRenderTargets[i]->Release();
    }
};

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
    if (!file.is_open()) {
        Log("LoadMapFile: " + filename + " not found.");
        return;
    }

    try {
        YAML::Node config = YAML::Load(file);
        for (const auto& node : config) {
            std::string filepath = node.first.as<std::string>();
            std::string value = node.second.as<std::string>();
            BgmInfo info;
            info.rawFileName = filepath;
            
            std::vector<std::string> parts = SplitString(value, '|');

            if (g_CurrentGame == GameID::SkyRemake) {
                if (parts.size() >= 1) info.songName = parts[0];
                if (parts.size() >= 2) info.japaneseName = parts[1];
                if (parts.size() >= 3) info.version = parts[2];
                if (parts.size() >= 4) info.disc = parts[3];
                if (parts.size() >= 5) info.track = parts[4];
            } else {
                if (parts.size() >= 1) info.songName = parts[0];
                if (parts.size() >= 2) info.japaneseName = parts[1];
                if (parts.size() >= 3) info.disc = parts[2];
                if (parts.size() >= 4) info.track = parts[3];
                if (parts.size() >= 5) info.album = parts[4];
            }

            if (info.japaneseName == info.songName || info.japaneseName == " ") info.japaneseName = "";
            g_bgmMap[filepath] = info;
        }
        Log("Loaded: " + filename);
    } catch (const YAML::Exception& e) {
        Log("YAML Error: " + std::string(e.what()));
    }
}

void LoadBgmMap() {
    g_bgmMap.clear();
    for (const auto& file : g_Config.yamlFiles) LoadMapFile(file);
    Log("Total entries: " + std::to_string(g_bgmMap.size()));
}

// =============================================================
// GRAPHICS HOOKS (DirectX 11 & ImGui)
// =============================================================
typedef HRESULT(WINAPI* PFN_PRESENT)(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);
static PFN_PRESENT g_pfnOriginalPresent = nullptr;
static WNDPROC g_pfnOriginalWndProc = NULL;
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT WINAPI WndProc(const HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (uMsg == WM_KEYDOWN) {
        if (wParam == g_ModConfig.menuHotkey) {
            g_showMenu = !g_showMenu;
            ImGuiIO& io = ImGui::GetIO();
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
            // Block mouse and keyboard inputs from reaching the game while menu is open
            if ((uMsg >= WM_MOUSEFIRST && uMsg <= WM_MOUSELAST) ||
                (uMsg >= WM_KEYFIRST && uMsg <= WM_KEYLAST) ||
                uMsg == WM_INPUT) {
                return 1;
            }
        }
    } else {
        ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam);
    }

    return CallWindowProc(g_pfnOriginalWndProc, hWnd, uMsg, wParam, lParam);
}

void InitImGui(IDXGISwapChain* pSwapChain) {
    if (g_imguiInitialized) return;

    Log("InitImGui starting...");

    if (!g_hWindow) {
        DXGI_SWAP_CHAIN_DESC desc;
        if (SUCCEEDED(pSwapChain->GetDesc(&desc))) {
            g_hWindow = desc.OutputWindow;
        }
    }

    if (!g_hWindow) {
        Log("ERROR: No valid window handle, skipping initialization");
        return;
    }

    if (SUCCEEDED(pSwapChain->GetDevice(__uuidof(ID3D11Device), (void**)&g_pd3dDevice))) {
        g_pd3dDevice->GetImmediateContext(&g_pd3dDeviceContext);
        g_pfnOriginalWndProc = (WNDPROC)SetWindowLongPtr(g_hWindow, GWLP_WNDPROC, (LONG_PTR)WndProc);

        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.IniFilename = NULL;

        io.ConfigFlags |= ImGuiConfigFlags_NoMouse;
        io.ConfigFlags |= ImGuiConfigFlags_NoKeyboard;

        std::string f1 = GetModDirectory() + "\\assets\\mod_font.otf";
        std::string f2 = GetModDirectory() + "\\assets\\mod_font_japanese.ttf";
        const ImWchar* ranges = io.Fonts->GetGlyphRangesJapanese();

        g_pMenuFont = io.Fonts->AddFontFromFileTTF(f1.c_str(), 18.0f);
        if (g_pMenuFont) {
            ImFontConfig cfg1;
            cfg1.MergeMode = true;
            io.Fonts->AddFontFromFileTTF(f2.c_str(), 18.0f, &cfg1, ranges);
        }

        g_pToastFont = io.Fonts->AddFontFromFileTTF(f1.c_str(), 28.0f);
        if (g_pToastFont) {
            ImFontConfig cfg2;
            cfg2.MergeMode = true;
            io.Fonts->AddFontFromFileTTF(f2.c_str(), 28.0f, &cfg2, ranges);
        }

        std::string texStr = GetModDirectory() + "\\assets\\bgm_info.dds";
        std::wstring texW(texStr.begin(), texStr.end());
        HRESULT hr = DirectX::CreateDDSTextureFromFile(g_pd3dDevice, texW.c_str(), &g_pToastResource, &g_pToastTexture);
        if (SUCCEEDED(hr) && g_pToastResource) {
            ID3D11Texture2D* pTex = nullptr;
            if (SUCCEEDED(g_pToastResource->QueryInterface(__uuidof(ID3D11Texture2D), (void**)&pTex))) {
                D3D11_TEXTURE2D_DESC desc;
                pTex->GetDesc(&desc);
                g_TextureWidth = (float)desc.Width;
                g_TextureHeight = (float)desc.Height;
                pTex->Release();
            }
        }

        ImGui_ImplWin32_Init(g_hWindow);
        ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);
        g_imguiInitialized = true;
        Log("InitImGui finished successfully.");
    }
}

void DrawRemapper(const char* label, int& key) {
    std::string kn = GetKeyName(key);
    char buf[128];
    sprintf_s(buf, "%s###%s_btn", kn.c_str(), label);

    if (ImGui::Button(buf, ImVec2(140, 0))) ImGui::OpenPopup(label);
    ImGui::SameLine();
    ImGui::Text("%s", label);

    if (ImGui::BeginPopup(label)) {
        g_isRemapping = true;
        ImGui::Text("Press any key...");
        for (int i = 1; i < 256; i++) {
            if (g_pfnOriginalGetAsyncKeyState(i) & 0x8000) {
                if (i != VK_LBUTTON && i != VK_RBUTTON) {
                    key = i;
                    ImGui::CloseCurrentPopup();
                    g_isRemapping = false;
                    break;
                }
            }
        }
        if (ImGui::Button("Cancel")) {
            ImGui::CloseCurrentPopup();
            g_isRemapping = false;
        }
        ImGui::EndPopup();
    }
}

HRESULT WINAPI My_Present(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags) {
    static int frameCount = 0;
    frameCount++;
    if (frameCount <= 3) {
        Log("Present called, frame " + std::to_string(frameCount));
    }

    D3D11StateSaver stateSaver(g_pd3dDeviceContext);
    InitImGui(pSwapChain);

    if (!g_imguiInitialized) {
        return g_pfnOriginalPresent(pSwapChain, SyncInterval, Flags);
    }

    ID3D11Texture2D* pBackBuffer = nullptr;
    float actual_width = 0.0f;
    float actual_height = 0.0f;

    if (SUCCEEDED(pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer))) {
        D3D11_TEXTURE2D_DESC desc;
        pBackBuffer->GetDesc(&desc);
        actual_width = (float)desc.Width;
        actual_height = (float)desc.Height;
        pBackBuffer->Release();
    }

    ImGui_ImplWin32_NewFrame();

    ImGuiIO& io = ImGui::GetIO();
    if (actual_width > 0.0f && actual_height > 0.0f) {
        io.DisplaySize.x = actual_width;
        io.DisplaySize.y = actual_height;
    }

    if (actual_width != g_lastDisplayWidth || actual_height != g_lastDisplayHeight) {
        g_lastDisplayWidth = actual_width;
        g_lastDisplayHeight = actual_height;
        if (g_toastCurrentX != -10000.0f && g_toastTimer > 0.0f) {
            g_toastCurrentX = -10000.0f;
        }
    }

    ImGui_ImplDX11_NewFrame();
    ImGui::NewFrame();
    
    g_isRemapping = false; // Reset remapping flag every frame

    if (g_showMenu) {
        ImGui::SetNextWindowSize(ImVec2(400, 300), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Falcom BGM Info Settings", &g_showMenu)) {
            ImGui::Checkbox("Ignore Cooldown (Always Show)", &g_ModConfig.ignoreCooldown);
            ImGui::Checkbox("Show Japanese Name", &g_ModConfig.showJapanese);
            ImGui::SliderFloat("Toast Duration", &g_ModConfig.toastDuration, 1.0f, 20.0f, "%.1f seconds");
            ImGui::SliderFloat("UI Scale", &g_ModConfig.uiScale, 0.3f, 2.0f, "%.2f");
            
            int h = g_ModConfig.cooldownHours;
            if (ImGui::SliderInt("Cooldown (Hours)", &h, 0, 24)) {
                g_ModConfig.cooldownHours = h;
            }
            
            ImGui::Separator();
            ImGui::Text("Hotkeys:");
            DrawRemapper("Menu Hotkey", g_ModConfig.menuHotkey);
            DrawRemapper("Show Info Hotkey", g_ModConfig.showHotkey);
            
            if (ImGui::Button("Reset Cooldowns")) { std::lock_guard<std::mutex> lock(g_BgmMutex); g_songLastShown.clear(); }
            if (ImGui::Button("Save Configuration")) SaveConfig();
        }
        ImGui::End();
        
        if (!g_showMenu) {
            io.ConfigFlags |= ImGuiConfigFlags_NoMouse;
            io.ConfigFlags |= ImGuiConfigFlags_NoKeyboard;
            io.MouseDrawCursor = false;
            SaveConfig();
        }
    }

    const float UI_SCALE = g_ModConfig.uiScale;
    const float SCREEN_PADDING = 10.0f;
    const float TEXT_PADDING_X = 20.0f * UI_SCALE;
    const float TEXT_PADDING_Y = 15.0f * UI_SCALE;

    {
        std::lock_guard<std::mutex> lock(g_BgmMutex);
        std::string line1 = g_currentBgmInfo.songName;
        std::string line2 = "";
        if (g_ModConfig.showJapanese && !g_currentBgmInfo.japaneseName.empty()) {
            line2 = "(" + g_currentBgmInfo.japaneseName + ")";
        }
        std::string line3 = g_currentBgmInfo.version;
        std::string line4 = "";
        if (!g_currentBgmInfo.disc.empty() || !g_currentBgmInfo.track.empty()) {
            line4 = "Disc " + g_currentBgmInfo.disc + ", Track " + g_currentBgmInfo.track;
        }
        std::string line5 = g_currentBgmInfo.album;

        if ((g_toastTimer > 0.0f || g_toastCurrentX != -10000.0f) && !line1.empty()) {
            bool fontPushed = false;
            if (g_pToastFont && g_pToastFont->IsLoaded()) {
                ImGui::PushFont(g_pToastFont);
                fontPushed = true;
            }

            ImVec2 s1 = ImGui::CalcTextSize(line1.c_str());
            ImVec2 s2 = ImGui::CalcTextSize(line2.c_str());
            ImVec2 s3 = ImGui::CalcTextSize(line3.c_str());
            ImVec2 s4 = ImGui::CalcTextSize(line4.c_str());
            ImVec2 s5 = ImGui::CalcTextSize(line5.c_str());

            float text_width = std::max({s1.x, s2.x, s3.x, s4.x, s5.x});
            float line_height = s1.y > 0 ? s1.y : 28.0f;

            if (text_width < 50.0f) {
                size_t maxLen = std::max({line1.length(), line2.length(), line3.length(), line4.length(), line5.length()});
                text_width = std::max(text_width, (float)maxLen * 8.0f);
            }

            int line_count = 0;
            if (!line1.empty()) line_count++;
            if (!line2.empty()) line_count++;
            if (!line3.empty()) line_count++;
            if (!line4.empty()) line_count++;
            if (!line5.empty()) line_count++;

            float text_height = line_height * (float)line_count + (line_height * 0.2f * (line_count - 1));
            float total_height = text_height + (TEXT_PADDING_Y * 2.0f);
            float note_icon_width = total_height;
            float box_width = text_width + (TEXT_PADDING_X * 2.0f);
            float total_width = note_icon_width + box_width;

            float screen_width = io.DisplaySize.x;
            float top_y = SCREEN_PADDING;

            float maxWidth = screen_width * 0.8f;
            float effectiveWidth = std::min(total_width, maxWidth);

            float target_onscreen_x = screen_width - effectiveWidth - SCREEN_PADDING;
            if (target_onscreen_x < SCREEN_PADDING) target_onscreen_x = SCREEN_PADDING;
            float target_offscreen_x = screen_width + SCREEN_PADDING;

            if (g_toastCurrentX == -10000.0f) {
                g_toastCurrentX = target_offscreen_x;
            }

            float deltaTime = io.DeltaTime;
            float animSpeed = 1500.0f;

            if (g_toastTimer > 0.0f) {
                if (g_toastCurrentX > target_onscreen_x) {
                    g_toastCurrentX -= animSpeed * deltaTime;
                    if (g_toastCurrentX < target_onscreen_x) g_toastCurrentX = target_onscreen_x;
                }
                g_toastTimer -= deltaTime;
            } else {
                if (g_toastCurrentX < target_offscreen_x) {
                    g_toastCurrentX += animSpeed * deltaTime;
                } else {
                    g_toastCurrentX = -10000.0f;
                }
            }

            if (g_toastCurrentX != -10000.0f) {
                ImDrawList* dl = ImGui::GetForegroundDrawList();
                float y = top_y;

                dl->AddRectFilled(
                    ImVec2(g_toastCurrentX + note_icon_width, y),
                    ImVec2(g_toastCurrentX + total_width, y + total_height),
                    IM_COL32(0, 0, 0, 180), 8.0f
                );

                if (g_pToastTexture) {
                    dl->AddImage(
                        (void*)g_pToastTexture,
                        ImVec2(g_toastCurrentX, y),
                        ImVec2(g_toastCurrentX + note_icon_width, y + total_height)
                    );
                }

                float ty = y + TEXT_PADDING_Y;
                auto DrawLine = [&](const std::string& s, ImU32 color) {
                    if (!s.empty()) {
                        dl->AddText(ImVec2(g_toastCurrentX + note_icon_width + TEXT_PADDING_X, ty), color, s.c_str());
                        ty += line_height * 1.2f;
                    }
                };

                DrawLine(line1, IM_COL32_WHITE);
                DrawLine(line2, IM_COL32(200, 200, 200, 255));
                DrawLine(line3, IM_COL32(180, 180, 180, 255));
                DrawLine(line4, IM_COL32(180, 180, 180, 255));
                DrawLine(line5, IM_COL32(180, 180, 180, 255));
            }

            if (fontPushed) ImGui::PopFont();
        }
    }

    ImGui::Render();

    ID3D11RenderTargetView* pRTV = nullptr;
    ID3D11Texture2D* pBackBufferTex = nullptr;
    if (SUCCEEDED(pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBufferTex))) {
        g_pd3dDevice->CreateRenderTargetView(pBackBufferTex, NULL, &pRTV);
        pBackBufferTex->Release();
    }

    if (pRTV) {
        g_pd3dDeviceContext->OMSetRenderTargets(1, &pRTV, NULL);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        pRTV->Release();
    }

    return g_pfnOriginalPresent(pSwapChain, SyncInterval, Flags);
}

// =============================================================
// FILE SYSTEM HOOKS
// =============================================================
bool IsTargetFile(const std::string& fname) {
    if (fname.length() < 5) return false;
    std::string ext4 = fname.substr(fname.length() - 4);
    std::string ext5 = fname.length() >= 5 ? fname.substr(fname.length() - 5) : "";
    std::transform(ext4.begin(), ext4.end(), ext4.begin(), ::tolower);
    std::transform(ext5.begin(), ext5.end(), ext5.begin(), ::tolower);

    if (g_Config.useWav && ext4 == ".wav") return true;
    if (g_Config.useOpus && (ext5 == ".opus" || ext4 == ".ogg")) return true;
    return false;
}

bool IsTargetFileW(const std::wstring& fname) {
    if (fname.length() < 5) return false;
    std::wstring ext4 = fname.substr(fname.length() - 4);
    std::wstring ext5 = fname.length() >= 5 ? fname.substr(fname.length() - 5) : L"";
    std::transform(ext4.begin(), ext4.end(), ext4.begin(), ::towlower);
    std::transform(ext5.begin(), ext5.end(), ext5.begin(), ::towlower);

    if (g_Config.useWav && ext4 == L".wav") return true;
    if (g_Config.useOpus && (ext5 == L".opus" || ext4 == L".ogg")) return true;
    return false;
}

typedef HANDLE(WINAPI* PFN_CREATEFILEW)(LPCWSTR, DWORD, DWORD, LPSECURITY_ATTRIBUTES, DWORD, DWORD, HANDLE);
static PFN_CREATEFILEW g_pfnOriginalCreateFileW = nullptr;
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

typedef HANDLE(WINAPI* PFN_CREATEFILEA)(LPCSTR, DWORD, DWORD, LPSECURITY_ATTRIBUTES, DWORD, DWORD, HANDLE);
static PFN_CREATEFILEA g_pfnOriginalCreateFileA = nullptr;
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

typedef void(__fastcall* PFN_SOUNDMANAGER)(uintptr_t rcx, const char* rdx_filename, uintptr_t r8, uintptr_t r9);
static PFN_SOUNDMANAGER g_pfnOriginalSoundManager = nullptr;
void __fastcall My_SoundManager(uintptr_t rcx, const char* rdx_filename, uintptr_t r8, uintptr_t r9) {
    if (rdx_filename) {
        if (g_bufferMutex.try_lock()) {
            if (!g_bNewBgmAvailable) {
                strcpy_s(g_bgmFilenameBuffer, MAX_PATH, rdx_filename);
                g_bNewBgmAvailable = true;
            }
            g_bufferMutex.unlock();
        }
    }
    g_pfnOriginalSoundManager(rcx, rdx_filename, r8, r9);
}

// =============================================================
// BGM PROCESSING
// =============================================================
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

            std::lock_guard<std::mutex> lock(g_BgmMutex);
            g_currentBgmInfo = entry.second;
            g_currentBgmInfo.rawFileName = entry.first;

            std::string songKey = g_currentBgmInfo.songName;
            bool shouldShow = false;

            if (g_ModConfig.ignoreCooldown) {
                shouldShow = true;
            } else {
                auto it = g_songLastShown.find(songKey);
                if (it == g_songLastShown.end()) {
                    shouldShow = true;
                } else {
                    auto now = std::chrono::steady_clock::now();
                    auto hours = std::chrono::duration_cast<std::chrono::hours>(now - it->second).count();
                    if (hours >= g_ModConfig.cooldownHours) shouldShow = true;
                }
            }

            if (shouldShow) {
                g_toastTimer = g_ModConfig.toastDuration;
                g_songLastShown[songKey] = std::chrono::steady_clock::now();
                g_toastCurrentX = -10000.0f;
            }
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
// HOOK INITIALIZATION
// =============================================================
uintptr_t FindPresentAddress(HWND hWnd) {
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    ID3D11Device* pDevice = nullptr;
    IDXGISwapChain* pSwapChain = nullptr;

    HRESULT hr = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, NULL, 0, D3D11_SDK_VERSION, &sd, &pSwapChain, &pDevice, NULL, NULL);

    uintptr_t pPresentAddr = 0;
    if (SUCCEEDED(hr)) {
        void** pVTable = *(void***)pSwapChain;
        pPresentAddr = (uintptr_t)pVTable[8];
        pSwapChain->Release();
        pDevice->Release();
    } else {
        std::stringstream ss;
        ss << "D3D11CreateDeviceAndSwapChain failed! HRESULT: 0x" << std::hex << hr;
        Log(ss.str());
    }

    return pPresentAddr;
}

void InitializeHooks() {
    // Guard against multiple initializations
    static bool s_bInitialized = false;
    if (s_bInitialized) {
        Log("InitializeHooks already called, skipping duplicate initialization.");
        return;
    }
    s_bInitialized = true;

    Log("Hook thread started.");

    DetectAndConfigure();
    LoadConfig();

    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(hr)) {
        Log("CoInitializeEx failed! HR: " + std::to_string(hr));
    } else {
        Log("CoInitializeEx successful.");
    }

    // Don't block for window here - we'll get it from the swap chain in InitImGui
    // Create a temporary window for vtable discovery
    HWND hTempWnd = CreateWindowExA(0, "STATIC", "TempD3D11Window", 0, 0, 0, 1, 1, NULL, NULL, NULL, NULL);
    if (!hTempWnd) {
        Log("Failed to create temporary window for hook discovery");
        return;
    }

    MH_STATUS mhStatus = MH_Initialize();
    if (mhStatus != MH_OK && mhStatus != MH_ERROR_ALREADY_INITIALIZED) {
        Log("MH_Initialize failed with status: " + std::to_string(mhStatus));
        DestroyWindow(hTempWnd);
        return;
    }
    Log("MH_Initialize successful.");

    LoadBgmMap();

    g_bWorkerThreadActive = true;
    g_workerThread = std::thread(BgmWorkerThread);

    if (g_Config.soundManagerRVA != 0) {
        uintptr_t base = (uintptr_t)GetModuleHandle(NULL);
        LPVOID pSoundManagerAddr = (LPVOID)(base + g_Config.soundManagerRVA);
        if (MH_CreateHook(pSoundManagerAddr, &My_SoundManager, (LPVOID*)&g_pfnOriginalSoundManager) != MH_OK) {
            Log("MH_CreateHook for SoundManager failed!");
        } else {
            Log("MH_CreateHook for SoundManager successful.");
        }
    } else {
        HMODULE hKernel32 = GetModuleHandleA("kernel32.dll");
        if (hKernel32) {
            void* pCreateFileW = (void*)GetProcAddress(hKernel32, "CreateFileW");
            if (pCreateFileW) {
                MH_CreateHook(pCreateFileW, &Detour_CreateFileW, (LPVOID*)&g_pfnOriginalCreateFileW);
                Log("Hooked CreateFileW");
            }

            void* pCreateFileA = (void*)GetProcAddress(hKernel32, "CreateFileA");
            if (pCreateFileA) {
                if (MH_CreateHook(pCreateFileA, &Detour_CreateFileA, (LPVOID*)&g_pfnOriginalCreateFileA) != MH_OK) {
                    Log("Failed to hook CreateFileA!");
                } else {
                    Log("Hooked CreateFileA successfully.");
                }
            }
        }
    }

    HMODULE hUser32 = GetModuleHandleA("user32.dll");
    if (hUser32) {
        MH_CreateHook(GetProcAddress(hUser32, "SetCursorPos"), &Detour_SetCursorPos, (LPVOID*)&g_pfnOriginalSetCursorPos);
        MH_CreateHook(GetProcAddress(hUser32, "ClipCursor"), &Detour_ClipCursor, (LPVOID*)&g_pfnOriginalClipCursor);
        MH_CreateHook(GetProcAddress(hUser32, "GetAsyncKeyState"), &Detour_GetAsyncKeyState, (LPVOID*)&g_pfnOriginalGetAsyncKeyState);
        MH_CreateHook(GetProcAddress(hUser32, "GetKeyState"), &Detour_GetKeyState, (LPVOID*)&g_pfnOriginalGetKeyState);
        MH_CreateHook(GetProcAddress(hUser32, "GetKeyboardState"), &Detour_GetKeyboardState, (LPVOID*)&g_pfnOriginalGetKeyboardState);
        
        MH_CreateHook(GetProcAddress(hUser32, "PeekMessageA"), &Detour_PeekMessageA, (LPVOID*)&g_pfnOriginalPeekMessageA);
        MH_CreateHook(GetProcAddress(hUser32, "PeekMessageW"), &Detour_PeekMessageW, (LPVOID*)&g_pfnOriginalPeekMessageW);
        MH_CreateHook(GetProcAddress(hUser32, "GetMessageA"), &Detour_GetMessageA, (LPVOID*)&g_pfnOriginalGetMessageA);
        MH_CreateHook(GetProcAddress(hUser32, "GetMessageW"), &Detour_GetMessageW, (LPVOID*)&g_pfnOriginalGetMessageW);
        Log("User32 hooks enabled (including PeekMessage/GetMessage).");
    }

    // Gamepad blocking removed as requested
    uintptr_t pPresentAddr = FindPresentAddress(hTempWnd);
    DestroyWindow(hTempWnd);  // Done with temp window

    if (pPresentAddr) {
        std::stringstream ss;
        ss << "FindPresentAddress successful. Found at: 0x" << std::hex << pPresentAddr;
        Log(ss.str());
        if (MH_CreateHook((LPVOID)pPresentAddr, &My_Present, (LPVOID*)&g_pfnOriginalPresent) != MH_OK) {
            Log("MH_CreateHook for Present failed!");
        } else {
            Log("MH_CreateHook for Present successful.");
        }
    } else {
        Log("FindPresentAddress FAILED!");
    }

    if (MH_EnableHook(MH_ALL_HOOKS) != MH_OK) {
        Log("MH_EnableHook(MH_ALL_HOOKS) failed!");
        return;
    }
    Log("All hooks enabled.");
}

// =============================================================
// DLL ENTRY POINT
// =============================================================
BOOL WINAPI DllMain(HMODULE hModule, DWORD fdwReason, LPVOID lpReserved) {
    if (fdwReason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);
        DetectProxyType(hModule);
        std::thread(InitializeHooks).detach();
    }
    else if (fdwReason == DLL_PROCESS_DETACH) {
        // Minimal cleanup only - full cleanup causes hanging in some games
        g_bWorkerThreadActive = false;
        // Don't join thread or release resources - let OS handle it
    }

    return TRUE;
}
