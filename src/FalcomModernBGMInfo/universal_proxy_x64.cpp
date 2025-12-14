#include "universal_proxy_x64.h"
#include <cstdio>
#include <shlwapi.h>

// =============================================================
// GLOBAL VARIABLES (Definitions)
// =============================================================
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

// =============================================================
// LOADER FUNCTIONS
// =============================================================
void LoadOriginalDll() {
    if (g_hOriginalDll) return;

    char systemPath[MAX_PATH];
    GetSystemDirectoryA(systemPath, MAX_PATH);
    char path[MAX_PATH];

    switch(g_ProxyType) {
        case ProxyType::XInput:
            // Try newest to oldest
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
        if (!g_pfnXInputGetState) g_pfnXInputGetState = GetProcAddress(g_hOriginalDll, (LPCSTR)100);
        LOAD_PROC(XInputSetState);
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
    else g_ProxyType = ProxyType::WinMM;

    LoadOriginalDll();
}

// =============================================================
// C++ PROXY WRAPPERS (x64)
// =============================================================
#define PROXY_IMPL(RET, NAME, ARGS_DEF, ARGS_CALL, TYPE_CAST) \
    RET WINAPI Proxy_##NAME ARGS_DEF { \
        if (!g_pfn##NAME) LoadOriginalDll(); \
        if (!g_pfn##NAME) return (RET)0; \
        return ((RET(WINAPI*) TYPE_CAST)g_pfn##NAME) ARGS_CALL; \
    }

#define PROXY_VOID(NAME, ARGS_DEF, ARGS_CALL, TYPE_CAST) \
    void WINAPI Proxy_##NAME ARGS_DEF { \
        if (!g_pfn##NAME) LoadOriginalDll(); \
        if (!g_pfn##NAME) return; \
        ((void(WINAPI*) TYPE_CAST)g_pfn##NAME) ARGS_CALL; \
    }

// Dummies for XInput Ordinal 1 (DllMain is not callable)
void WINAPI Proxy_XInputDllMain(void* hinstDLL, DWORD fdwReason, void* lpvReserved) {
    // Do nothing, just return
}

// DINPUT8
PROXY_IMPL(HRESULT, DirectInput8Create, (HINSTANCE hinst, DWORD dwVersion, REFIID riidltf, LPVOID* ppvOut, void* punkOuter), (hinst, dwVersion, riidltf, ppvOut, punkOuter), (HINSTANCE, DWORD, REFIID, LPVOID*, void*))
PROXY_IMPL(HRESULT, DllCanUnloadNow, (), (), ())
PROXY_IMPL(HRESULT, DllGetClassObject, (REFCLSID rclsid, REFIID riid, LPVOID* ppv), (rclsid, riid, ppv), (REFCLSID, REFIID, LPVOID*))
PROXY_IMPL(HRESULT, DllRegisterServer, (), (), ())
PROXY_IMPL(HRESULT, DllUnregisterServer, (), (), ())

// VERSION
PROXY_IMPL(BOOL, GetFileVersionInfoA, (LPCSTR lptstrFilename, DWORD dwHandle, DWORD dwLen, LPVOID lpData), (lptstrFilename, dwHandle, dwLen, lpData), (LPCSTR, DWORD, DWORD, LPVOID))
PROXY_IMPL(BOOL, GetFileVersionInfoW, (LPCWSTR lptstrFilename, DWORD dwHandle, DWORD dwLen, LPVOID lpData), (lptstrFilename, dwHandle, dwLen, lpData), (LPCWSTR, DWORD, DWORD, LPVOID))
PROXY_IMPL(BOOL, GetFileVersionInfoByHandle, (int w, int x, int y, int z), (w, x, y, z), (int, int, int, int))
PROXY_IMPL(BOOL, GetFileVersionInfoExA, (DWORD dwFlags, LPCSTR lpwstrFilename, DWORD dwHandle, DWORD dwLen, LPVOID lpData), (dwFlags, lpwstrFilename, dwHandle, dwLen, lpData), (DWORD, LPCSTR, DWORD, DWORD, LPVOID))
PROXY_IMPL(BOOL, GetFileVersionInfoExW, (DWORD dwFlags, LPCWSTR lpwstrFilename, DWORD dwHandle, DWORD dwLen, LPVOID lpData), (dwFlags, lpwstrFilename, dwHandle, dwLen, lpData), (DWORD, LPCWSTR, DWORD, DWORD, LPVOID))
PROXY_IMPL(DWORD, GetFileVersionInfoSizeA, (LPCSTR lptstrFilename, LPDWORD lpdwHandle), (lptstrFilename, lpdwHandle), (LPCSTR, LPDWORD))
PROXY_IMPL(DWORD, GetFileVersionInfoSizeW, (LPCWSTR lptstrFilename, LPDWORD lpdwHandle), (lptstrFilename, lpdwHandle), (LPCWSTR, LPDWORD))
PROXY_IMPL(DWORD, GetFileVersionInfoSizeExA, (DWORD dwFlags, LPCSTR lpwstrFilename, LPDWORD lpdwHandle), (dwFlags, lpwstrFilename, lpdwHandle), (DWORD, LPCSTR, LPDWORD))
PROXY_IMPL(DWORD, GetFileVersionInfoSizeExW, (DWORD dwFlags, LPCWSTR lpwstrFilename, LPDWORD lpdwHandle), (dwFlags, lpwstrFilename, lpdwHandle), (DWORD, LPCWSTR, LPDWORD))
PROXY_IMPL(BOOL, VerQueryValueA, (LPCVOID pBlock, LPCSTR lpSubBlock, LPVOID* lplpBuffer, PUINT puLen), (pBlock, lpSubBlock, lplpBuffer, puLen), (LPCVOID, LPCSTR, LPVOID*, PUINT))
PROXY_IMPL(BOOL, VerQueryValueW, (LPCVOID pBlock, LPCWSTR lpSubBlock, LPVOID* lplpBuffer, PUINT puLen), (pBlock, lpSubBlock, lplpBuffer, puLen), (LPCVOID, LPCWSTR, LPVOID*, PUINT))
PROXY_IMPL(DWORD, VerFindFileA, (DWORD uFlags, LPCSTR szFileName, LPCSTR szWinDir, LPCSTR szAppDir, LPSTR szCurDir, PUINT puCurDirLen, LPSTR szDestDir, PUINT puDestDirLen), (uFlags, szFileName, szWinDir, szAppDir, szCurDir, puCurDirLen, szDestDir, puDestDirLen), (DWORD, LPCSTR, LPCSTR, LPCSTR, LPSTR, PUINT, LPSTR, PUINT))
PROXY_IMPL(DWORD, VerFindFileW, (DWORD uFlags, LPCWSTR szFileName, LPCWSTR szWinDir, LPCWSTR szAppDir, LPWSTR szCurDir, PUINT puCurDirLen, LPWSTR szDestDir, PUINT puDestDirLen), (uFlags, szFileName, szWinDir, szAppDir, szCurDir, puCurDirLen, szDestDir, puDestDirLen), (DWORD, LPCWSTR, LPCWSTR, LPCWSTR, LPWSTR, PUINT, LPWSTR, PUINT))
PROXY_IMPL(DWORD, VerInstallFileA, (DWORD uFlags, LPCSTR szSrcFileName, LPCSTR szDestFileName, LPCSTR szSrcDir, LPCSTR szDestDir, LPCSTR szCurDir, LPSTR szTmpFile, PUINT puTmpFileLen), (uFlags, szSrcFileName, szDestFileName, szSrcDir, szDestDir, szCurDir, szTmpFile, puTmpFileLen), (DWORD, LPCSTR, LPCSTR, LPCSTR, LPCSTR, LPCSTR, LPSTR, PUINT))
PROXY_IMPL(DWORD, VerInstallFileW, (DWORD uFlags, LPCWSTR szSrcFileName, LPCWSTR szDestFileName, LPCWSTR szSrcDir, LPCWSTR szDestDir, LPCWSTR szCurDir, LPWSTR szTmpFile, PUINT puTmpFileLen), (uFlags, szSrcFileName, szDestFileName, szSrcDir, szDestDir, szCurDir, szTmpFile, puTmpFileLen), (DWORD, LPCWSTR, LPCWSTR, LPCWSTR, LPCWSTR, LPCWSTR, LPWSTR, PUINT))
PROXY_IMPL(DWORD, VerLanguageNameA, (DWORD wLang, LPSTR szLang, DWORD cchLang), (wLang, szLang, cchLang), (DWORD, LPSTR, DWORD))
PROXY_IMPL(DWORD, VerLanguageNameW, (DWORD wLang, LPWSTR szLang, DWORD cchLang), (wLang, szLang, cchLang), (DWORD, LPWSTR, DWORD))

// XINPUT
PROXY_VOID(XInputEnable, (BOOL enable), (enable), (BOOL))
PROXY_IMPL(DWORD, XInputGetAudioDeviceIds, (DWORD dwUserIndex, void* pRenderDeviceId, void* pRenderCount, void* pCaptureDeviceId, void* pCaptureCount), (dwUserIndex, pRenderDeviceId, pRenderCount, pCaptureDeviceId, pCaptureCount), (DWORD, void*, void*, void*, void*))
PROXY_IMPL(DWORD, XInputGetBatteryInformation, (DWORD dwUserIndex, BYTE devType, void* pBatteryInformation), (dwUserIndex, devType, pBatteryInformation), (DWORD, BYTE, void*))
PROXY_IMPL(DWORD, XInputGetCapabilities, (DWORD dwUserIndex, DWORD dwFlags, void* pCapabilities), (dwUserIndex, dwFlags, pCapabilities), (DWORD, DWORD, void*))
PROXY_IMPL(DWORD, XInputGetDSoundAudioDeviceGuids, (DWORD dwUserIndex, void* pDSoundRenderGuid, void* pDSoundCaptureGuid), (dwUserIndex, pDSoundRenderGuid, pDSoundCaptureGuid), (DWORD, void*, void*))
PROXY_IMPL(DWORD, XInputGetKeystroke, (DWORD dwUserIndex, DWORD dwReserved, void* pKeystroke), (dwUserIndex, dwReserved, pKeystroke), (DWORD, DWORD, void*))
PROXY_IMPL(DWORD, XInputGetState, (DWORD dwUserIndex, void* pState), (dwUserIndex, pState), (DWORD, void*))
PROXY_IMPL(DWORD, XInputSetState, (DWORD dwUserIndex, void* pVibration), (dwUserIndex, pVibration), (DWORD, void*))
PROXY_IMPL(DWORD, XInputGetStateEx, (DWORD dwUserIndex, void* pState), (dwUserIndex, pState), (DWORD, void*))
PROXY_IMPL(DWORD, XInputWaitForGuideButton, (DWORD dwUserIndex, DWORD dwFlags, void* pUnknown), (dwUserIndex, dwFlags, pUnknown), (DWORD, DWORD, void*))
PROXY_IMPL(DWORD, XInputCancelGuideButtonWait, (DWORD dwUserIndex), (dwUserIndex), (DWORD))
PROXY_IMPL(DWORD, XInputPowerOffController, (DWORD dwUserIndex), (dwUserIndex), (DWORD))
PROXY_IMPL(DWORD, XInputGetBaseBusInformation, (DWORD dwUserIndex, void* pBusInfo), (dwUserIndex, pBusInfo), (DWORD, void*))
PROXY_IMPL(DWORD, XInputGetCapabilitiesEx, (DWORD dw1, DWORD dw2, DWORD dw3, void* p4), (dw1, dw2, dw3, p4), (DWORD, DWORD, DWORD, void*))

// WINMM
PROXY_IMPL(MMRESULT, CloseDriver, (HDRVR hDriver, LPARAM lParam1, LPARAM lParam2), (hDriver, lParam1, lParam2), (HDRVR, LPARAM, LPARAM))
PROXY_IMPL(LRESULT, DefDriverProc, (DWORD_PTR dwDriverIdentifier, HDRVR hDriver, UINT uMsg, LPARAM lParam1, LPARAM lParam2), (dwDriverIdentifier, hDriver, uMsg, lParam1, lParam2), (DWORD_PTR, HDRVR, UINT, LPARAM, LPARAM))
PROXY_IMPL(BOOL, DriverCallback, (DWORD dwCallback, DWORD dwFlags, HDRVR hDevice, DWORD dwMsg, DWORD dwUser, DWORD dwParam1, DWORD dwParam2), (dwCallback, dwFlags, hDevice, dwMsg, dwUser, dwParam1, dwParam2), (DWORD, DWORD, HDRVR, DWORD, DWORD, DWORD, DWORD))
PROXY_IMPL(HMODULE, DrvGetModuleHandle, (HDRVR hDriver), (hDriver), (HDRVR))
PROXY_IMPL(HMODULE, GetDriverModuleHandle, (HDRVR hDriver), (hDriver), (HDRVR))
PROXY_IMPL(BOOL, NotifyCallbackData, (UINT uData, UINT uMsg, DWORD dwParam1, DWORD dwParam2), (uData, uMsg, dwParam1, dwParam2), (UINT, UINT, DWORD, DWORD))
PROXY_IMPL(HDRVR, OpenDriver, (LPCWSTR szDriverName, LPCWSTR szSectionName, LPARAM lParam2), (szDriverName, szSectionName, lParam2), (LPCWSTR, LPCWSTR, LPARAM))
PROXY_IMPL(BOOL, PlaySound, (LPCWSTR pszSound, HMODULE hmod, DWORD fdwSound), (pszSound, hmod, fdwSound), (LPCWSTR, HMODULE, DWORD))
PROXY_IMPL(BOOL, PlaySoundA, (LPCSTR pszSound, HMODULE hmod, DWORD fdwSound), (pszSound, hmod, fdwSound), (LPCSTR, HMODULE, DWORD))
PROXY_IMPL(BOOL, PlaySoundW, (LPCWSTR pszSound, HMODULE hmod, DWORD fdwSound), (pszSound, hmod, fdwSound), (LPCWSTR, HMODULE, DWORD))
PROXY_IMPL(LRESULT, SendDriverMessage, (HDRVR hDriver, UINT uMsg, LPARAM lParam1, LPARAM lParam2), (hDriver, uMsg, lParam1, lParam2), (HDRVR, UINT, LPARAM, LPARAM))
PROXY_IMPL(BOOL, WOW32DriverCallback, (DWORD dwCallback, DWORD dwFlags, WORD wID, WORD wMsg, DWORD dwUser, DWORD dwParam1, DWORD dwParam2), (dwCallback, dwFlags, wID, wMsg, dwUser, dwParam1, dwParam2), (DWORD, DWORD, WORD, WORD, DWORD, DWORD, DWORD))
PROXY_IMPL(BOOL, WOW32ResolveMultiMediaHandle, (UINT uHandle, UINT uType, LPWORD lpwHandle, LPWORD lpwType, LPWORD lpwID, LPWORD lpwReserved), (uHandle, uType, lpwHandle, lpwType, lpwID, lpwReserved), (UINT, UINT, LPWORD, LPWORD, LPWORD, LPWORD))
PROXY_VOID(WOWAppExit, (HANDLE hTask), (hTask), (HANDLE))
PROXY_IMPL(UINT, aux32Message, (UINT uDeviceID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dwParam1, DWORD_PTR dwParam2), (uDeviceID, uMsg, dwUser, dwParam1, dwParam2), (UINT, UINT, DWORD_PTR, DWORD_PTR, DWORD_PTR))
PROXY_IMPL(MMRESULT, auxGetDevCapsA, (UINT_PTR uDeviceID, LPAUXCAPSA pac, UINT cbac), (uDeviceID, pac, cbac), (UINT_PTR, LPAUXCAPSA, UINT))
PROXY_IMPL(MMRESULT, auxGetDevCapsW, (UINT_PTR uDeviceID, LPAUXCAPSW pac, UINT cbac), (uDeviceID, pac, cbac), (UINT_PTR, LPAUXCAPSW, UINT))
PROXY_IMPL(UINT, auxGetNumDevs, (), (), ())
PROXY_IMPL(MMRESULT, auxGetVolume, (UINT uDeviceID, LPDWORD pdwVolume), (uDeviceID, pdwVolume), (UINT, LPDWORD))
PROXY_IMPL(MMRESULT, auxOutMessage, (UINT uDeviceID, UINT uMsg, DWORD_PTR dw1, DWORD_PTR dw2), (uDeviceID, uMsg, dw1, dw2), (UINT, UINT, DWORD_PTR, DWORD_PTR))
PROXY_IMPL(MMRESULT, auxSetVolume, (UINT uDeviceID, DWORD dwVolume), (uDeviceID, dwVolume), (UINT, DWORD))
PROXY_IMPL(UINT, joy32Message, (UINT uJoyID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dwParam1, DWORD_PTR dwParam2), (uJoyID, uMsg, dwUser, dwParam1, dwParam2), (UINT, UINT, DWORD_PTR, DWORD_PTR, DWORD_PTR))
PROXY_IMPL(MMRESULT, joyConfigChanged, (DWORD dwFlags), (dwFlags), (DWORD))
PROXY_IMPL(MMRESULT, joyGetDevCapsA, (UINT_PTR uJoyID, LPJOYCAPSA pjc, UINT cbjc), (uJoyID, pjc, cbjc), (UINT_PTR, LPJOYCAPSA, UINT))
PROXY_IMPL(MMRESULT, joyGetDevCapsW, (UINT_PTR uJoyID, LPJOYCAPSW pjc, UINT cbjc), (uJoyID, pjc, cbjc), (UINT_PTR, LPJOYCAPSW, UINT))
PROXY_IMPL(UINT, joyGetNumDevs, (), (), ())
PROXY_IMPL(MMRESULT, joyGetPos, (UINT uJoyID, LPJOYINFO pji), (uJoyID, pji), (UINT, LPJOYINFO))
PROXY_IMPL(MMRESULT, joyGetPosEx, (UINT uJoyID, LPJOYINFOEX pji), (uJoyID, pji), (UINT, LPJOYINFOEX))
PROXY_IMPL(MMRESULT, joyGetThreshold, (UINT uJoyID, LPUINT puThreshold), (uJoyID, puThreshold), (UINT, LPUINT))
PROXY_IMPL(MMRESULT, joyReleaseCapture, (UINT uJoyID), (uJoyID), (UINT))
PROXY_IMPL(MMRESULT, joySetCapture, (HWND hwnd, UINT uJoyID, UINT uPeriod, BOOL fChanged), (hwnd, uJoyID, uPeriod, fChanged), (HWND, UINT, UINT, BOOL))
PROXY_IMPL(MMRESULT, joySetThreshold, (UINT uJoyID, UINT uThreshold), (uJoyID, uThreshold), (UINT, UINT))
PROXY_IMPL(UINT, mci32Message, (UINT uDeviceID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dwParam1, DWORD_PTR dwParam2), (uDeviceID, uMsg, dwUser, dwParam1, dwParam2), (UINT, UINT, DWORD_PTR, DWORD_PTR, DWORD_PTR))
PROXY_IMPL(BOOL, mciDriverNotify, (HANDLE hCallback, UINT uDeviceID, UINT uStatus), (hCallback, uDeviceID, uStatus), (HANDLE, UINT, UINT))
PROXY_IMPL(UINT, mciDriverYield, (UINT uDeviceID), (uDeviceID), (UINT))
PROXY_IMPL(BOOL, mciExecute, (LPCSTR pszCommand), (pszCommand), (LPCSTR))
PROXY_IMPL(BOOL, mciFreeCommandResource, (UINT uTable), (uTable), (UINT))
PROXY_IMPL(HTASK, mciGetCreatorTask, (MCIDEVICEID mciId), (mciId), (MCIDEVICEID))
PROXY_IMPL(MCIDEVICEID, mciGetDeviceIDA, (LPCSTR pszDevice), (pszDevice), (LPCSTR))
PROXY_IMPL(MCIDEVICEID, mciGetDeviceIDFromElementIDA, (DWORD dwElementID, LPCSTR pszType), (dwElementID, pszType), (DWORD, LPCSTR))
PROXY_IMPL(MCIDEVICEID, mciGetDeviceIDFromElementIDW, (DWORD dwElementID, LPCWSTR pszType), (dwElementID, pszType), (DWORD, LPCWSTR))
PROXY_IMPL(MCIDEVICEID, mciGetDeviceIDW, (LPCWSTR pszDevice), (pszDevice), (LPCWSTR))
PROXY_IMPL(DWORD_PTR, mciGetDriverData, (MCIDEVICEID mciId), (mciId), (MCIDEVICEID))
PROXY_IMPL(BOOL, mciGetErrorStringA, (MCIERROR mcierr, LPSTR pszText, UINT cchText), (mcierr, pszText, cchText), (MCIERROR, LPSTR, UINT))
PROXY_IMPL(BOOL, mciGetErrorStringW, (MCIERROR mcierr, LPWSTR pszText, UINT cchText), (mcierr, pszText, cchText), (MCIERROR, LPWSTR, UINT))
PROXY_IMPL(YIELDPROC, mciGetYieldProc, (MCIDEVICEID mciId, LPDWORD pdwYieldData), (mciId, pdwYieldData), (MCIDEVICEID, LPDWORD))
PROXY_IMPL(UINT, mciLoadCommandResource, (HANDLE hInstance, LPCWSTR lpResName, UINT uType), (hInstance, lpResName, uType), (HANDLE, LPCWSTR, UINT))
PROXY_IMPL(MCIERROR, mciSendCommandA, (MCIDEVICEID mciId, UINT uMsg, DWORD_PTR dwParam1, DWORD_PTR dwParam2), (mciId, uMsg, dwParam1, dwParam2), (MCIDEVICEID, UINT, DWORD_PTR, DWORD_PTR))
PROXY_IMPL(MCIERROR, mciSendCommandW, (MCIDEVICEID mciId, UINT uMsg, DWORD_PTR dwParam1, DWORD_PTR dwParam2), (mciId, uMsg, dwParam1, dwParam2), (MCIDEVICEID, UINT, DWORD_PTR, DWORD_PTR))
PROXY_IMPL(MCIERROR, mciSendStringA, (LPCSTR lpstrCommand, LPSTR lpstrReturnString, UINT uReturnLength, HWND hwndCallback), (lpstrCommand, lpstrReturnString, uReturnLength, hwndCallback), (LPCSTR, LPSTR, UINT, HWND))
PROXY_IMPL(MCIERROR, mciSendStringW, (LPCWSTR lpstrCommand, LPWSTR lpstrReturnString, UINT uReturnLength, HWND hwndCallback), (lpstrCommand, lpstrReturnString, uReturnLength, hwndCallback), (LPCWSTR, LPWSTR, UINT, HWND))
PROXY_IMPL(BOOL, mciSetDriverData, (MCIDEVICEID mciId, DWORD_PTR dwData), (mciId, dwData), (MCIDEVICEID, DWORD_PTR))
PROXY_IMPL(BOOL, mciSetYieldProc, (MCIDEVICEID mciId, YIELDPROC fpYieldProc, DWORD dwYieldData), (mciId, fpYieldProc, dwYieldData), (MCIDEVICEID, YIELDPROC, DWORD))
PROXY_IMPL(UINT, mid32Message, (UINT uDeviceID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dwParam1, DWORD_PTR dwParam2), (uDeviceID, uMsg, dwUser, dwParam1, dwParam2), (UINT, UINT, DWORD_PTR, DWORD_PTR, DWORD_PTR))
PROXY_IMPL(MMRESULT, midiConnect, (HMIDI hmi, HMIDIOUT hmo, LPVOID pReserved), (hmi, hmo, pReserved), (HMIDI, HMIDIOUT, LPVOID))
PROXY_IMPL(MMRESULT, midiDisconnect, (HMIDI hmi, HMIDIOUT hmo, LPVOID pReserved), (hmi, hmo, pReserved), (HMIDI, HMIDIOUT, LPVOID))
PROXY_IMPL(MMRESULT, midiInAddBuffer, (HMIDIIN hmi, LPMIDIHDR pmh, UINT cbmh), (hmi, pmh, cbmh), (HMIDIIN, LPMIDIHDR, UINT))
PROXY_IMPL(MMRESULT, midiInClose, (HMIDIIN hmi), (hmi), (HMIDIIN))
PROXY_IMPL(MMRESULT, midiInGetDevCapsA, (UINT_PTR uDeviceID, LPMIDIINCAPSA pmic, UINT cbmic), (uDeviceID, pmic, cbmic), (UINT_PTR, LPMIDIINCAPSA, UINT))
PROXY_IMPL(MMRESULT, midiInGetDevCapsW, (UINT_PTR uDeviceID, LPMIDIINCAPSW pmic, UINT cbmic), (uDeviceID, pmic, cbmic), (UINT_PTR, LPMIDIINCAPSW, UINT))
PROXY_IMPL(MMRESULT, midiInGetErrorTextA, (MMRESULT mmrError, LPSTR pszText, UINT cchText), (mmrError, pszText, cchText), (MMRESULT, LPSTR, UINT))
PROXY_IMPL(MMRESULT, midiInGetErrorTextW, (MMRESULT mmrError, LPWSTR pszText, UINT cchText), (mmrError, pszText, cchText), (MMRESULT, LPWSTR, UINT))
PROXY_IMPL(MMRESULT, midiInGetID, (HMIDIIN hmi, LPUINT puDeviceID), (hmi, puDeviceID), (HMIDIIN, LPUINT))
PROXY_IMPL(UINT, midiInGetNumDevs, (), (), ())
PROXY_IMPL(MMRESULT, midiInMessage, (HMIDIIN hmi, UINT uMsg, DWORD_PTR dw1, DWORD_PTR dw2), (hmi, uMsg, dw1, dw2), (HMIDIIN, UINT, DWORD_PTR, DWORD_PTR))
PROXY_IMPL(MMRESULT, midiInOpen, (LPHMIDIIN phmi, UINT uDeviceID, DWORD_PTR dwCallback, DWORD_PTR dwInstance, DWORD fdwOpen), (phmi, uDeviceID, dwCallback, dwInstance, fdwOpen), (LPHMIDIIN, UINT, DWORD_PTR, DWORD_PTR, DWORD))
PROXY_IMPL(MMRESULT, midiInPrepareHeader, (HMIDIIN hmi, LPMIDIHDR pmh, UINT cbmh), (hmi, pmh, cbmh), (HMIDIIN, LPMIDIHDR, UINT))
PROXY_IMPL(MMRESULT, midiInReset, (HMIDIIN hmi), (hmi), (HMIDIIN))
PROXY_IMPL(MMRESULT, midiInStart, (HMIDIIN hmi), (hmi), (HMIDIIN))
PROXY_IMPL(MMRESULT, midiInStop, (HMIDIIN hmi), (hmi), (HMIDIIN))
PROXY_IMPL(MMRESULT, midiInUnprepareHeader, (HMIDIIN hmi, LPMIDIHDR pmh, UINT cbmh), (hmi, pmh, cbmh), (HMIDIIN, LPMIDIHDR, UINT))
PROXY_IMPL(MMRESULT, midiOutCacheDrumPatches, (HMIDIOUT hmo, UINT uPatch, LPWORD pwkya, UINT fuCache), (hmo, uPatch, pwkya, fuCache), (HMIDIOUT, UINT, LPWORD, UINT))
PROXY_IMPL(MMRESULT, midiOutCachePatches, (HMIDIOUT hmo, UINT uBank, LPWORD pwpa, UINT fuCache), (hmo, uBank, pwpa, fuCache), (HMIDIOUT, UINT, LPWORD, UINT))
PROXY_IMPL(MMRESULT, midiOutClose, (HMIDIOUT hmo), (hmo), (HMIDIOUT))
PROXY_IMPL(MMRESULT, midiOutGetDevCapsA, (UINT_PTR uDeviceID, LPMIDIOUTCAPSA pmoc, UINT cbmoc), (uDeviceID, pmoc, cbmoc), (UINT_PTR, LPMIDIOUTCAPSA, UINT))
PROXY_IMPL(MMRESULT, midiOutGetDevCapsW, (UINT_PTR uDeviceID, LPMIDIOUTCAPSW pmoc, UINT cbmoc), (uDeviceID, pmoc, cbmoc), (UINT_PTR, LPMIDIOUTCAPSW, UINT))
PROXY_IMPL(MMRESULT, midiOutGetErrorTextA, (MMRESULT mmrError, LPSTR pszText, UINT cchText), (mmrError, pszText, cchText), (MMRESULT, LPSTR, UINT))
PROXY_IMPL(MMRESULT, midiOutGetErrorTextW, (MMRESULT mmrError, LPWSTR pszText, UINT cchText), (mmrError, pszText, cchText), (MMRESULT, LPWSTR, UINT))
PROXY_IMPL(MMRESULT, midiOutGetID, (HMIDIOUT hmo, LPUINT puDeviceID), (hmo, puDeviceID), (HMIDIOUT, LPUINT))
PROXY_IMPL(UINT, midiOutGetNumDevs, (), (), ())
PROXY_IMPL(MMRESULT, midiOutGetVolume, (HMIDIOUT hmo, LPDWORD pdwVolume), (hmo, pdwVolume), (HMIDIOUT, LPDWORD))
PROXY_IMPL(MMRESULT, midiOutLongMsg, (HMIDIOUT hmo, LPMIDIHDR pmh, UINT cbmh), (hmo, pmh, cbmh), (HMIDIOUT, LPMIDIHDR, UINT))
PROXY_IMPL(MMRESULT, midiOutMessage, (HMIDIOUT hmo, UINT uMsg, DWORD_PTR dw1, DWORD_PTR dw2), (hmo, uMsg, dw1, dw2), (HMIDIOUT, UINT, DWORD_PTR, DWORD_PTR))
PROXY_IMPL(MMRESULT, midiOutOpen, (LPHMIDIOUT phmo, UINT uDeviceID, DWORD_PTR dwCallback, DWORD_PTR dwInstance, DWORD fdwOpen), (phmo, uDeviceID, dwCallback, dwInstance, fdwOpen), (LPHMIDIOUT, UINT, DWORD_PTR, DWORD_PTR, DWORD))
PROXY_IMPL(MMRESULT, midiOutPrepareHeader, (HMIDIOUT hmo, LPMIDIHDR pmh, UINT cbmh), (hmo, pmh, cbmh), (HMIDIOUT, LPMIDIHDR, UINT))
PROXY_IMPL(MMRESULT, midiOutReset, (HMIDIOUT hmo), (hmo), (HMIDIOUT))
PROXY_IMPL(MMRESULT, midiOutSetVolume, (HMIDIOUT hmo, DWORD dwVolume), (hmo, dwVolume), (HMIDIOUT, DWORD))
PROXY_IMPL(MMRESULT, midiOutShortMsg, (HMIDIOUT hmo, DWORD dwMsg), (hmo, dwMsg), (HMIDIOUT, DWORD))
PROXY_IMPL(MMRESULT, midiOutUnprepareHeader, (HMIDIOUT hmo, LPMIDIHDR pmh, UINT cbmh), (hmo, pmh, cbmh), (HMIDIOUT, LPMIDIHDR, UINT))
PROXY_IMPL(MMRESULT, midiStreamClose, (HMIDISTRM hms), (hms), (HMIDISTRM))
PROXY_IMPL(MMRESULT, midiStreamOpen, (LPHMIDISTRM phms, LPUINT puDeviceID, DWORD cMidi, DWORD_PTR dwCallback, DWORD_PTR dwInstance, DWORD fdwOpen), (phms, puDeviceID, cMidi, dwCallback, dwInstance, fdwOpen), (LPHMIDISTRM, LPUINT, DWORD, DWORD_PTR, DWORD_PTR, DWORD))
PROXY_IMPL(MMRESULT, midiStreamOut, (HMIDISTRM hms, LPMIDIHDR pmh, UINT cbmh), (hms, pmh, cbmh), (HMIDISTRM, LPMIDIHDR, UINT))
PROXY_IMPL(MMRESULT, midiStreamPause, (HMIDISTRM hms), (hms), (HMIDISTRM))
PROXY_IMPL(MMRESULT, midiStreamPosition, (HMIDISTRM hms, LPMMTIME pmmt, UINT cbmmt), (hms, pmmt, cbmmt), (HMIDISTRM, LPMMTIME, UINT))
PROXY_IMPL(MMRESULT, midiStreamProperty, (HMIDISTRM hms, LPBYTE lppropdata, DWORD dwProperty), (hms, lppropdata, dwProperty), (HMIDISTRM, LPBYTE, DWORD))
PROXY_IMPL(MMRESULT, midiStreamRestart, (HMIDISTRM hms), (hms), (HMIDISTRM))
PROXY_IMPL(MMRESULT, midiStreamStop, (HMIDISTRM hms), (hms), (HMIDISTRM))
PROXY_IMPL(MMRESULT, mixerClose, (HMIXER hmx), (hmx), (HMIXER))
PROXY_IMPL(MMRESULT, mixerGetControlDetailsA, (HMIXEROBJ hmxobj, LPMIXERCONTROLDETAILS pmxcd, DWORD fdwDetails), (hmxobj, pmxcd, fdwDetails), (HMIXEROBJ, LPMIXERCONTROLDETAILS, DWORD))
PROXY_IMPL(MMRESULT, mixerGetControlDetailsW, (HMIXEROBJ hmxobj, LPMIXERCONTROLDETAILS pmxcd, DWORD fdwDetails), (hmxobj, pmxcd, fdwDetails), (HMIXEROBJ, LPMIXERCONTROLDETAILS, DWORD))
PROXY_IMPL(MMRESULT, mixerGetDevCapsA, (UINT_PTR uMxId, LPMIXERCAPSA pmxc, UINT cbmxc), (uMxId, pmxc, cbmxc), (UINT_PTR, LPMIXERCAPSA, UINT))
PROXY_IMPL(MMRESULT, mixerGetDevCapsW, (UINT_PTR uMxId, LPMIXERCAPSW pmxc, UINT cbmxc), (uMxId, pmxc, cbmxc), (UINT_PTR, LPMIXERCAPSW, UINT))
PROXY_IMPL(MMRESULT, mixerGetID, (HMIXEROBJ hmxobj, LPUINT puMxId, DWORD fdwId), (hmxobj, puMxId, fdwId), (HMIXEROBJ, LPUINT, DWORD))
PROXY_IMPL(MMRESULT, mixerGetLineControlsA, (HMIXEROBJ hmxobj, LPMIXERLINECONTROLSA pmxlc, DWORD fdwControls), (hmxobj, pmxlc, fdwControls), (HMIXEROBJ, LPMIXERLINECONTROLSA, DWORD))
PROXY_IMPL(MMRESULT, mixerGetLineControlsW, (HMIXEROBJ hmxobj, LPMIXERLINECONTROLSW pmxlc, DWORD fdwControls), (hmxobj, pmxlc, fdwControls), (HMIXEROBJ, LPMIXERLINECONTROLSW, DWORD))
PROXY_IMPL(MMRESULT, mixerGetLineInfoA, (HMIXEROBJ hmxobj, LPMIXERLINEA pmxl, DWORD fdwInfo), (hmxobj, pmxl, fdwInfo), (HMIXEROBJ, LPMIXERLINEA, DWORD))
PROXY_IMPL(MMRESULT, mixerGetLineInfoW, (HMIXEROBJ hmxobj, LPMIXERLINEW pmxl, DWORD fdwInfo), (hmxobj, pmxl, fdwInfo), (HMIXEROBJ, LPMIXERLINEW, DWORD))
PROXY_IMPL(UINT, mixerGetNumDevs, (), (), ())
PROXY_IMPL(MMRESULT, mixerMessage, (HMIXER hmx, UINT uMsg, DWORD_PTR dwParam1, DWORD_PTR dwParam2), (hmx, uMsg, dwParam1, dwParam2), (HMIXER, UINT, DWORD_PTR, DWORD_PTR))
PROXY_IMPL(MMRESULT, mixerOpen, (LPHMIXER phmx, UINT uMxId, DWORD_PTR dwCallback, DWORD_PTR dwInstance, DWORD fdwOpen), (phmx, uMxId, dwCallback, dwInstance, fdwOpen), (LPHMIXER, UINT, DWORD_PTR, DWORD_PTR, DWORD))
PROXY_IMPL(MMRESULT, mixerSetControlDetails, (HMIXEROBJ hmxobj, LPMIXERCONTROLDETAILS pmxcd, DWORD fdwDetails), (hmxobj, pmxcd, fdwDetails), (HMIXEROBJ, LPMIXERCONTROLDETAILS, DWORD))
PROXY_IMPL(UINT, mmDrvInstall, (HDRVR hDriver, LPCWSTR wszDrvEntry, LPCWSTR wszDrvUser, UINT wFlags), (hDriver, wszDrvEntry, wszDrvUser, wFlags), (HDRVR, LPCWSTR, LPCWSTR, UINT))
PROXY_IMPL(HTASK, mmGetCurrentTask, (), (), ())
PROXY_VOID(mmTaskBlock, (HTASK h), (h), (HTASK))
PROXY_IMPL(HTASK, mmTaskCreate, (HTASK lpTask, HTASK* lphTask, DWORD_PTR dwInst), (lpTask, lphTask, dwInst), (HTASK, HTASK*, DWORD_PTR))
PROXY_VOID(mmTaskSignal, (HTASK h), (h), (HTASK))
PROXY_VOID(mmTaskYield, (), (), ())
PROXY_IMPL(MMRESULT, mmioAdvance, (HMMIO hmmio, LPMMIOINFO pmmioinfo, UINT fuAdvance), (hmmio, pmmioinfo, fuAdvance), (HMMIO, LPMMIOINFO, UINT))
PROXY_IMPL(MMRESULT, mmioAscend, (HMMIO hmmio, LPMMCKINFO pmmcki, UINT fuAscend), (hmmio, pmmcki, fuAscend), (HMMIO, LPMMCKINFO, UINT))
PROXY_IMPL(MMRESULT, mmioClose, (HMMIO hmmio, UINT fuClose), (hmmio, fuClose), (HMMIO, UINT))
PROXY_IMPL(MMRESULT, mmioCreateChunk, (HMMIO hmmio, LPMMCKINFO pmmcki, UINT fuCreate), (hmmio, pmmcki, fuCreate), (HMMIO, LPMMCKINFO, UINT))
PROXY_IMPL(MMRESULT, mmioDescend, (HMMIO hmmio, LPMMCKINFO pmmcki, const MMCKINFO* pmmckiParent, UINT fuDescend), (hmmio, pmmcki, pmmckiParent, fuDescend), (HMMIO, LPMMCKINFO, const MMCKINFO*, UINT))
PROXY_IMPL(MMRESULT, mmioFlush, (HMMIO hmmio, UINT fuFlush), (hmmio, fuFlush), (HMMIO, UINT))
PROXY_IMPL(MMRESULT, mmioGetInfo, (HMMIO hmmio, LPMMIOINFO pmmioinfo, UINT fuInfo), (hmmio, pmmioinfo, fuInfo), (HMMIO, LPMMIOINFO, UINT))
PROXY_IMPL(LPMMIOPROC, mmioInstallIOProcA, (FOURCC fccIOProc, LPMMIOPROC pIOProc, DWORD dwFlags), (fccIOProc, pIOProc, dwFlags), (FOURCC, LPMMIOPROC, DWORD))
PROXY_IMPL(LPMMIOPROC, mmioInstallIOProcW, (FOURCC fccIOProc, LPMMIOPROC pIOProc, DWORD dwFlags), (fccIOProc, pIOProc, dwFlags), (FOURCC, LPMMIOPROC, DWORD))
PROXY_IMPL(HMMIO, mmioOpenA, (LPSTR pszFileName, LPMMIOINFO pmmioinfo, DWORD fdwOpen), (pszFileName, pmmioinfo, fdwOpen), (LPSTR, LPMMIOINFO, DWORD))
PROXY_IMPL(HMMIO, mmioOpenW, (LPWSTR pszFileName, LPMMIOINFO pmmioinfo, DWORD fdwOpen), (pszFileName, pmmioinfo, fdwOpen), (LPWSTR, LPMMIOINFO, DWORD))
PROXY_IMPL(LONG, mmioRead, (HMMIO hmmio, HPSTR pch, LONG cch), (hmmio, pch, cch), (HMMIO, HPSTR, LONG))
PROXY_IMPL(MMRESULT, mmioRenameA, (LPCSTR pszFileName, LPCSTR pszNewFileName, LPCMMIOINFO pmmioinfo, DWORD fdwRename), (pszFileName, pszNewFileName, pmmioinfo, fdwRename), (LPCSTR, LPCSTR, LPCMMIOINFO, DWORD))
PROXY_IMPL(MMRESULT, mmioRenameW, (LPCWSTR pszFileName, LPCWSTR pszNewFileName, LPCMMIOINFO pmmioinfo, DWORD fdwRename), (pszFileName, pszNewFileName, pmmioinfo, fdwRename), (LPCWSTR, LPCWSTR, LPCMMIOINFO, DWORD))
PROXY_IMPL(LONG, mmioSeek, (HMMIO hmmio, LONG lOffset, int iOrigin), (hmmio, lOffset, iOrigin), (HMMIO, LONG, int))
PROXY_IMPL(LRESULT, mmioSendMessage, (HMMIO hmmio, UINT uMsg, LPARAM lParam1, LPARAM lParam2), (hmmio, uMsg, lParam1, lParam2), (HMMIO, UINT, LPARAM, LPARAM))
PROXY_IMPL(MMRESULT, mmioSetBuffer, (HMMIO hmmio, LPSTR pchBuffer, LONG cchBuffer, UINT fuBuffer), (hmmio, pchBuffer, cchBuffer, fuBuffer), (HMMIO, LPSTR, LONG, UINT))
PROXY_IMPL(MMRESULT, mmioSetInfo, (HMMIO hmmio, LPCMMIOINFO pmmioinfo, UINT fuInfo), (hmmio, pmmioinfo, fuInfo), (HMMIO, LPCMMIOINFO, UINT))
PROXY_IMPL(FOURCC, mmioStringToFOURCCA, (LPCSTR sz, UINT uFlags), (sz, uFlags), (LPCSTR, UINT))
PROXY_IMPL(FOURCC, mmioStringToFOURCCW, (LPCWSTR sz, UINT uFlags), (sz, uFlags), (LPCWSTR, UINT))
PROXY_IMPL(LONG, mmioWrite, (HMMIO hmmio, const char* pch, LONG cch), (hmmio, pch, cch), (HMMIO, const char*, LONG))
PROXY_IMPL(UINT, mmsystemGetVersion, (), (), ())
PROXY_IMPL(UINT, mod32Message, (UINT uDeviceID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dwParam1, DWORD_PTR dwParam2), (uDeviceID, uMsg, dwUser, dwParam1, dwParam2), (UINT, UINT, DWORD_PTR, DWORD_PTR, DWORD_PTR))
PROXY_IMPL(UINT, mxd32Message, (UINT uDeviceID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dwParam1, DWORD_PTR dwParam2), (uDeviceID, uMsg, dwUser, dwParam1, dwParam2), (UINT, UINT, DWORD_PTR, DWORD_PTR, DWORD_PTR))
PROXY_IMPL(BOOL, sndPlaySoundA, (LPCSTR pszSound, UINT fuSound), (pszSound, fuSound), (LPCSTR, UINT))
PROXY_IMPL(BOOL, sndPlaySoundW, (LPCWSTR pszSound, UINT fuSound), (pszSound, fuSound), (LPCWSTR, UINT))
PROXY_IMPL(UINT, tid32Message, (UINT uDeviceID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dwParam1, DWORD_PTR dwParam2), (uDeviceID, uMsg, dwUser, dwParam1, dwParam2), (UINT, UINT, DWORD_PTR, DWORD_PTR, DWORD_PTR))
PROXY_IMPL(MMRESULT, timeBeginPeriod, (UINT uPeriod), (uPeriod), (UINT))
PROXY_IMPL(MMRESULT, timeEndPeriod, (UINT uPeriod), (uPeriod), (UINT))
PROXY_IMPL(MMRESULT, timeGetDevCaps, (LPTIMECAPS ptc, UINT cbtc), (ptc, cbtc), (LPTIMECAPS, UINT))
PROXY_IMPL(MMRESULT, timeGetSystemTime, (LPMMTIME pmmt, UINT cbmmt), (pmmt, cbmmt), (LPMMTIME, UINT))
PROXY_IMPL(DWORD, timeGetTime, (), (), ())
PROXY_IMPL(MMRESULT, timeKillEvent, (UINT uTimerID), (uTimerID), (UINT))
PROXY_IMPL(MMRESULT, timeSetEvent, (UINT uDelay, UINT uResolution, LPTIMECALLBACK fptc, DWORD_PTR dwUser, UINT fuEvent), (uDelay, uResolution, fptc, dwUser, fuEvent), (UINT, UINT, LPTIMECALLBACK, DWORD_PTR, UINT))
PROXY_IMPL(MMRESULT, waveInAddBuffer, (HWAVEIN hwi, LPWAVEHDR pwh, UINT cbwh), (hwi, pwh, cbwh), (HWAVEIN, LPWAVEHDR, UINT))
PROXY_IMPL(MMRESULT, waveInClose, (HWAVEIN hwi), (hwi), (HWAVEIN))
PROXY_IMPL(MMRESULT, waveInGetDevCapsA, (UINT_PTR uDeviceID, LPWAVEINCAPSA pwic, UINT cbwic), (uDeviceID, pwic, cbwic), (UINT_PTR, LPWAVEINCAPSA, UINT))
PROXY_IMPL(MMRESULT, waveInGetDevCapsW, (UINT_PTR uDeviceID, LPWAVEINCAPSW pwic, UINT cbwic), (uDeviceID, pwic, cbwic), (UINT_PTR, LPWAVEINCAPSW, UINT))
PROXY_IMPL(MMRESULT, waveInGetErrorTextA, (MMRESULT mmrError, LPSTR pszText, UINT cchText), (mmrError, pszText, cchText), (MMRESULT, LPSTR, UINT))
PROXY_IMPL(MMRESULT, waveInGetErrorTextW, (MMRESULT mmrError, LPWSTR pszText, UINT cchText), (mmrError, pszText, cchText), (MMRESULT, LPWSTR, UINT))
PROXY_IMPL(MMRESULT, waveInGetID, (HWAVEIN hwi, LPUINT puDeviceID), (hwi, puDeviceID), (HWAVEIN, LPUINT))
PROXY_IMPL(UINT, waveInGetNumDevs, (), (), ())
PROXY_IMPL(MMRESULT, waveInGetPosition, (HWAVEIN hwi, LPMMTIME pmmt, UINT cbmmt), (hwi, pmmt, cbmmt), (HWAVEIN, LPMMTIME, UINT))
PROXY_IMPL(MMRESULT, waveInMessage, (HWAVEIN hwi, UINT uMsg, DWORD_PTR dw1, DWORD_PTR dw2), (hwi, uMsg, dw1, dw2), (HWAVEIN, UINT, DWORD_PTR, DWORD_PTR))
PROXY_IMPL(MMRESULT, waveInOpen, (LPHWAVEIN phwi, UINT uDeviceID, LPCWAVEFORMATEX pwfx, DWORD_PTR dwCallback, DWORD_PTR dwInstance, DWORD fdwOpen), (phwi, uDeviceID, pwfx, dwCallback, dwInstance, fdwOpen), (LPHWAVEIN, UINT, LPCWAVEFORMATEX, DWORD_PTR, DWORD_PTR, DWORD))
PROXY_IMPL(MMRESULT, waveInPrepareHeader, (HWAVEIN hwi, LPWAVEHDR pwh, UINT cbwh), (hwi, pwh, cbwh), (HWAVEIN, LPWAVEHDR, UINT))
PROXY_IMPL(MMRESULT, waveInReset, (HWAVEIN hwi), (hwi), (HWAVEIN))
PROXY_IMPL(MMRESULT, waveInStart, (HWAVEIN hwi), (hwi), (HWAVEIN))
PROXY_IMPL(MMRESULT, waveInStop, (HWAVEIN hwi), (hwi), (HWAVEIN))
PROXY_IMPL(MMRESULT, waveInUnprepareHeader, (HWAVEIN hwi, LPWAVEHDR pwh, UINT cbwh), (hwi, pwh, cbwh), (HWAVEIN, LPWAVEHDR, UINT))
PROXY_IMPL(MMRESULT, waveOutBreakLoop, (HWAVEOUT hwo), (hwo), (HWAVEOUT))
PROXY_IMPL(MMRESULT, waveOutClose, (HWAVEOUT hwo), (hwo), (HWAVEOUT))
PROXY_IMPL(MMRESULT, waveOutGetDevCapsA, (UINT_PTR uDeviceID, LPWAVEOUTCAPSA pwoc, UINT cbwoc), (uDeviceID, pwoc, cbwoc), (UINT_PTR, LPWAVEOUTCAPSA, UINT))
PROXY_IMPL(MMRESULT, waveOutGetDevCapsW, (UINT_PTR uDeviceID, LPWAVEOUTCAPSW pwoc, UINT cbwoc), (uDeviceID, pwoc, cbwoc), (UINT_PTR, LPWAVEOUTCAPSW, UINT))
PROXY_IMPL(MMRESULT, waveOutGetErrorTextA, (MMRESULT mmrError, LPSTR pszText, UINT cchText), (mmrError, pszText, cchText), (MMRESULT, LPSTR, UINT))
PROXY_IMPL(MMRESULT, waveOutGetErrorTextW, (MMRESULT mmrError, LPWSTR pszText, UINT cchText), (mmrError, pszText, cchText), (MMRESULT, LPWSTR, UINT))
PROXY_IMPL(MMRESULT, waveOutGetID, (HWAVEOUT hwo, LPUINT puDeviceID), (hwo, puDeviceID), (HWAVEOUT, LPUINT))
PROXY_IMPL(UINT, waveOutGetNumDevs, (), (), ())
PROXY_IMPL(MMRESULT, waveOutGetPitch, (HWAVEOUT hwo, LPDWORD pdwPitch), (hwo, pdwPitch), (HWAVEOUT, LPDWORD))
PROXY_IMPL(MMRESULT, waveOutGetPlaybackRate, (HWAVEOUT hwo, LPDWORD pdwRate), (hwo, pdwRate), (HWAVEOUT, LPDWORD))
PROXY_IMPL(MMRESULT, waveOutGetPosition, (HWAVEOUT hwo, LPMMTIME pmmt, UINT cbmmt), (hwo, pmmt, cbmmt), (HWAVEOUT, LPMMTIME, UINT))
PROXY_IMPL(MMRESULT, waveOutGetVolume, (HWAVEOUT hwo, LPDWORD pdwVolume), (hwo, pdwVolume), (HWAVEOUT, LPDWORD))
PROXY_IMPL(MMRESULT, waveOutMessage, (HWAVEOUT hwo, UINT uMsg, DWORD_PTR dw1, DWORD_PTR dw2), (hwo, uMsg, dw1, dw2), (HWAVEOUT, UINT, DWORD_PTR, DWORD_PTR))
PROXY_IMPL(MMRESULT, waveOutOpen, (LPHWAVEOUT phwo, UINT uDeviceID, LPCWAVEFORMATEX pwfx, DWORD_PTR dwCallback, DWORD_PTR dwInstance, DWORD fdwOpen), (phwo, uDeviceID, pwfx, dwCallback, dwInstance, fdwOpen), (LPHWAVEOUT, UINT, LPCWAVEFORMATEX, DWORD_PTR, DWORD_PTR, DWORD))
PROXY_IMPL(MMRESULT, waveOutPause, (HWAVEOUT hwo), (hwo), (HWAVEOUT))
PROXY_IMPL(MMRESULT, waveOutPrepareHeader, (HWAVEOUT hwo, LPWAVEHDR pwh, UINT cbwh), (hwo, pwh, cbwh), (HWAVEOUT, LPWAVEHDR, UINT))
PROXY_IMPL(MMRESULT, waveOutReset, (HWAVEOUT hwo), (hwo), (HWAVEOUT))
PROXY_IMPL(MMRESULT, waveOutRestart, (HWAVEOUT hwo), (hwo), (HWAVEOUT))
PROXY_IMPL(MMRESULT, waveOutSetPitch, (HWAVEOUT hwo, DWORD dwPitch), (hwo, dwPitch), (HWAVEOUT, DWORD))
PROXY_IMPL(MMRESULT, waveOutSetPlaybackRate, (HWAVEOUT hwo, DWORD dwRate), (hwo, dwRate), (HWAVEOUT, DWORD))
PROXY_IMPL(MMRESULT, waveOutSetVolume, (HWAVEOUT hwo, DWORD dwVolume), (hwo, dwVolume), (HWAVEOUT, DWORD))
PROXY_IMPL(MMRESULT, waveOutUnprepareHeader, (HWAVEOUT hwo, LPWAVEHDR pwh, UINT cbwh), (hwo, pwh, cbwh), (HWAVEOUT, LPWAVEHDR, UINT))
PROXY_IMPL(MMRESULT, waveOutWrite, (HWAVEOUT hwo, LPWAVEHDR pwh, UINT cbwh), (hwo, pwh, cbwh), (HWAVEOUT, LPWAVEHDR, UINT))
PROXY_IMPL(UINT, wid32Message, (UINT uDeviceID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dwParam1, DWORD_PTR dwParam2), (uDeviceID, uMsg, dwUser, dwParam1, dwParam2), (UINT, UINT, DWORD_PTR, DWORD_PTR, DWORD_PTR))
PROXY_IMPL(UINT, wod32Message, (UINT uDeviceID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dwParam1, DWORD_PTR dwParam2), (uDeviceID, uMsg, dwUser, dwParam1, dwParam2), (UINT, UINT, DWORD_PTR, DWORD_PTR, DWORD_PTR))