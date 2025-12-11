#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")

// =============================================================
// PROXY TYPE DETECTION
// =============================================================
enum class ProxyType { None, XInput, Version, DInput8, WinMM };
static ProxyType g_ProxyType = ProxyType::None;
static HMODULE g_hOriginalDll = nullptr;

// =============================================================
// FUNCTION POINTERS
// =============================================================

// XInput
static FARPROC g_pfnXInputGetState = nullptr;
static FARPROC g_pfnXInputSetState = nullptr;
static FARPROC g_pfnXInputGetCapabilities = nullptr;
static FARPROC g_pfnXInputEnable = nullptr;
static FARPROC g_pfnXInputGetAudioDeviceIds = nullptr;
static FARPROC g_pfnXInputGetBatteryInformation = nullptr;
static FARPROC g_pfnXInputGetKeystroke = nullptr;
static FARPROC g_pfnXInputGetDSoundAudioDeviceGuids = nullptr;

// Version
static FARPROC g_pfnGetFileVersionInfoA = nullptr;
static FARPROC g_pfnGetFileVersionInfoW = nullptr;
static FARPROC g_pfnGetFileVersionInfoExA = nullptr;
static FARPROC g_pfnGetFileVersionInfoExW = nullptr;
static FARPROC g_pfnGetFileVersionInfoSizeA = nullptr;
static FARPROC g_pfnGetFileVersionInfoSizeW = nullptr;
static FARPROC g_pfnGetFileVersionInfoSizeExA = nullptr;
static FARPROC g_pfnGetFileVersionInfoSizeExW = nullptr;
static FARPROC g_pfnVerQueryValueA = nullptr;
static FARPROC g_pfnVerQueryValueW = nullptr;
static FARPROC g_pfnVerFindFileA = nullptr;
static FARPROC g_pfnVerFindFileW = nullptr;
static FARPROC g_pfnVerInstallFileA = nullptr;
static FARPROC g_pfnVerInstallFileW = nullptr;
static FARPROC g_pfnVerLanguageNameA = nullptr;
static FARPROC g_pfnVerLanguageNameW = nullptr;

// DInput8
static FARPROC g_pfnDirectInput8Create = nullptr;
static FARPROC g_pfnDInput_DllCanUnloadNow = nullptr;
static FARPROC g_pfnDInput_DllGetClassObject = nullptr;
static FARPROC g_pfnDInput_DllRegisterServer = nullptr;
static FARPROC g_pfnDInput_DllUnregisterServer = nullptr;

// WinMM (essential functions)
static FARPROC g_pfnTimeGetTime = nullptr;
static FARPROC g_pfnTimeBeginPeriod = nullptr;
static FARPROC g_pfnTimeEndPeriod = nullptr;
static FARPROC g_pfnTimeGetDevCaps = nullptr;
static FARPROC g_pfnTimeSetEvent = nullptr;
static FARPROC g_pfnTimeKillEvent = nullptr;
static FARPROC g_pfnTimeGetSystemTime = nullptr;
static FARPROC g_pfnPlaySoundA = nullptr;
static FARPROC g_pfnPlaySoundW = nullptr;
static FARPROC g_pfnsndPlaySoundA = nullptr;
static FARPROC g_pfnsndPlaySoundW = nullptr;
static FARPROC g_pfnWaveOutGetNumDevs = nullptr;
static FARPROC g_pfnWaveOutOpen = nullptr;
static FARPROC g_pfnWaveOutClose = nullptr;
static FARPROC g_pfnWaveOutPrepareHeader = nullptr;
static FARPROC g_pfnWaveOutUnprepareHeader = nullptr;
static FARPROC g_pfnWaveOutWrite = nullptr;
static FARPROC g_pfnWaveOutPause = nullptr;
static FARPROC g_pfnWaveOutRestart = nullptr;
static FARPROC g_pfnWaveOutReset = nullptr;
static FARPROC g_pfnWaveOutBreakLoop = nullptr;
static FARPROC g_pfnWaveOutGetPosition = nullptr;
static FARPROC g_pfnWaveOutGetPitch = nullptr;
static FARPROC g_pfnWaveOutSetPitch = nullptr;
static FARPROC g_pfnWaveOutGetPlaybackRate = nullptr;
static FARPROC g_pfnWaveOutSetPlaybackRate = nullptr;
static FARPROC g_pfnWaveOutGetVolume = nullptr;
static FARPROC g_pfnWaveOutSetVolume = nullptr;
static FARPROC g_pfnWaveOutGetID = nullptr;
static FARPROC g_pfnWaveOutMessage = nullptr;
static FARPROC g_pfnWaveOutGetDevCapsA = nullptr;
static FARPROC g_pfnWaveOutGetDevCapsW = nullptr;
static FARPROC g_pfnWaveOutGetErrorTextA = nullptr;
static FARPROC g_pfnWaveOutGetErrorTextW = nullptr;
static FARPROC g_pfnMciSendCommandA = nullptr;
static FARPROC g_pfnMciSendCommandW = nullptr;
static FARPROC g_pfnMciSendStringA = nullptr;
static FARPROC g_pfnMciSendStringW = nullptr;
static FARPROC g_pfnMciGetErrorStringA = nullptr;
static FARPROC g_pfnMciGetErrorStringW = nullptr;

// =============================================================
// LOADER FUNCTIONS
// =============================================================
static void LoadOriginalXInput() {
    if (g_hOriginalDll) return;
    char path[MAX_PATH];
    GetSystemDirectoryA(path, MAX_PATH);
    strcat_s(path, "\\xinput1_4.dll");
    g_hOriginalDll = LoadLibraryA(path);
    if (!g_hOriginalDll) {
        GetSystemDirectoryA(path, MAX_PATH);
        strcat_s(path, "\\xinput1_3.dll");
        g_hOriginalDll = LoadLibraryA(path);
    }
    if (!g_hOriginalDll) {
        GetSystemDirectoryA(path, MAX_PATH);
        strcat_s(path, "\\xinput9_1_0.dll");
        g_hOriginalDll = LoadLibraryA(path);
    }
    if (g_hOriginalDll) {
        g_pfnXInputGetState = GetProcAddress(g_hOriginalDll, "XInputGetState");
        g_pfnXInputSetState = GetProcAddress(g_hOriginalDll, "XInputSetState");
        g_pfnXInputGetCapabilities = GetProcAddress(g_hOriginalDll, "XInputGetCapabilities");
        g_pfnXInputEnable = GetProcAddress(g_hOriginalDll, "XInputEnable");
        g_pfnXInputGetAudioDeviceIds = GetProcAddress(g_hOriginalDll, "XInputGetAudioDeviceIds");
        g_pfnXInputGetBatteryInformation = GetProcAddress(g_hOriginalDll, "XInputGetBatteryInformation");
        g_pfnXInputGetKeystroke = GetProcAddress(g_hOriginalDll, "XInputGetKeystroke");
        g_pfnXInputGetDSoundAudioDeviceGuids = GetProcAddress(g_hOriginalDll, "XInputGetDSoundAudioDeviceGuids");
    }
}

static void LoadOriginalVersion() {
    if (g_hOriginalDll) return;
    char path[MAX_PATH];
    GetSystemDirectoryA(path, MAX_PATH);
    strcat_s(path, "\\version.dll");
    g_hOriginalDll = LoadLibraryA(path);
    if (g_hOriginalDll) {
        g_pfnGetFileVersionInfoA = GetProcAddress(g_hOriginalDll, "GetFileVersionInfoA");
        g_pfnGetFileVersionInfoW = GetProcAddress(g_hOriginalDll, "GetFileVersionInfoW");
        g_pfnGetFileVersionInfoExA = GetProcAddress(g_hOriginalDll, "GetFileVersionInfoExA");
        g_pfnGetFileVersionInfoExW = GetProcAddress(g_hOriginalDll, "GetFileVersionInfoExW");
        g_pfnGetFileVersionInfoSizeA = GetProcAddress(g_hOriginalDll, "GetFileVersionInfoSizeA");
        g_pfnGetFileVersionInfoSizeW = GetProcAddress(g_hOriginalDll, "GetFileVersionInfoSizeW");
        g_pfnGetFileVersionInfoSizeExA = GetProcAddress(g_hOriginalDll, "GetFileVersionInfoSizeExA");
        g_pfnGetFileVersionInfoSizeExW = GetProcAddress(g_hOriginalDll, "GetFileVersionInfoSizeExW");
        g_pfnVerQueryValueA = GetProcAddress(g_hOriginalDll, "VerQueryValueA");
        g_pfnVerQueryValueW = GetProcAddress(g_hOriginalDll, "VerQueryValueW");
        g_pfnVerFindFileA = GetProcAddress(g_hOriginalDll, "VerFindFileA");
        g_pfnVerFindFileW = GetProcAddress(g_hOriginalDll, "VerFindFileW");
        g_pfnVerInstallFileA = GetProcAddress(g_hOriginalDll, "VerInstallFileA");
        g_pfnVerInstallFileW = GetProcAddress(g_hOriginalDll, "VerInstallFileW");
        g_pfnVerLanguageNameA = GetProcAddress(g_hOriginalDll, "VerLanguageNameA");
        g_pfnVerLanguageNameW = GetProcAddress(g_hOriginalDll, "VerLanguageNameW");
    }
}

static void LoadOriginalDInput8() {
    if (g_hOriginalDll) return;
    char path[MAX_PATH];
    GetSystemDirectoryA(path, MAX_PATH);
    strcat_s(path, "\\dinput8.dll");
    g_hOriginalDll = LoadLibraryA(path);
    if (g_hOriginalDll) {
        g_pfnDirectInput8Create = GetProcAddress(g_hOriginalDll, "DirectInput8Create");
        g_pfnDInput_DllCanUnloadNow = GetProcAddress(g_hOriginalDll, "DllCanUnloadNow");
        g_pfnDInput_DllGetClassObject = GetProcAddress(g_hOriginalDll, "DllGetClassObject");
        g_pfnDInput_DllRegisterServer = GetProcAddress(g_hOriginalDll, "DllRegisterServer");
        g_pfnDInput_DllUnregisterServer = GetProcAddress(g_hOriginalDll, "DllUnregisterServer");
    }
}

static void LoadOriginalWinMM() {
    if (g_hOriginalDll) return;
    char path[MAX_PATH];
    GetSystemDirectoryA(path, MAX_PATH);
    strcat_s(path, "\\winmm.dll");
    g_hOriginalDll = LoadLibraryA(path);
    if (g_hOriginalDll) {
        g_pfnTimeGetTime = GetProcAddress(g_hOriginalDll, "timeGetTime");
        g_pfnTimeBeginPeriod = GetProcAddress(g_hOriginalDll, "timeBeginPeriod");
        g_pfnTimeEndPeriod = GetProcAddress(g_hOriginalDll, "timeEndPeriod");
        g_pfnTimeGetDevCaps = GetProcAddress(g_hOriginalDll, "timeGetDevCaps");
        g_pfnTimeSetEvent = GetProcAddress(g_hOriginalDll, "timeSetEvent");
        g_pfnTimeKillEvent = GetProcAddress(g_hOriginalDll, "timeKillEvent");
        g_pfnTimeGetSystemTime = GetProcAddress(g_hOriginalDll, "timeGetSystemTime");
        g_pfnPlaySoundA = GetProcAddress(g_hOriginalDll, "PlaySoundA");
        g_pfnPlaySoundW = GetProcAddress(g_hOriginalDll, "PlaySoundW");
        g_pfnsndPlaySoundA = GetProcAddress(g_hOriginalDll, "sndPlaySoundA");
        g_pfnsndPlaySoundW = GetProcAddress(g_hOriginalDll, "sndPlaySoundW");
        g_pfnWaveOutGetNumDevs = GetProcAddress(g_hOriginalDll, "waveOutGetNumDevs");
        g_pfnWaveOutOpen = GetProcAddress(g_hOriginalDll, "waveOutOpen");
        g_pfnWaveOutClose = GetProcAddress(g_hOriginalDll, "waveOutClose");
        g_pfnWaveOutPrepareHeader = GetProcAddress(g_hOriginalDll, "waveOutPrepareHeader");
        g_pfnWaveOutUnprepareHeader = GetProcAddress(g_hOriginalDll, "waveOutUnprepareHeader");
        g_pfnWaveOutWrite = GetProcAddress(g_hOriginalDll, "waveOutWrite");
        g_pfnWaveOutPause = GetProcAddress(g_hOriginalDll, "waveOutPause");
        g_pfnWaveOutRestart = GetProcAddress(g_hOriginalDll, "waveOutRestart");
        g_pfnWaveOutReset = GetProcAddress(g_hOriginalDll, "waveOutReset");
        g_pfnWaveOutBreakLoop = GetProcAddress(g_hOriginalDll, "waveOutBreakLoop");
        g_pfnWaveOutGetPosition = GetProcAddress(g_hOriginalDll, "waveOutGetPosition");
        g_pfnWaveOutGetPitch = GetProcAddress(g_hOriginalDll, "waveOutGetPitch");
        g_pfnWaveOutSetPitch = GetProcAddress(g_hOriginalDll, "waveOutSetPitch");
        g_pfnWaveOutGetPlaybackRate = GetProcAddress(g_hOriginalDll, "waveOutGetPlaybackRate");
        g_pfnWaveOutSetPlaybackRate = GetProcAddress(g_hOriginalDll, "waveOutSetPlaybackRate");
        g_pfnWaveOutGetVolume = GetProcAddress(g_hOriginalDll, "waveOutGetVolume");
        g_pfnWaveOutSetVolume = GetProcAddress(g_hOriginalDll, "waveOutSetVolume");
        g_pfnWaveOutGetID = GetProcAddress(g_hOriginalDll, "waveOutGetID");
        g_pfnWaveOutMessage = GetProcAddress(g_hOriginalDll, "waveOutMessage");
        g_pfnWaveOutGetDevCapsA = GetProcAddress(g_hOriginalDll, "waveOutGetDevCapsA");
        g_pfnWaveOutGetDevCapsW = GetProcAddress(g_hOriginalDll, "waveOutGetDevCapsW");
        g_pfnWaveOutGetErrorTextA = GetProcAddress(g_hOriginalDll, "waveOutGetErrorTextA");
        g_pfnWaveOutGetErrorTextW = GetProcAddress(g_hOriginalDll, "waveOutGetErrorTextW");
        g_pfnMciSendCommandA = GetProcAddress(g_hOriginalDll, "mciSendCommandA");
        g_pfnMciSendCommandW = GetProcAddress(g_hOriginalDll, "mciSendCommandW");
        g_pfnMciSendStringA = GetProcAddress(g_hOriginalDll, "mciSendStringA");
        g_pfnMciSendStringW = GetProcAddress(g_hOriginalDll, "mciSendStringW");
        g_pfnMciGetErrorStringA = GetProcAddress(g_hOriginalDll, "mciGetErrorStringA");
        g_pfnMciGetErrorStringW = GetProcAddress(g_hOriginalDll, "mciGetErrorStringW");
    }
}

// Call this in DllMain or your init function
static void DetectProxyType(HMODULE hModule) {
    char dllPath[MAX_PATH];
    GetModuleFileNameA(hModule, dllPath, MAX_PATH);
    char* filename = strrchr(dllPath, '\\');
    if (filename) filename++; else filename = dllPath;
    char lower[MAX_PATH];
    strcpy_s(lower, filename);
    _strlwr_s(lower);

    if (strstr(lower, "xinput")) { g_ProxyType = ProxyType::XInput; LoadOriginalXInput(); }
    else if (strstr(lower, "version")) { g_ProxyType = ProxyType::Version; LoadOriginalVersion(); }
    else if (strstr(lower, "dinput8")) { g_ProxyType = ProxyType::DInput8; LoadOriginalDInput8(); }
    else if (strstr(lower, "winmm")) { g_ProxyType = ProxyType::WinMM; LoadOriginalWinMM(); }
    else { g_ProxyType = ProxyType::XInput; LoadOriginalXInput(); } // Default fallback
}

// =============================================================
// EXPORTED PROXY FUNCTIONS - XINPUT
// =============================================================
extern "C" {

__declspec(dllexport) DWORD WINAPI Proxy_XInputGetState(DWORD dwUserIndex, void* pState) {
    if (!g_pfnXInputGetState) LoadOriginalXInput();
    if (g_pfnXInputGetState) return ((DWORD(WINAPI*)(DWORD, void*))g_pfnXInputGetState)(dwUserIndex, pState);
    return ERROR_DEVICE_NOT_CONNECTED;
}

__declspec(dllexport) DWORD WINAPI Proxy_XInputSetState(DWORD dwUserIndex, void* pVibration) {
    if (!g_pfnXInputSetState) LoadOriginalXInput();
    if (g_pfnXInputSetState) return ((DWORD(WINAPI*)(DWORD, void*))g_pfnXInputSetState)(dwUserIndex, pVibration);
    return ERROR_DEVICE_NOT_CONNECTED;
}

__declspec(dllexport) DWORD WINAPI Proxy_XInputGetCapabilities(DWORD dwUserIndex, DWORD dwFlags, void* pCapabilities) {
    if (!g_pfnXInputGetCapabilities) LoadOriginalXInput();
    if (g_pfnXInputGetCapabilities) return ((DWORD(WINAPI*)(DWORD, DWORD, void*))g_pfnXInputGetCapabilities)(dwUserIndex, dwFlags, pCapabilities);
    return ERROR_DEVICE_NOT_CONNECTED;
}

__declspec(dllexport) void WINAPI Proxy_XInputEnable(BOOL enable) {
    if (!g_pfnXInputEnable) LoadOriginalXInput();
    if (g_pfnXInputEnable) ((void(WINAPI*)(BOOL))g_pfnXInputEnable)(enable);
}

__declspec(dllexport) DWORD WINAPI Proxy_XInputGetAudioDeviceIds(DWORD dwUserIndex, void* pRenderDeviceId, void* pRenderCount, void* pCaptureDeviceId, void* pCaptureCount) {
    if (!g_pfnXInputGetAudioDeviceIds) LoadOriginalXInput();
    if (g_pfnXInputGetAudioDeviceIds) return ((DWORD(WINAPI*)(DWORD, void*, void*, void*, void*))g_pfnXInputGetAudioDeviceIds)(dwUserIndex, pRenderDeviceId, pRenderCount, pCaptureDeviceId, pCaptureCount);
    return ERROR_DEVICE_NOT_CONNECTED;
}

__declspec(dllexport) DWORD WINAPI Proxy_XInputGetBatteryInformation(DWORD dwUserIndex, BYTE devType, void* pBatteryInformation) {
    if (!g_pfnXInputGetBatteryInformation) LoadOriginalXInput();
    if (g_pfnXInputGetBatteryInformation) return ((DWORD(WINAPI*)(DWORD, BYTE, void*))g_pfnXInputGetBatteryInformation)(dwUserIndex, devType, pBatteryInformation);
    return ERROR_DEVICE_NOT_CONNECTED;
}

__declspec(dllexport) DWORD WINAPI Proxy_XInputGetKeystroke(DWORD dwUserIndex, DWORD dwReserved, void* pKeystroke) {
    if (!g_pfnXInputGetKeystroke) LoadOriginalXInput();
    if (g_pfnXInputGetKeystroke) return ((DWORD(WINAPI*)(DWORD, DWORD, void*))g_pfnXInputGetKeystroke)(dwUserIndex, dwReserved, pKeystroke);
    return ERROR_DEVICE_NOT_CONNECTED;
}

__declspec(dllexport) DWORD WINAPI Proxy_XInputGetDSoundAudioDeviceGuids(DWORD dwUserIndex, void* pDSoundRenderGuid, void* pDSoundCaptureGuid) {
    if (!g_pfnXInputGetDSoundAudioDeviceGuids) LoadOriginalXInput();
    if (g_pfnXInputGetDSoundAudioDeviceGuids) return ((DWORD(WINAPI*)(DWORD, void*, void*))g_pfnXInputGetDSoundAudioDeviceGuids)(dwUserIndex, pDSoundRenderGuid, pDSoundCaptureGuid);
    return ERROR_DEVICE_NOT_CONNECTED;
}

// =============================================================
// EXPORTED PROXY FUNCTIONS - VERSION.DLL
// =============================================================
__declspec(dllexport) BOOL WINAPI Proxy_GetFileVersionInfoA(LPCSTR lptstrFilename, DWORD dwHandle, DWORD dwLen, LPVOID lpData) {
    if (!g_pfnGetFileVersionInfoA) LoadOriginalVersion();
    if (g_pfnGetFileVersionInfoA) return ((BOOL(WINAPI*)(LPCSTR, DWORD, DWORD, LPVOID))g_pfnGetFileVersionInfoA)(lptstrFilename, dwHandle, dwLen, lpData);
    return FALSE;
}

__declspec(dllexport) BOOL WINAPI Proxy_GetFileVersionInfoW(LPCWSTR lptstrFilename, DWORD dwHandle, DWORD dwLen, LPVOID lpData) {
    if (!g_pfnGetFileVersionInfoW) LoadOriginalVersion();
    if (g_pfnGetFileVersionInfoW) return ((BOOL(WINAPI*)(LPCWSTR, DWORD, DWORD, LPVOID))g_pfnGetFileVersionInfoW)(lptstrFilename, dwHandle, dwLen, lpData);
    return FALSE;
}

__declspec(dllexport) BOOL WINAPI Proxy_GetFileVersionInfoExA(DWORD dwFlags, LPCSTR lpwstrFilename, DWORD dwHandle, DWORD dwLen, LPVOID lpData) {
    if (!g_pfnGetFileVersionInfoExA) LoadOriginalVersion();
    if (g_pfnGetFileVersionInfoExA) return ((BOOL(WINAPI*)(DWORD, LPCSTR, DWORD, DWORD, LPVOID))g_pfnGetFileVersionInfoExA)(dwFlags, lpwstrFilename, dwHandle, dwLen, lpData);
    return FALSE;
}

__declspec(dllexport) BOOL WINAPI Proxy_GetFileVersionInfoExW(DWORD dwFlags, LPCWSTR lpwstrFilename, DWORD dwHandle, DWORD dwLen, LPVOID lpData) {
    if (!g_pfnGetFileVersionInfoExW) LoadOriginalVersion();
    if (g_pfnGetFileVersionInfoExW) return ((BOOL(WINAPI*)(DWORD, LPCWSTR, DWORD, DWORD, LPVOID))g_pfnGetFileVersionInfoExW)(dwFlags, lpwstrFilename, dwHandle, dwLen, lpData);
    return FALSE;
}

__declspec(dllexport) DWORD WINAPI Proxy_GetFileVersionInfoSizeA(LPCSTR lptstrFilename, LPDWORD lpdwHandle) {
    if (!g_pfnGetFileVersionInfoSizeA) LoadOriginalVersion();
    if (g_pfnGetFileVersionInfoSizeA) return ((DWORD(WINAPI*)(LPCSTR, LPDWORD))g_pfnGetFileVersionInfoSizeA)(lptstrFilename, lpdwHandle);
    return 0;
}

__declspec(dllexport) DWORD WINAPI Proxy_GetFileVersionInfoSizeW(LPCWSTR lptstrFilename, LPDWORD lpdwHandle) {
    if (!g_pfnGetFileVersionInfoSizeW) LoadOriginalVersion();
    if (g_pfnGetFileVersionInfoSizeW) return ((DWORD(WINAPI*)(LPCWSTR, LPDWORD))g_pfnGetFileVersionInfoSizeW)(lptstrFilename, lpdwHandle);
    return 0;
}

__declspec(dllexport) DWORD WINAPI Proxy_GetFileVersionInfoSizeExA(DWORD dwFlags, LPCSTR lpwstrFilename, LPDWORD lpdwHandle) {
    if (!g_pfnGetFileVersionInfoSizeExA) LoadOriginalVersion();
    if (g_pfnGetFileVersionInfoSizeExA) return ((DWORD(WINAPI*)(DWORD, LPCSTR, LPDWORD))g_pfnGetFileVersionInfoSizeExA)(dwFlags, lpwstrFilename, lpdwHandle);
    return 0;
}

__declspec(dllexport) DWORD WINAPI Proxy_GetFileVersionInfoSizeExW(DWORD dwFlags, LPCWSTR lpwstrFilename, LPDWORD lpdwHandle) {
    if (!g_pfnGetFileVersionInfoSizeExW) LoadOriginalVersion();
    if (g_pfnGetFileVersionInfoSizeExW) return ((DWORD(WINAPI*)(DWORD, LPCWSTR, LPDWORD))g_pfnGetFileVersionInfoSizeExW)(dwFlags, lpwstrFilename, lpdwHandle);
    return 0;
}

__declspec(dllexport) BOOL WINAPI Proxy_VerQueryValueA(LPCVOID pBlock, LPCSTR lpSubBlock, LPVOID* lplpBuffer, PUINT puLen) {
    if (!g_pfnVerQueryValueA) LoadOriginalVersion();
    if (g_pfnVerQueryValueA) return ((BOOL(WINAPI*)(LPCVOID, LPCSTR, LPVOID*, PUINT))g_pfnVerQueryValueA)(pBlock, lpSubBlock, lplpBuffer, puLen);
    return FALSE;
}

__declspec(dllexport) BOOL WINAPI Proxy_VerQueryValueW(LPCVOID pBlock, LPCWSTR lpSubBlock, LPVOID* lplpBuffer, PUINT puLen) {
    if (!g_pfnVerQueryValueW) LoadOriginalVersion();
    if (g_pfnVerQueryValueW) return ((BOOL(WINAPI*)(LPCVOID, LPCWSTR, LPVOID*, PUINT))g_pfnVerQueryValueW)(pBlock, lpSubBlock, lplpBuffer, puLen);
    return FALSE;
}

__declspec(dllexport) DWORD WINAPI Proxy_VerFindFileA(DWORD uFlags, LPCSTR szFileName, LPCSTR szWinDir, LPCSTR szAppDir, LPSTR szCurDir, PUINT puCurDirLen, LPSTR szDestDir, PUINT puDestDirLen) {
    if (!g_pfnVerFindFileA) LoadOriginalVersion();
    if (g_pfnVerFindFileA) return ((DWORD(WINAPI*)(DWORD, LPCSTR, LPCSTR, LPCSTR, LPSTR, PUINT, LPSTR, PUINT))g_pfnVerFindFileA)(uFlags, szFileName, szWinDir, szAppDir, szCurDir, puCurDirLen, szDestDir, puDestDirLen);
    return 0;
}

__declspec(dllexport) DWORD WINAPI Proxy_VerFindFileW(DWORD uFlags, LPCWSTR szFileName, LPCWSTR szWinDir, LPCWSTR szAppDir, LPWSTR szCurDir, PUINT puCurDirLen, LPWSTR szDestDir, PUINT puDestDirLen) {
    if (!g_pfnVerFindFileW) LoadOriginalVersion();
    if (g_pfnVerFindFileW) return ((DWORD(WINAPI*)(DWORD, LPCWSTR, LPCWSTR, LPCWSTR, LPWSTR, PUINT, LPWSTR, PUINT))g_pfnVerFindFileW)(uFlags, szFileName, szWinDir, szAppDir, szCurDir, puCurDirLen, szDestDir, puDestDirLen);
    return 0;
}

__declspec(dllexport) DWORD WINAPI Proxy_VerInstallFileA(DWORD uFlags, LPCSTR szSrcFileName, LPCSTR szDestFileName, LPCSTR szSrcDir, LPCSTR szDestDir, LPCSTR szCurDir, LPSTR szTmpFile, PUINT puTmpFileLen) {
    if (!g_pfnVerInstallFileA) LoadOriginalVersion();
    if (g_pfnVerInstallFileA) return ((DWORD(WINAPI*)(DWORD, LPCSTR, LPCSTR, LPCSTR, LPCSTR, LPCSTR, LPSTR, PUINT))g_pfnVerInstallFileA)(uFlags, szSrcFileName, szDestFileName, szSrcDir, szDestDir, szCurDir, szTmpFile, puTmpFileLen);
    return 0;
}

__declspec(dllexport) DWORD WINAPI Proxy_VerInstallFileW(DWORD uFlags, LPCWSTR szSrcFileName, LPCWSTR szDestFileName, LPCWSTR szSrcDir, LPCWSTR szDestDir, LPCWSTR szCurDir, LPWSTR szTmpFile, PUINT puTmpFileLen) {
    if (!g_pfnVerInstallFileW) LoadOriginalVersion();
    if (g_pfnVerInstallFileW) return ((DWORD(WINAPI*)(DWORD, LPCWSTR, LPCWSTR, LPCWSTR, LPCWSTR, LPCWSTR, LPWSTR, PUINT))g_pfnVerInstallFileW)(uFlags, szSrcFileName, szDestFileName, szSrcDir, szDestDir, szCurDir, szTmpFile, puTmpFileLen);
    return 0;
}

__declspec(dllexport) DWORD WINAPI Proxy_VerLanguageNameA(DWORD wLang, LPSTR szLang, DWORD cchLang) {
    if (!g_pfnVerLanguageNameA) LoadOriginalVersion();
    if (g_pfnVerLanguageNameA) return ((DWORD(WINAPI*)(DWORD, LPSTR, DWORD))g_pfnVerLanguageNameA)(wLang, szLang, cchLang);
    return 0;
}

__declspec(dllexport) DWORD WINAPI Proxy_VerLanguageNameW(DWORD wLang, LPWSTR szLang, DWORD cchLang) {
    if (!g_pfnVerLanguageNameW) LoadOriginalVersion();
    if (g_pfnVerLanguageNameW) return ((DWORD(WINAPI*)(DWORD, LPWSTR, DWORD))g_pfnVerLanguageNameW)(wLang, szLang, cchLang);
    return 0;
}

// =============================================================
// EXPORTED PROXY FUNCTIONS - DINPUT8.DLL
// =============================================================
__declspec(dllexport) HRESULT WINAPI Proxy_DirectInput8Create(HINSTANCE hinst, DWORD dwVersion, REFIID riidltf, LPVOID* ppvOut, void* punkOuter) {
    if (!g_pfnDirectInput8Create) LoadOriginalDInput8();
    if (g_pfnDirectInput8Create) return ((HRESULT(WINAPI*)(HINSTANCE, DWORD, REFIID, LPVOID*, void*))g_pfnDirectInput8Create)(hinst, dwVersion, riidltf, ppvOut, punkOuter);
    return E_FAIL;
}

__declspec(dllexport) HRESULT WINAPI Proxy_DInput_DllCanUnloadNow() {
    if (!g_pfnDInput_DllCanUnloadNow) LoadOriginalDInput8();
    if (g_pfnDInput_DllCanUnloadNow) return ((HRESULT(WINAPI*)())g_pfnDInput_DllCanUnloadNow)();
    return S_FALSE;
}

__declspec(dllexport) HRESULT WINAPI Proxy_DInput_DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv) {
    if (!g_pfnDInput_DllGetClassObject) LoadOriginalDInput8();
    if (g_pfnDInput_DllGetClassObject) return ((HRESULT(WINAPI*)(REFCLSID, REFIID, LPVOID*))g_pfnDInput_DllGetClassObject)(rclsid, riid, ppv);
    return CLASS_E_CLASSNOTAVAILABLE;
}

__declspec(dllexport) HRESULT WINAPI Proxy_DInput_DllRegisterServer() {
    if (!g_pfnDInput_DllRegisterServer) LoadOriginalDInput8();
    if (g_pfnDInput_DllRegisterServer) return ((HRESULT(WINAPI*)())g_pfnDInput_DllRegisterServer)();
    return E_FAIL;
}

__declspec(dllexport) HRESULT WINAPI Proxy_DInput_DllUnregisterServer() {
    if (!g_pfnDInput_DllUnregisterServer) LoadOriginalDInput8();
    if (g_pfnDInput_DllUnregisterServer) return ((HRESULT(WINAPI*)())g_pfnDInput_DllUnregisterServer)();
    return E_FAIL;
}

// =============================================================
// EXPORTED PROXY FUNCTIONS - WINMM.DLL
// =============================================================
__declspec(dllexport) DWORD WINAPI Proxy_timeGetTime() {
    if (!g_pfnTimeGetTime) LoadOriginalWinMM();
    if (g_pfnTimeGetTime) return ((DWORD(WINAPI*)())g_pfnTimeGetTime)();
    return 0;
}

__declspec(dllexport) MMRESULT WINAPI Proxy_timeBeginPeriod(UINT uPeriod) {
    if (!g_pfnTimeBeginPeriod) LoadOriginalWinMM();
    if (g_pfnTimeBeginPeriod) return ((MMRESULT(WINAPI*)(UINT))g_pfnTimeBeginPeriod)(uPeriod);
    return TIMERR_NOCANDO;
}

__declspec(dllexport) MMRESULT WINAPI Proxy_timeEndPeriod(UINT uPeriod) {
    if (!g_pfnTimeEndPeriod) LoadOriginalWinMM();
    if (g_pfnTimeEndPeriod) return ((MMRESULT(WINAPI*)(UINT))g_pfnTimeEndPeriod)(uPeriod);
    return TIMERR_NOCANDO;
}

__declspec(dllexport) MMRESULT WINAPI Proxy_timeGetDevCaps(LPTIMECAPS ptc, UINT cbtc) {
    if (!g_pfnTimeGetDevCaps) LoadOriginalWinMM();
    if (g_pfnTimeGetDevCaps) return ((MMRESULT(WINAPI*)(LPTIMECAPS, UINT))g_pfnTimeGetDevCaps)(ptc, cbtc);
    return TIMERR_NOCANDO;
}

__declspec(dllexport) MMRESULT WINAPI Proxy_timeSetEvent(UINT uDelay, UINT uResolution, LPTIMECALLBACK fptc, DWORD_PTR dwUser, UINT fuEvent) {
    if (!g_pfnTimeSetEvent) LoadOriginalWinMM();
    if (g_pfnTimeSetEvent) return ((MMRESULT(WINAPI*)(UINT, UINT, LPTIMECALLBACK, DWORD_PTR, UINT))g_pfnTimeSetEvent)(uDelay, uResolution, fptc, dwUser, fuEvent);
    return 0;
}

__declspec(dllexport) MMRESULT WINAPI Proxy_timeKillEvent(UINT uTimerID) {
    if (!g_pfnTimeKillEvent) LoadOriginalWinMM();
    if (g_pfnTimeKillEvent) return ((MMRESULT(WINAPI*)(UINT))g_pfnTimeKillEvent)(uTimerID);
    return TIMERR_NOCANDO;
}

__declspec(dllexport) MMRESULT WINAPI Proxy_timeGetSystemTime(LPMMTIME pmmt, UINT cbmmt) {
    if (!g_pfnTimeGetSystemTime) LoadOriginalWinMM();
    if (g_pfnTimeGetSystemTime) return ((MMRESULT(WINAPI*)(LPMMTIME, UINT))g_pfnTimeGetSystemTime)(pmmt, cbmmt);
    return TIMERR_NOCANDO;
}

__declspec(dllexport) BOOL WINAPI Proxy_PlaySoundA(LPCSTR pszSound, HMODULE hmod, DWORD fdwSound) {
    if (!g_pfnPlaySoundA) LoadOriginalWinMM();
    if (g_pfnPlaySoundA) return ((BOOL(WINAPI*)(LPCSTR, HMODULE, DWORD))g_pfnPlaySoundA)(pszSound, hmod, fdwSound);
    return FALSE;
}

__declspec(dllexport) BOOL WINAPI Proxy_PlaySoundW(LPCWSTR pszSound, HMODULE hmod, DWORD fdwSound) {
    if (!g_pfnPlaySoundW) LoadOriginalWinMM();
    if (g_pfnPlaySoundW) return ((BOOL(WINAPI*)(LPCWSTR, HMODULE, DWORD))g_pfnPlaySoundW)(pszSound, hmod, fdwSound);
    return FALSE;
}

__declspec(dllexport) BOOL WINAPI Proxy_sndPlaySoundA(LPCSTR pszSound, UINT fuSound) {
    if (!g_pfnsndPlaySoundA) LoadOriginalWinMM();
    if (g_pfnsndPlaySoundA) return ((BOOL(WINAPI*)(LPCSTR, UINT))g_pfnsndPlaySoundA)(pszSound, fuSound);
    return FALSE;
}

__declspec(dllexport) BOOL WINAPI Proxy_sndPlaySoundW(LPCWSTR pszSound, UINT fuSound) {
    if (!g_pfnsndPlaySoundW) LoadOriginalWinMM();
    if (g_pfnsndPlaySoundW) return ((BOOL(WINAPI*)(LPCWSTR, UINT))g_pfnsndPlaySoundW)(pszSound, fuSound);
    return FALSE;
}

__declspec(dllexport) UINT WINAPI Proxy_waveOutGetNumDevs() {
    if (!g_pfnWaveOutGetNumDevs) LoadOriginalWinMM();
    if (g_pfnWaveOutGetNumDevs) return ((UINT(WINAPI*)())g_pfnWaveOutGetNumDevs)();
    return 0;
}

__declspec(dllexport) MMRESULT WINAPI Proxy_waveOutOpen(LPHWAVEOUT phwo, UINT uDeviceID, LPCWAVEFORMATEX pwfx, DWORD_PTR dwCallback, DWORD_PTR dwInstance, DWORD fdwOpen) {
    if (!g_pfnWaveOutOpen) LoadOriginalWinMM();
    if (g_pfnWaveOutOpen) return ((MMRESULT(WINAPI*)(LPHWAVEOUT, UINT, LPCWAVEFORMATEX, DWORD_PTR, DWORD_PTR, DWORD))g_pfnWaveOutOpen)(phwo, uDeviceID, pwfx, dwCallback, dwInstance, fdwOpen);
    return MMSYSERR_ERROR;
}

__declspec(dllexport) MMRESULT WINAPI Proxy_waveOutClose(HWAVEOUT hwo) {
    if (!g_pfnWaveOutClose) LoadOriginalWinMM();
    if (g_pfnWaveOutClose) return ((MMRESULT(WINAPI*)(HWAVEOUT))g_pfnWaveOutClose)(hwo);
    return MMSYSERR_ERROR;
}

__declspec(dllexport) MMRESULT WINAPI Proxy_waveOutPrepareHeader(HWAVEOUT hwo, LPWAVEHDR pwh, UINT cbwh) {
    if (!g_pfnWaveOutPrepareHeader) LoadOriginalWinMM();
    if (g_pfnWaveOutPrepareHeader) return ((MMRESULT(WINAPI*)(HWAVEOUT, LPWAVEHDR, UINT))g_pfnWaveOutPrepareHeader)(hwo, pwh, cbwh);
    return MMSYSERR_ERROR;
}

__declspec(dllexport) MMRESULT WINAPI Proxy_waveOutUnprepareHeader(HWAVEOUT hwo, LPWAVEHDR pwh, UINT cbwh) {
    if (!g_pfnWaveOutUnprepareHeader) LoadOriginalWinMM();
    if (g_pfnWaveOutUnprepareHeader) return ((MMRESULT(WINAPI*)(HWAVEOUT, LPWAVEHDR, UINT))g_pfnWaveOutUnprepareHeader)(hwo, pwh, cbwh);
    return MMSYSERR_ERROR;
}

__declspec(dllexport) MMRESULT WINAPI Proxy_waveOutWrite(HWAVEOUT hwo, LPWAVEHDR pwh, UINT cbwh) {
    if (!g_pfnWaveOutWrite) LoadOriginalWinMM();
    if (g_pfnWaveOutWrite) return ((MMRESULT(WINAPI*)(HWAVEOUT, LPWAVEHDR, UINT))g_pfnWaveOutWrite)(hwo, pwh, cbwh);
    return MMSYSERR_ERROR;
}

__declspec(dllexport) MMRESULT WINAPI Proxy_waveOutPause(HWAVEOUT hwo) {
    if (!g_pfnWaveOutPause) LoadOriginalWinMM();
    if (g_pfnWaveOutPause) return ((MMRESULT(WINAPI*)(HWAVEOUT))g_pfnWaveOutPause)(hwo);
    return MMSYSERR_ERROR;
}

__declspec(dllexport) MMRESULT WINAPI Proxy_waveOutRestart(HWAVEOUT hwo) {
    if (!g_pfnWaveOutRestart) LoadOriginalWinMM();
    if (g_pfnWaveOutRestart) return ((MMRESULT(WINAPI*)(HWAVEOUT))g_pfnWaveOutRestart)(hwo);
    return MMSYSERR_ERROR;
}

__declspec(dllexport) MMRESULT WINAPI Proxy_waveOutReset(HWAVEOUT hwo) {
    if (!g_pfnWaveOutReset) LoadOriginalWinMM();
    if (g_pfnWaveOutReset) return ((MMRESULT(WINAPI*)(HWAVEOUT))g_pfnWaveOutReset)(hwo);
    return MMSYSERR_ERROR;
}

__declspec(dllexport) MMRESULT WINAPI Proxy_waveOutBreakLoop(HWAVEOUT hwo) {
    if (!g_pfnWaveOutBreakLoop) LoadOriginalWinMM();
    if (g_pfnWaveOutBreakLoop) return ((MMRESULT(WINAPI*)(HWAVEOUT))g_pfnWaveOutBreakLoop)(hwo);
    return MMSYSERR_ERROR;
}

__declspec(dllexport) MMRESULT WINAPI Proxy_waveOutGetPosition(HWAVEOUT hwo, LPMMTIME pmmt, UINT cbmmt) {
    if (!g_pfnWaveOutGetPosition) LoadOriginalWinMM();
    if (g_pfnWaveOutGetPosition) return ((MMRESULT(WINAPI*)(HWAVEOUT, LPMMTIME, UINT))g_pfnWaveOutGetPosition)(hwo, pmmt, cbmmt);
    return MMSYSERR_ERROR;
}

__declspec(dllexport) MMRESULT WINAPI Proxy_waveOutGetPitch(HWAVEOUT hwo, LPDWORD pdwPitch) {
    if (!g_pfnWaveOutGetPitch) LoadOriginalWinMM();
    if (g_pfnWaveOutGetPitch) return ((MMRESULT(WINAPI*)(HWAVEOUT, LPDWORD))g_pfnWaveOutGetPitch)(hwo, pdwPitch);
    return MMSYSERR_ERROR;
}

__declspec(dllexport) MMRESULT WINAPI Proxy_waveOutSetPitch(HWAVEOUT hwo, DWORD dwPitch) {
    if (!g_pfnWaveOutSetPitch) LoadOriginalWinMM();
    if (g_pfnWaveOutSetPitch) return ((MMRESULT(WINAPI*)(HWAVEOUT, DWORD))g_pfnWaveOutSetPitch)(hwo, dwPitch);
    return MMSYSERR_ERROR;
}

__declspec(dllexport) MMRESULT WINAPI Proxy_waveOutGetPlaybackRate(HWAVEOUT hwo, LPDWORD pdwRate) {
    if (!g_pfnWaveOutGetPlaybackRate) LoadOriginalWinMM();
    if (g_pfnWaveOutGetPlaybackRate) return ((MMRESULT(WINAPI*)(HWAVEOUT, LPDWORD))g_pfnWaveOutGetPlaybackRate)(hwo, pdwRate);
    return MMSYSERR_ERROR;
}

__declspec(dllexport) MMRESULT WINAPI Proxy_waveOutSetPlaybackRate(HWAVEOUT hwo, DWORD dwRate) {
    if (!g_pfnWaveOutSetPlaybackRate) LoadOriginalWinMM();
    if (g_pfnWaveOutSetPlaybackRate) return ((MMRESULT(WINAPI*)(HWAVEOUT, DWORD))g_pfnWaveOutSetPlaybackRate)(hwo, dwRate);
    return MMSYSERR_ERROR;
}

__declspec(dllexport) MMRESULT WINAPI Proxy_waveOutGetVolume(HWAVEOUT hwo, LPDWORD pdwVolume) {
    if (!g_pfnWaveOutGetVolume) LoadOriginalWinMM();
    if (g_pfnWaveOutGetVolume) return ((MMRESULT(WINAPI*)(HWAVEOUT, LPDWORD))g_pfnWaveOutGetVolume)(hwo, pdwVolume);
    return MMSYSERR_ERROR;
}

__declspec(dllexport) MMRESULT WINAPI Proxy_waveOutSetVolume(HWAVEOUT hwo, DWORD dwVolume) {
    if (!g_pfnWaveOutSetVolume) LoadOriginalWinMM();
    if (g_pfnWaveOutSetVolume) return ((MMRESULT(WINAPI*)(HWAVEOUT, DWORD))g_pfnWaveOutSetVolume)(hwo, dwVolume);
    return MMSYSERR_ERROR;
}

__declspec(dllexport) MMRESULT WINAPI Proxy_waveOutGetID(HWAVEOUT hwo, LPUINT puDeviceID) {
    if (!g_pfnWaveOutGetID) LoadOriginalWinMM();
    if (g_pfnWaveOutGetID) return ((MMRESULT(WINAPI*)(HWAVEOUT, LPUINT))g_pfnWaveOutGetID)(hwo, puDeviceID);
    return MMSYSERR_ERROR;
}

__declspec(dllexport) MMRESULT WINAPI Proxy_waveOutMessage(HWAVEOUT hwo, UINT uMsg, DWORD_PTR dw1, DWORD_PTR dw2) {
    if (!g_pfnWaveOutMessage) LoadOriginalWinMM();
    if (g_pfnWaveOutMessage) return ((MMRESULT(WINAPI*)(HWAVEOUT, UINT, DWORD_PTR, DWORD_PTR))g_pfnWaveOutMessage)(hwo, uMsg, dw1, dw2);
    return MMSYSERR_ERROR;
}

__declspec(dllexport) MMRESULT WINAPI Proxy_waveOutGetDevCapsA(UINT_PTR uDeviceID, LPWAVEOUTCAPSA pwoc, UINT cbwoc) {
    if (!g_pfnWaveOutGetDevCapsA) LoadOriginalWinMM();
    if (g_pfnWaveOutGetDevCapsA) return ((MMRESULT(WINAPI*)(UINT_PTR, LPWAVEOUTCAPSA, UINT))g_pfnWaveOutGetDevCapsA)(uDeviceID, pwoc, cbwoc);
    return MMSYSERR_ERROR;
}

__declspec(dllexport) MMRESULT WINAPI Proxy_waveOutGetDevCapsW(UINT_PTR uDeviceID, LPWAVEOUTCAPSW pwoc, UINT cbwoc) {
    if (!g_pfnWaveOutGetDevCapsW) LoadOriginalWinMM();
    if (g_pfnWaveOutGetDevCapsW) return ((MMRESULT(WINAPI*)(UINT_PTR, LPWAVEOUTCAPSW, UINT))g_pfnWaveOutGetDevCapsW)(uDeviceID, pwoc, cbwoc);
    return MMSYSERR_ERROR;
}

__declspec(dllexport) MMRESULT WINAPI Proxy_waveOutGetErrorTextA(MMRESULT mmrError, LPSTR pszText, UINT cchText) {
    if (!g_pfnWaveOutGetErrorTextA) LoadOriginalWinMM();
    if (g_pfnWaveOutGetErrorTextA) return ((MMRESULT(WINAPI*)(MMRESULT, LPSTR, UINT))g_pfnWaveOutGetErrorTextA)(mmrError, pszText, cchText);
    return MMSYSERR_ERROR;
}

__declspec(dllexport) MMRESULT WINAPI Proxy_waveOutGetErrorTextW(MMRESULT mmrError, LPWSTR pszText, UINT cchText) {
    if (!g_pfnWaveOutGetErrorTextW) LoadOriginalWinMM();
    if (g_pfnWaveOutGetErrorTextW) return ((MMRESULT(WINAPI*)(MMRESULT, LPWSTR, UINT))g_pfnWaveOutGetErrorTextW)(mmrError, pszText, cchText);
    return MMSYSERR_ERROR;
}

__declspec(dllexport) MCIERROR WINAPI Proxy_mciSendCommandA(MCIDEVICEID mciId, UINT uMsg, DWORD_PTR dwParam1, DWORD_PTR dwParam2) {
    if (!g_pfnMciSendCommandA) LoadOriginalWinMM();
    if (g_pfnMciSendCommandA) return ((MCIERROR(WINAPI*)(MCIDEVICEID, UINT, DWORD_PTR, DWORD_PTR))g_pfnMciSendCommandA)(mciId, uMsg, dwParam1, dwParam2);
    return MCIERR_UNRECOGNIZED_COMMAND;
}

__declspec(dllexport) MCIERROR WINAPI Proxy_mciSendCommandW(MCIDEVICEID mciId, UINT uMsg, DWORD_PTR dwParam1, DWORD_PTR dwParam2) {
    if (!g_pfnMciSendCommandW) LoadOriginalWinMM();
    if (g_pfnMciSendCommandW) return ((MCIERROR(WINAPI*)(MCIDEVICEID, UINT, DWORD_PTR, DWORD_PTR))g_pfnMciSendCommandW)(mciId, uMsg, dwParam1, dwParam2);
    return MCIERR_UNRECOGNIZED_COMMAND;
}

__declspec(dllexport) MCIERROR WINAPI Proxy_mciSendStringA(LPCSTR lpstrCommand, LPSTR lpstrReturnString, UINT uReturnLength, HWND hwndCallback) {
    if (!g_pfnMciSendStringA) LoadOriginalWinMM();
    if (g_pfnMciSendStringA) return ((MCIERROR(WINAPI*)(LPCSTR, LPSTR, UINT, HWND))g_pfnMciSendStringA)(lpstrCommand, lpstrReturnString, uReturnLength, hwndCallback);
    return MCIERR_UNRECOGNIZED_COMMAND;
}

__declspec(dllexport) MCIERROR WINAPI Proxy_mciSendStringW(LPCWSTR lpstrCommand, LPWSTR lpstrReturnString, UINT uReturnLength, HWND hwndCallback) {
    if (!g_pfnMciSendStringW) LoadOriginalWinMM();
    if (g_pfnMciSendStringW) return ((MCIERROR(WINAPI*)(LPCWSTR, LPWSTR, UINT, HWND))g_pfnMciSendStringW)(lpstrCommand, lpstrReturnString, uReturnLength, hwndCallback);
    return MCIERR_UNRECOGNIZED_COMMAND;
}

__declspec(dllexport) BOOL WINAPI Proxy_mciGetErrorStringA(MCIERROR mcierr, LPSTR pszText, UINT cchText) {
    if (!g_pfnMciGetErrorStringA) LoadOriginalWinMM();
    if (g_pfnMciGetErrorStringA) return ((BOOL(WINAPI*)(MCIERROR, LPSTR, UINT))g_pfnMciGetErrorStringA)(mcierr, pszText, cchText);
    return FALSE;
}

__declspec(dllexport) BOOL WINAPI Proxy_mciGetErrorStringW(MCIERROR mcierr, LPWSTR pszText, UINT cchText) {
    if (!g_pfnMciGetErrorStringW) LoadOriginalWinMM();
    if (g_pfnMciGetErrorStringW) return ((BOOL(WINAPI*)(MCIERROR, LPWSTR, UINT))g_pfnMciGetErrorStringW)(mcierr, pszText, cchText);
    return FALSE;
}

} // extern "C"