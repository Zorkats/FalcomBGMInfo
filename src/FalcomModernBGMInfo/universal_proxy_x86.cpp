#include "universal_proxy_x86.h"
#include <cstdio>
#include <cstring>
#include <shlwapi.h>

HMODULE g_hOriginalDll = nullptr;
enum class ProxyType { None, XInput, Version, DInput8, WinMM };
ProxyType g_ProxyType = ProxyType::None;

// Function Pointers - Defined
extern "C" {
    // DInput8
    FARPROC g_pfnDirectInput8Create = nullptr;
    FARPROC g_pfnDllCanUnloadNow = nullptr;
    FARPROC g_pfnDllGetClassObject = nullptr;
    FARPROC g_pfnDllRegisterServer = nullptr;
    FARPROC g_pfnDllUnregisterServer = nullptr;

    // Version
    FARPROC g_pfnGetFileVersionInfoA = nullptr;
    FARPROC g_pfnGetFileVersionInfoByHandle = nullptr;
    FARPROC g_pfnGetFileVersionInfoExA = nullptr;
    FARPROC g_pfnGetFileVersionInfoExW = nullptr;
    FARPROC g_pfnGetFileVersionInfoSizeA = nullptr;
    FARPROC g_pfnGetFileVersionInfoSizeExA = nullptr;
    FARPROC g_pfnGetFileVersionInfoSizeExW = nullptr;
    FARPROC g_pfnGetFileVersionInfoSizeW = nullptr;
    FARPROC g_pfnGetFileVersionInfoW = nullptr;
    FARPROC g_pfnVerFindFileA = nullptr;
    FARPROC g_pfnVerFindFileW = nullptr;
    FARPROC g_pfnVerInstallFileA = nullptr;
    FARPROC g_pfnVerInstallFileW = nullptr;
    FARPROC g_pfnVerLanguageNameA = nullptr;
    FARPROC g_pfnVerLanguageNameW = nullptr;
    FARPROC g_pfnVerQueryValueA = nullptr;
    FARPROC g_pfnVerQueryValueW = nullptr;

    // XInput
    FARPROC g_pfnXInputEnable = nullptr;
    FARPROC g_pfnXInputGetAudioDeviceIds = nullptr;
    FARPROC g_pfnXInputGetBaseBusInformation = nullptr;
    FARPROC g_pfnXInputGetBatteryInformation = nullptr;
    FARPROC g_pfnXInputGetCapabilities = nullptr;
    FARPROC g_pfnXInputGetCapabilitiesEx = nullptr;
    FARPROC g_pfnXInputGetDSoundAudioDeviceGuids = nullptr;
    FARPROC g_pfnXInputGetKeystroke = nullptr;
    FARPROC g_pfnXInputGetState = nullptr;
    FARPROC g_pfnXInputGetStateEx = nullptr;
    FARPROC g_pfnXInputSetState = nullptr;
    FARPROC g_pfnXInputWaitForGuideButton = nullptr;
    FARPROC g_pfnXInputCancelGuideButtonWait = nullptr;
    FARPROC g_pfnXInputPowerOffController = nullptr;

    // WinMM
    FARPROC g_pfnCloseDriver = nullptr;
    FARPROC g_pfnDefDriverProc = nullptr;
    FARPROC g_pfnDriverCallback = nullptr;
    FARPROC g_pfnDrvGetModuleHandle = nullptr;
    FARPROC g_pfnGetDriverModuleHandle = nullptr;
    FARPROC g_pfnNotifyCallbackData = nullptr;
    FARPROC g_pfnOpenDriver = nullptr;
    FARPROC g_pfnPlaySound = nullptr;
    FARPROC g_pfnPlaySoundA = nullptr;
    FARPROC g_pfnPlaySoundW = nullptr;
    FARPROC g_pfnSendDriverMessage = nullptr;
    FARPROC g_pfnWOW32DriverCallback = nullptr;
    FARPROC g_pfnWOW32ResolveMultiMediaHandle = nullptr;
    FARPROC g_pfnWOWAppExit = nullptr;
    FARPROC g_pfnaux32Message = nullptr;
    FARPROC g_pfnauxGetDevCapsA = nullptr;
    FARPROC g_pfnauxGetDevCapsW = nullptr;
    FARPROC g_pfnauxGetNumDevs = nullptr;
    FARPROC g_pfnauxGetVolume = nullptr;
    FARPROC g_pfnauxOutMessage = nullptr;
    FARPROC g_pfnauxSetVolume = nullptr;
    FARPROC g_pfnjoy32Message = nullptr;
    FARPROC g_pfnjoyConfigChanged = nullptr;
    FARPROC g_pfnjoyGetDevCapsA = nullptr;
    FARPROC g_pfnjoyGetDevCapsW = nullptr;
    FARPROC g_pfnjoyGetNumDevs = nullptr;
    FARPROC g_pfnjoyGetPos = nullptr;
    FARPROC g_pfnjoyGetPosEx = nullptr;
    FARPROC g_pfnjoyGetThreshold = nullptr;
    FARPROC g_pfnjoyReleaseCapture = nullptr;
    FARPROC g_pfnjoySetCapture = nullptr;
    FARPROC g_pfnjoySetThreshold = nullptr;
    FARPROC g_pfnmci32Message = nullptr;
    FARPROC g_pfnmciDriverNotify = nullptr;
    FARPROC g_pfnmciDriverYield = nullptr;
    FARPROC g_pfnmciExecute = nullptr;
    FARPROC g_pfnmciFreeCommandResource = nullptr;
    FARPROC g_pfnmciGetCreatorTask = nullptr;
    FARPROC g_pfnmciGetDeviceIDA = nullptr;
    FARPROC g_pfnmciGetDeviceIDFromElementIDA = nullptr;
    FARPROC g_pfnmciGetDeviceIDFromElementIDW = nullptr;
    FARPROC g_pfnmciGetDeviceIDW = nullptr;
    FARPROC g_pfnmciGetDriverData = nullptr;
    FARPROC g_pfnmciGetErrorStringA = nullptr;
    FARPROC g_pfnmciGetErrorStringW = nullptr;
    FARPROC g_pfnmciGetYieldProc = nullptr;
    FARPROC g_pfnmciLoadCommandResource = nullptr;
    FARPROC g_pfnmciSendCommandA = nullptr;
    FARPROC g_pfnmciSendCommandW = nullptr;
    FARPROC g_pfnmciSendStringA = nullptr;
    FARPROC g_pfnmciSendStringW = nullptr;
    FARPROC g_pfnmciSetDriverData = nullptr;
    FARPROC g_pfnmciSetYieldProc = nullptr;
    FARPROC g_pfnmid32Message = nullptr;
    FARPROC g_pfnmidiConnect = nullptr;
    FARPROC g_pfnmidiDisconnect = nullptr;
    FARPROC g_pfnmidiInAddBuffer = nullptr;
    FARPROC g_pfnmidiInClose = nullptr;
    FARPROC g_pfnmidiInGetDevCapsA = nullptr;
    FARPROC g_pfnmidiInGetDevCapsW = nullptr;
    FARPROC g_pfnmidiInGetErrorTextA = nullptr;
    FARPROC g_pfnmidiInGetErrorTextW = nullptr;
    FARPROC g_pfnmidiInGetID = nullptr;
    FARPROC g_pfnmidiInGetNumDevs = nullptr;
    FARPROC g_pfnmidiInMessage = nullptr;
    FARPROC g_pfnmidiInOpen = nullptr;
    FARPROC g_pfnmidiInPrepareHeader = nullptr;
    FARPROC g_pfnmidiInReset = nullptr;
    FARPROC g_pfnmidiInStart = nullptr;
    FARPROC g_pfnmidiInStop = nullptr;
    FARPROC g_pfnmidiInUnprepareHeader = nullptr;
    FARPROC g_pfnmidiOutCacheDrumPatches = nullptr;
    FARPROC g_pfnmidiOutCachePatches = nullptr;
    FARPROC g_pfnmidiOutClose = nullptr;
    FARPROC g_pfnmidiOutGetDevCapsA = nullptr;
    FARPROC g_pfnmidiOutGetDevCapsW = nullptr;
    FARPROC g_pfnmidiOutGetErrorTextA = nullptr;
    FARPROC g_pfnmidiOutGetErrorTextW = nullptr;
    FARPROC g_pfnmidiOutGetID = nullptr;
    FARPROC g_pfnmidiOutGetNumDevs = nullptr;
    FARPROC g_pfnmidiOutGetVolume = nullptr;
    FARPROC g_pfnmidiOutLongMsg = nullptr;
    FARPROC g_pfnmidiOutMessage = nullptr;
    FARPROC g_pfnmidiOutOpen = nullptr;
    FARPROC g_pfnmidiOutPrepareHeader = nullptr;
    FARPROC g_pfnmidiOutReset = nullptr;
    FARPROC g_pfnmidiOutSetVolume = nullptr;
    FARPROC g_pfnmidiOutShortMsg = nullptr;
    FARPROC g_pfnmidiOutUnprepareHeader = nullptr;
    FARPROC g_pfnmidiStreamClose = nullptr;
    FARPROC g_pfnmidiStreamOpen = nullptr;
    FARPROC g_pfnmidiStreamOut = nullptr;
    FARPROC g_pfnmidiStreamPause = nullptr;
    FARPROC g_pfnmidiStreamPosition = nullptr;
    FARPROC g_pfnmidiStreamProperty = nullptr;
    FARPROC g_pfnmidiStreamRestart = nullptr;
    FARPROC g_pfnmidiStreamStop = nullptr;
    FARPROC g_pfnmixerClose = nullptr;
    FARPROC g_pfnmixerGetControlDetailsA = nullptr;
    FARPROC g_pfnmixerGetControlDetailsW = nullptr;
    FARPROC g_pfnmixerGetDevCapsA = nullptr;
    FARPROC g_pfnmixerGetDevCapsW = nullptr;
    FARPROC g_pfnmixerGetID = nullptr;
    FARPROC g_pfnmixerGetLineControlsA = nullptr;
    FARPROC g_pfnmixerGetLineControlsW = nullptr;
    FARPROC g_pfnmixerGetLineInfoA = nullptr;
    FARPROC g_pfnmixerGetLineInfoW = nullptr;
    FARPROC g_pfnmixerGetNumDevs = nullptr;
    FARPROC g_pfnmixerMessage = nullptr;
    FARPROC g_pfnmixerOpen = nullptr;
    FARPROC g_pfnmixerSetControlDetails = nullptr;
    FARPROC g_pfnmmDrvInstall = nullptr;
    FARPROC g_pfnmmGetCurrentTask = nullptr;
    FARPROC g_pfnmmTaskBlock = nullptr;
    FARPROC g_pfnmmTaskCreate = nullptr;
    FARPROC g_pfnmmTaskSignal = nullptr;
    FARPROC g_pfnmmTaskYield = nullptr;
    FARPROC g_pfnmmioAdvance = nullptr;
    FARPROC g_pfnmmioAscend = nullptr;
    FARPROC g_pfnmmioClose = nullptr;
    FARPROC g_pfnmmioCreateChunk = nullptr;
    FARPROC g_pfnmmioDescend = nullptr;
    FARPROC g_pfnmmioFlush = nullptr;
    FARPROC g_pfnmmioGetInfo = nullptr;
    FARPROC g_pfnmmioInstallIOProcA = nullptr;
    FARPROC g_pfnmmioInstallIOProcW = nullptr;
    FARPROC g_pfnmmioOpenA = nullptr;
    FARPROC g_pfnmmioOpenW = nullptr;
    FARPROC g_pfnmmioRead = nullptr;
    FARPROC g_pfnmmioRenameA = nullptr;
    FARPROC g_pfnmmioRenameW = nullptr;
    FARPROC g_pfnmmioSeek = nullptr;
    FARPROC g_pfnmmioSendMessage = nullptr;
    FARPROC g_pfnmmioSetBuffer = nullptr;
    FARPROC g_pfnmmioSetInfo = nullptr;
    FARPROC g_pfnmmioStringToFOURCCA = nullptr;
    FARPROC g_pfnmmioStringToFOURCCW = nullptr;
    FARPROC g_pfnmmioWrite = nullptr;
    FARPROC g_pfnmmsystemGetVersion = nullptr;
    FARPROC g_pfnmod32Message = nullptr;
    FARPROC g_pfnmxd32Message = nullptr;
    FARPROC g_pfnsndPlaySoundA = nullptr;
    FARPROC g_pfnsndPlaySoundW = nullptr;
    FARPROC g_pfntid32Message = nullptr;
    FARPROC g_pfntimeBeginPeriod = nullptr;
    FARPROC g_pfntimeEndPeriod = nullptr;
    FARPROC g_pfntimeGetDevCaps = nullptr;
    FARPROC g_pfntimeGetSystemTime = nullptr;
    FARPROC g_pfntimeGetTime = nullptr;
    FARPROC g_pfntimeKillEvent = nullptr;
    FARPROC g_pfntimeSetEvent = nullptr;
    FARPROC g_pfnwaveInAddBuffer = nullptr;
    FARPROC g_pfnwaveInClose = nullptr;
    FARPROC g_pfnwaveInGetDevCapsA = nullptr;
    FARPROC g_pfnwaveInGetDevCapsW = nullptr;
    FARPROC g_pfnwaveInGetErrorTextA = nullptr;
    FARPROC g_pfnwaveInGetErrorTextW = nullptr;
    FARPROC g_pfnwaveInGetID = nullptr;
    FARPROC g_pfnwaveInGetNumDevs = nullptr;
    FARPROC g_pfnwaveInGetPosition = nullptr;
    FARPROC g_pfnwaveInMessage = nullptr;
    FARPROC g_pfnwaveInOpen = nullptr;
    FARPROC g_pfnwaveInPrepareHeader = nullptr;
    FARPROC g_pfnwaveInReset = nullptr;
    FARPROC g_pfnwaveInStart = nullptr;
    FARPROC g_pfnwaveInStop = nullptr;
    FARPROC g_pfnwaveInUnprepareHeader = nullptr;
    FARPROC g_pfnwaveOutBreakLoop = nullptr;
    FARPROC g_pfnwaveOutClose = nullptr;
    FARPROC g_pfnwaveOutGetDevCapsA = nullptr;
    FARPROC g_pfnwaveOutGetDevCapsW = nullptr;
    FARPROC g_pfnwaveOutGetErrorTextA = nullptr;
    FARPROC g_pfnwaveOutGetErrorTextW = nullptr;
    FARPROC g_pfnwaveOutGetID = nullptr;
    FARPROC g_pfnwaveOutGetNumDevs = nullptr;
    FARPROC g_pfnwaveOutGetPitch = nullptr;
    FARPROC g_pfnwaveOutGetPlaybackRate = nullptr;
    FARPROC g_pfnwaveOutGetPosition = nullptr;
    FARPROC g_pfnwaveOutGetVolume = nullptr;
    FARPROC g_pfnwaveOutMessage = nullptr;
    FARPROC g_pfnwaveOutOpen = nullptr;
    FARPROC g_pfnwaveOutPause = nullptr;
    FARPROC g_pfnwaveOutPrepareHeader = nullptr;
    FARPROC g_pfnwaveOutReset = nullptr;
    FARPROC g_pfnwaveOutRestart = nullptr;
    FARPROC g_pfnwaveOutSetPitch = nullptr;
    FARPROC g_pfnwaveOutSetPlaybackRate = nullptr;
    FARPROC g_pfnwaveOutSetVolume = nullptr;
    FARPROC g_pfnwaveOutUnprepareHeader = nullptr;
    FARPROC g_pfnwaveOutWrite = nullptr;
    FARPROC g_pfnwid32Message = nullptr;
    FARPROC g_pfnwod32Message = nullptr;
}

// Function to detect proxy type and load original DLL
void DetectProxyType(HMODULE hModule) {
    char dllPath[MAX_PATH];
    GetModuleFileNameA(hModule, dllPath, MAX_PATH);
    char* filename = strrchr(dllPath, '\\');
    if (filename) filename++; else filename = dllPath;
    char lower[MAX_PATH];
    strcpy_s(lower, filename);
    _strlwr_s(lower);

    if (strstr(lower, "xinput")) g_ProxyType = ProxyType::XInput;
    else if (strstr(lower, "version")) g_ProxyType = ProxyType::Version;
    else if (strstr(lower, "dinput8")) g_ProxyType = ProxyType::DInput8;
    else if (strstr(lower, "winmm")) g_ProxyType = ProxyType::WinMM;
    else g_ProxyType = ProxyType::WinMM; // Default

    LoadOriginalDll();
}

void LoadOriginalDll() {
    if (g_hOriginalDll) return;

    char systemPath[MAX_PATH];
    GetSystemDirectoryA(systemPath, MAX_PATH);
    char path[MAX_PATH];

    switch(g_ProxyType) {
        case ProxyType::XInput:
            // Try newest to oldest XInput
            sprintf_s(path, "%s\\xinput1_4.dll", systemPath);
            g_hOriginalDll = LoadLibraryA(path);
            if (!g_hOriginalDll) {
                sprintf_s(path, "%s\\xinput1_3.dll", systemPath);
                g_hOriginalDll = LoadLibraryA(path);
            }
            if (!g_hOriginalDll) {
                sprintf_s(path, "%s\\xinput9_1_0.dll", systemPath);
                g_hOriginalDll = LoadLibraryA(path);
            }
            break;
        case ProxyType::Version:
            sprintf_s(path, "%s\\version.dll", systemPath);
            g_hOriginalDll = LoadLibraryA(path);
            break;
        case ProxyType::DInput8:
            sprintf_s(path, "%s\\dinput8.dll", systemPath);
            g_hOriginalDll = LoadLibraryA(path);
            break;
        case ProxyType::WinMM:
        default:
            sprintf_s(path, "%s\\winmm.dll", systemPath);
            g_hOriginalDll = LoadLibraryA(path);
            break;
    }

    if (!g_hOriginalDll) return;

    // Macro to load address. If missing in this DLL, g_pfn... remains nullptr.
    #define LOAD_PROC(x) g_pfn##x = GetProcAddress(g_hOriginalDll, #x)

    if (g_ProxyType == ProxyType::DInput8) {
        LOAD_PROC(DirectInput8Create);
        LOAD_PROC(DllCanUnloadNow);
        LOAD_PROC(DllGetClassObject);
        LOAD_PROC(DllRegisterServer);
        LOAD_PROC(DllUnregisterServer);
    }
    else if (g_ProxyType == ProxyType::Version) {
        LOAD_PROC(GetFileVersionInfoA);
        LOAD_PROC(GetFileVersionInfoByHandle);
        LOAD_PROC(GetFileVersionInfoExA);
        LOAD_PROC(GetFileVersionInfoExW);
        LOAD_PROC(GetFileVersionInfoSizeA);
        LOAD_PROC(GetFileVersionInfoSizeExA);
        LOAD_PROC(GetFileVersionInfoSizeExW);
        LOAD_PROC(GetFileVersionInfoSizeW);
        LOAD_PROC(GetFileVersionInfoW);
        LOAD_PROC(VerFindFileA);
        LOAD_PROC(VerFindFileW);
        LOAD_PROC(VerInstallFileA);
        LOAD_PROC(VerInstallFileW);
        LOAD_PROC(VerLanguageNameA);
        LOAD_PROC(VerLanguageNameW);
        LOAD_PROC(VerQueryValueA);
        LOAD_PROC(VerQueryValueW);
    }
    else if (g_ProxyType == ProxyType::XInput) {
        LOAD_PROC(XInputEnable);
        LOAD_PROC(XInputGetAudioDeviceIds);
        LOAD_PROC(XInputGetBaseBusInformation);
        LOAD_PROC(XInputGetBatteryInformation);
        LOAD_PROC(XInputGetCapabilities);
        LOAD_PROC(XInputGetCapabilitiesEx);
        LOAD_PROC(XInputGetDSoundAudioDeviceGuids);
        LOAD_PROC(XInputGetKeystroke);
        LOAD_PROC(XInputGetState);
        // Special case for ordinal 100 in 1.3
        if (!g_pfnXInputGetState) g_pfnXInputGetState = GetProcAddress(g_hOriginalDll, (LPCSTR)100);

        LOAD_PROC(XInputSetState);

        // Load hidden/ordinal exports if names fail
        if (!g_pfnXInputGetStateEx) g_pfnXInputGetStateEx = GetProcAddress(g_hOriginalDll, (LPCSTR)100);
        if (!g_pfnXInputWaitForGuideButton) g_pfnXInputWaitForGuideButton = GetProcAddress(g_hOriginalDll, (LPCSTR)101);
        if (!g_pfnXInputCancelGuideButtonWait) g_pfnXInputCancelGuideButtonWait = GetProcAddress(g_hOriginalDll, (LPCSTR)102);
        if (!g_pfnXInputPowerOffController) g_pfnXInputPowerOffController = GetProcAddress(g_hOriginalDll, (LPCSTR)103);
    }
    else { // WinMM
        LOAD_PROC(CloseDriver);
        LOAD_PROC(DefDriverProc);
        LOAD_PROC(DriverCallback);
        LOAD_PROC(DrvGetModuleHandle);
        LOAD_PROC(GetDriverModuleHandle);
        LOAD_PROC(NotifyCallbackData);
        LOAD_PROC(OpenDriver);
        LOAD_PROC(PlaySound);
        LOAD_PROC(PlaySoundA);
        LOAD_PROC(PlaySoundW);
        LOAD_PROC(SendDriverMessage);
        LOAD_PROC(WOW32DriverCallback);
        LOAD_PROC(WOW32ResolveMultiMediaHandle);
        LOAD_PROC(WOWAppExit);
        LOAD_PROC(aux32Message);
        LOAD_PROC(auxGetDevCapsA);
        LOAD_PROC(auxGetDevCapsW);
        LOAD_PROC(auxGetNumDevs);
        LOAD_PROC(auxGetVolume);
        LOAD_PROC(auxOutMessage);
        LOAD_PROC(auxSetVolume);
        LOAD_PROC(joy32Message);
        LOAD_PROC(joyConfigChanged);
        LOAD_PROC(joyGetDevCapsA);
        LOAD_PROC(joyGetDevCapsW);
        LOAD_PROC(joyGetNumDevs);
        LOAD_PROC(joyGetPos);
        LOAD_PROC(joyGetPosEx);
        LOAD_PROC(joyGetThreshold);
        LOAD_PROC(joyReleaseCapture);
        LOAD_PROC(joySetCapture);
        LOAD_PROC(joySetThreshold);
        LOAD_PROC(mci32Message);
        LOAD_PROC(mciDriverNotify);
        LOAD_PROC(mciDriverYield);
        LOAD_PROC(mciExecute);
        LOAD_PROC(mciFreeCommandResource);
        LOAD_PROC(mciGetCreatorTask);
        LOAD_PROC(mciGetDeviceIDA);
        LOAD_PROC(mciGetDeviceIDFromElementIDA);
        LOAD_PROC(mciGetDeviceIDFromElementIDW);
        LOAD_PROC(mciGetDeviceIDW);
        LOAD_PROC(mciGetDriverData);
        LOAD_PROC(mciGetErrorStringA);
        LOAD_PROC(mciGetErrorStringW);
        LOAD_PROC(mciGetYieldProc);
        LOAD_PROC(mciLoadCommandResource);
        LOAD_PROC(mciSendCommandA);
        LOAD_PROC(mciSendCommandW);
        LOAD_PROC(mciSendStringA);
        LOAD_PROC(mciSendStringW);
        LOAD_PROC(mciSetDriverData);
        LOAD_PROC(mciSetYieldProc);
        LOAD_PROC(mid32Message);
        LOAD_PROC(midiConnect);
        LOAD_PROC(midiDisconnect);
        LOAD_PROC(midiInAddBuffer);
        LOAD_PROC(midiInClose);
        LOAD_PROC(midiInGetDevCapsA);
        LOAD_PROC(midiInGetDevCapsW);
        LOAD_PROC(midiInGetErrorTextA);
        LOAD_PROC(midiInGetErrorTextW);
        LOAD_PROC(midiInGetID);
        LOAD_PROC(midiInGetNumDevs);
        LOAD_PROC(midiInMessage);
        LOAD_PROC(midiInOpen);
        LOAD_PROC(midiInPrepareHeader);
        LOAD_PROC(midiInReset);
        LOAD_PROC(midiInStart);
        LOAD_PROC(midiInStop);
        LOAD_PROC(midiInUnprepareHeader);
        LOAD_PROC(midiOutCacheDrumPatches);
        LOAD_PROC(midiOutCachePatches);
        LOAD_PROC(midiOutClose);
        LOAD_PROC(midiOutGetDevCapsA);
        LOAD_PROC(midiOutGetDevCapsW);
        LOAD_PROC(midiOutGetErrorTextA);
        LOAD_PROC(midiOutGetErrorTextW);
        LOAD_PROC(midiOutGetID);
        LOAD_PROC(midiOutGetNumDevs);
        LOAD_PROC(midiOutGetVolume);
        LOAD_PROC(midiOutLongMsg);
        LOAD_PROC(midiOutMessage);
        LOAD_PROC(midiOutOpen);
        LOAD_PROC(midiOutPrepareHeader);
        LOAD_PROC(midiOutReset);
        LOAD_PROC(midiOutSetVolume);
        LOAD_PROC(midiOutShortMsg);
        LOAD_PROC(midiOutUnprepareHeader);
        LOAD_PROC(midiStreamClose);
        LOAD_PROC(midiStreamOpen);
        LOAD_PROC(midiStreamOut);
        LOAD_PROC(midiStreamPause);
        LOAD_PROC(midiStreamPosition);
        LOAD_PROC(midiStreamProperty);
        LOAD_PROC(midiStreamRestart);
        LOAD_PROC(midiStreamStop);
        LOAD_PROC(mixerClose);
        LOAD_PROC(mixerGetControlDetailsA);
        LOAD_PROC(mixerGetControlDetailsW);
        LOAD_PROC(mixerGetDevCapsA);
        LOAD_PROC(mixerGetDevCapsW);
        LOAD_PROC(mixerGetID);
        LOAD_PROC(mixerGetLineControlsA);
        LOAD_PROC(mixerGetLineControlsW);
        LOAD_PROC(mixerGetLineInfoA);
        LOAD_PROC(mixerGetLineInfoW);
        LOAD_PROC(mixerGetNumDevs);
        LOAD_PROC(mixerMessage);
        LOAD_PROC(mixerOpen);
        LOAD_PROC(mixerSetControlDetails);
        LOAD_PROC(mmDrvInstall);
        LOAD_PROC(mmGetCurrentTask);
        LOAD_PROC(mmTaskBlock);
        LOAD_PROC(mmTaskCreate);
        LOAD_PROC(mmTaskSignal);
        LOAD_PROC(mmTaskYield);
        LOAD_PROC(mmioAdvance);
        LOAD_PROC(mmioAscend);
        LOAD_PROC(mmioClose);
        LOAD_PROC(mmioCreateChunk);
        LOAD_PROC(mmioDescend);
        LOAD_PROC(mmioFlush);
        LOAD_PROC(mmioGetInfo);
        LOAD_PROC(mmioInstallIOProcA);
        LOAD_PROC(mmioInstallIOProcW);
        LOAD_PROC(mmioOpenA);
        LOAD_PROC(mmioOpenW);
        LOAD_PROC(mmioRead);
        LOAD_PROC(mmioRenameA);
        LOAD_PROC(mmioRenameW);
        LOAD_PROC(mmioSeek);
        LOAD_PROC(mmioSendMessage);
        LOAD_PROC(mmioSetBuffer);
        LOAD_PROC(mmioSetInfo);
        LOAD_PROC(mmioStringToFOURCCA);
        LOAD_PROC(mmioStringToFOURCCW);
        LOAD_PROC(mmioWrite);
        LOAD_PROC(mmsystemGetVersion);
        LOAD_PROC(mod32Message);
        LOAD_PROC(mxd32Message);
        LOAD_PROC(sndPlaySoundA);
        LOAD_PROC(sndPlaySoundW);
        LOAD_PROC(tid32Message);
        LOAD_PROC(timeBeginPeriod);
        LOAD_PROC(timeEndPeriod);
        LOAD_PROC(timeGetDevCaps);
        LOAD_PROC(timeGetSystemTime);
        LOAD_PROC(timeGetTime);
        LOAD_PROC(timeKillEvent);
        LOAD_PROC(timeSetEvent);
        LOAD_PROC(waveInAddBuffer);
        LOAD_PROC(waveInClose);
        LOAD_PROC(waveInGetDevCapsA);
        LOAD_PROC(waveInGetDevCapsW);
        LOAD_PROC(waveInGetErrorTextA);
        LOAD_PROC(waveInGetErrorTextW);
        LOAD_PROC(waveInGetID);
        LOAD_PROC(waveInGetNumDevs);
        LOAD_PROC(waveInGetPosition);
        LOAD_PROC(waveInMessage);
        LOAD_PROC(waveInOpen);
        LOAD_PROC(waveInPrepareHeader);
        LOAD_PROC(waveInReset);
        LOAD_PROC(waveInStart);
        LOAD_PROC(waveInStop);
        LOAD_PROC(waveInUnprepareHeader);
        LOAD_PROC(waveOutBreakLoop);
        LOAD_PROC(waveOutClose);
        LOAD_PROC(waveOutGetDevCapsA);
        LOAD_PROC(waveOutGetDevCapsW);
        LOAD_PROC(waveOutGetErrorTextA);
        LOAD_PROC(waveOutGetErrorTextW);
        LOAD_PROC(waveOutGetID);
        LOAD_PROC(waveOutGetNumDevs);
        LOAD_PROC(waveOutGetPitch);
        LOAD_PROC(waveOutGetPlaybackRate);
        LOAD_PROC(waveOutGetPosition);
        LOAD_PROC(waveOutGetVolume);
        LOAD_PROC(waveOutMessage);
        LOAD_PROC(waveOutOpen);
        LOAD_PROC(waveOutPause);
        LOAD_PROC(waveOutPrepareHeader);
        LOAD_PROC(waveOutReset);
        LOAD_PROC(waveOutRestart);
        LOAD_PROC(waveOutSetPitch);
        LOAD_PROC(waveOutSetPlaybackRate);
        LOAD_PROC(waveOutSetVolume);
        LOAD_PROC(waveOutUnprepareHeader);
        LOAD_PROC(waveOutWrite);
        LOAD_PROC(wid32Message);
        LOAD_PROC(wod32Message);
    }
}

// Macro for stub functions
#define PROXY(name) \
    __declspec(naked) void Proxy_##name() { \
        __asm { jmp [g_pfn##name] } \
    }

// DInput8
PROXY(DirectInput8Create)
PROXY(DllCanUnloadNow)
PROXY(DllGetClassObject)
PROXY(DllRegisterServer)
PROXY(DllUnregisterServer)

// Version
PROXY(GetFileVersionInfoA)
PROXY(GetFileVersionInfoByHandle)
PROXY(GetFileVersionInfoExA)
PROXY(GetFileVersionInfoExW)
PROXY(GetFileVersionInfoSizeA)
PROXY(GetFileVersionInfoSizeExA)
PROXY(GetFileVersionInfoSizeExW)
PROXY(GetFileVersionInfoSizeW)
PROXY(GetFileVersionInfoW)
PROXY(VerFindFileA)
PROXY(VerFindFileW)
PROXY(VerInstallFileA)
PROXY(VerInstallFileW)
PROXY(VerLanguageNameA)
PROXY(VerLanguageNameW)
PROXY(VerQueryValueA)
PROXY(VerQueryValueW)

// XInput
PROXY(XInputEnable)
PROXY(XInputGetAudioDeviceIds)
PROXY(XInputGetBaseBusInformation)
PROXY(XInputGetBatteryInformation)
PROXY(XInputGetCapabilities)
PROXY(XInputGetCapabilitiesEx)
PROXY(XInputGetDSoundAudioDeviceGuids)
PROXY(XInputGetKeystroke)
PROXY(XInputGetState)
PROXY(XInputGetStateEx)
PROXY(XInputSetState)
PROXY(XInputWaitForGuideButton)
PROXY(XInputCancelGuideButtonWait)
PROXY(XInputPowerOffController)

// WinMM
PROXY(CloseDriver)
PROXY(DefDriverProc)
PROXY(DriverCallback)
PROXY(DrvGetModuleHandle)
PROXY(GetDriverModuleHandle)
PROXY(NotifyCallbackData)
PROXY(OpenDriver)
PROXY(PlaySound)
PROXY(PlaySoundA)
PROXY(PlaySoundW)
PROXY(SendDriverMessage)
PROXY(WOW32DriverCallback)
PROXY(WOW32ResolveMultiMediaHandle)
PROXY(WOWAppExit)
PROXY(aux32Message)
PROXY(auxGetDevCapsA)
PROXY(auxGetDevCapsW)
PROXY(auxGetNumDevs)
PROXY(auxGetVolume)
PROXY(auxOutMessage)
PROXY(auxSetVolume)
PROXY(joy32Message)
PROXY(joyConfigChanged)
PROXY(joyGetDevCapsA)
PROXY(joyGetDevCapsW)
PROXY(joyGetNumDevs)
PROXY(joyGetPos)
PROXY(joyGetPosEx)
PROXY(joyGetThreshold)
PROXY(joyReleaseCapture)
PROXY(joySetCapture)
PROXY(joySetThreshold)
PROXY(mci32Message)
PROXY(mciDriverNotify)
PROXY(mciDriverYield)
PROXY(mciExecute)
PROXY(mciFreeCommandResource)
PROXY(mciGetCreatorTask)
PROXY(mciGetDeviceIDA)
PROXY(mciGetDeviceIDFromElementIDA)
PROXY(mciGetDeviceIDFromElementIDW)
PROXY(mciGetDeviceIDW)
PROXY(mciGetDriverData)
PROXY(mciGetErrorStringA)
PROXY(mciGetErrorStringW)
PROXY(mciGetYieldProc)
PROXY(mciLoadCommandResource)
PROXY(mciSendCommandA)
PROXY(mciSendCommandW)
PROXY(mciSendStringA)
PROXY(mciSendStringW)
PROXY(mciSetDriverData)
PROXY(mciSetYieldProc)
PROXY(mid32Message)
PROXY(midiConnect)
PROXY(midiDisconnect)
PROXY(midiInAddBuffer)
PROXY(midiInClose)
PROXY(midiInGetDevCapsA)
PROXY(midiInGetDevCapsW)
PROXY(midiInGetErrorTextA)
PROXY(midiInGetErrorTextW)
PROXY(midiInGetID)
PROXY(midiInGetNumDevs)
PROXY(midiInMessage)
PROXY(midiInOpen)
PROXY(midiInPrepareHeader)
PROXY(midiInReset)
PROXY(midiInStart)
PROXY(midiInStop)
PROXY(midiInUnprepareHeader)
PROXY(midiOutCacheDrumPatches)
PROXY(midiOutCachePatches)
PROXY(midiOutClose)
PROXY(midiOutGetDevCapsA)
PROXY(midiOutGetDevCapsW)
PROXY(midiOutGetErrorTextA)
PROXY(midiOutGetErrorTextW)
PROXY(midiOutGetID)
PROXY(midiOutGetNumDevs)
PROXY(midiOutGetVolume)
PROXY(midiOutLongMsg)
PROXY(midiOutMessage)
PROXY(midiOutOpen)
PROXY(midiOutPrepareHeader)
PROXY(midiOutReset)
PROXY(midiOutSetVolume)
PROXY(midiOutShortMsg)
PROXY(midiOutUnprepareHeader)
PROXY(midiStreamClose)
PROXY(midiStreamOpen)
PROXY(midiStreamOut)
PROXY(midiStreamPause)
PROXY(midiStreamPosition)
PROXY(midiStreamProperty)
PROXY(midiStreamRestart)
PROXY(midiStreamStop)
PROXY(mixerClose)
PROXY(mixerGetControlDetailsA)
PROXY(mixerGetControlDetailsW)
PROXY(mixerGetDevCapsA)
PROXY(mixerGetDevCapsW)
PROXY(mixerGetID)
PROXY(mixerGetLineControlsA)
PROXY(mixerGetLineControlsW)
PROXY(mixerGetLineInfoA)
PROXY(mixerGetLineInfoW)
PROXY(mixerGetNumDevs)
PROXY(mixerMessage)
PROXY(mixerOpen)
PROXY(mixerSetControlDetails)
PROXY(mmDrvInstall)
PROXY(mmGetCurrentTask)
PROXY(mmTaskBlock)
PROXY(mmTaskCreate)
PROXY(mmTaskSignal)
PROXY(mmTaskYield)
PROXY(mmioAdvance)
PROXY(mmioAscend)
PROXY(mmioClose)
PROXY(mmioCreateChunk)
PROXY(mmioDescend)
PROXY(mmioFlush)
PROXY(mmioGetInfo)
PROXY(mmioInstallIOProcA)
PROXY(mmioInstallIOProcW)
PROXY(mmioOpenA)
PROXY(mmioOpenW)
PROXY(mmioRead)
PROXY(mmioRenameA)
PROXY(mmioRenameW)
PROXY(mmioSeek)
PROXY(mmioSendMessage)
PROXY(mmioSetBuffer)
PROXY(mmioSetInfo)
PROXY(mmioStringToFOURCCA)
PROXY(mmioStringToFOURCCW)
PROXY(mmioWrite)
PROXY(mmsystemGetVersion)
PROXY(mod32Message)
PROXY(mxd32Message)
PROXY(sndPlaySoundA)
PROXY(sndPlaySoundW)
PROXY(tid32Message)
PROXY(timeBeginPeriod)
PROXY(timeEndPeriod)
PROXY(timeGetDevCaps)
PROXY(timeGetSystemTime)
PROXY(timeGetTime)
PROXY(timeKillEvent)
PROXY(timeSetEvent)
PROXY(waveInAddBuffer)
PROXY(waveInClose)
PROXY(waveInGetDevCapsA)
PROXY(waveInGetDevCapsW)
PROXY(waveInGetErrorTextA)
PROXY(waveInGetErrorTextW)
PROXY(waveInGetID)
PROXY(waveInGetNumDevs)
PROXY(waveInGetPosition)
PROXY(waveInMessage)
PROXY(waveInOpen)
PROXY(waveInPrepareHeader)
PROXY(waveInReset)
PROXY(waveInStart)
PROXY(waveInStop)
PROXY(waveInUnprepareHeader)
PROXY(waveOutBreakLoop)
PROXY(waveOutClose)
PROXY(waveOutGetDevCapsA)
PROXY(waveOutGetDevCapsW)
PROXY(waveOutGetErrorTextA)
PROXY(waveOutGetErrorTextW)
PROXY(waveOutGetID)
PROXY(waveOutGetNumDevs)
PROXY(waveOutGetPitch)
PROXY(waveOutGetPlaybackRate)
PROXY(waveOutGetPosition)
PROXY(waveOutGetVolume)
PROXY(waveOutMessage)
PROXY(waveOutOpen)
PROXY(waveOutPause)
PROXY(waveOutPrepareHeader)
PROXY(waveOutReset)
PROXY(waveOutRestart)
PROXY(waveOutSetPitch)
PROXY(waveOutSetPlaybackRate)
PROXY(waveOutSetVolume)
PROXY(waveOutUnprepareHeader)
PROXY(waveOutWrite)
PROXY(wid32Message)
PROXY(wod32Message)