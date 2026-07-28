#pragma once

#include "window_subclass_lifecycle.h"

#include <windows.h>
#include <commctrl.h>

#include <atomic>
#include <mutex>

namespace bgm_window {

class OwnerThreadSubclass {
public:
    using PublicationProbe = void (*)(void*);

    OwnerThreadSubclass() = default;

    OwnerThreadSubclass(const OwnerThreadSubclass&) = delete;
    OwnerThreadSubclass& operator=(const OwnerThreadSubclass&) = delete;

    void Configure(HMODULE module, SUBCLASSPROC procedure, UINT_PTR subclassId) {
        std::lock_guard<std::mutex> lock(mutex_);
        module_ = module;
        procedure_ = procedure;
        subclassId_ = subclassId;
    }

    bool RequestInstall(
        HWND window,
        std::uintptr_t ownerToken
    ) {
        DWORD processId = 0;
        const DWORD ownerThread = GetWindowThreadProcessId(window, &processId);
        if (!window ||
            ownerToken == 0 ||
            ownerThread == 0 ||
            processId != GetCurrentProcessId()) {
            if (window && ownerToken != 0) {
                std::lock_guard<std::mutex> lock(mutex_);
                if (lifecycle_.RejectInstall(
                        reinterpret_cast<std::uintptr_t>(window),
                        ownerThread,
                        ownerToken)) {
                    window_ = window;
                    lastError_ =
                        ownerThread == 0
                            ? ERROR_INVALID_WINDOW_HANDLE
                            : ERROR_ACCESS_DENIED;
                }
            }
            return false;
        }

        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (!module_ ||
                !procedure_ ||
                !lifecycle_.BeginInstall(
                    reinterpret_cast<std::uintptr_t>(window),
                    ownerThread,
                    ownerToken)) {
                return false;
            }
            window_ = window;
        }

        DispatchPendingCommand(ownerThread);
        return true;
    }

    void RequestRemove() {
        DWORD ownerThread = 0;
        bool dispatch = false;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (!lifecycle_.BeginRemove()) {
                return;
            }
            ownerThread = lifecycle_.OwnerThread();
            dispatch = lifecycle_.Phase() == SubclassPhase::RemoveQueued;
        }

        if (dispatch) {
            DispatchPendingCommand(ownerThread);
        }
    }

    void NotifyDestroyed(HWND window) {
        NotifyDestroyedOnOwnerThread(
            window,
            GetCurrentThreadId()
        );
    }

    bool HandleControlMessage(
        HWND window,
        UINT message
    ) {
        if (message != ControlMessage()) {
            return false;
        }
        ExecutePendingCommand(window);
        return true;
    }

    bool IsInstalledFor(
        HWND window,
        std::uintptr_t ownerToken
    ) const {
        DWORD processId = 0;
        const DWORD ownerThread = GetWindowThreadProcessId(window, &processId);
        std::lock_guard<std::mutex> lock(mutex_);
        return
            processId == GetCurrentProcessId() &&
            lifecycle_.IsInstalledFor(
                reinterpret_cast<std::uintptr_t>(window),
                ownerThread,
                ownerToken);
    }

    bool IsInstallBlockedFor(
        HWND window,
        std::uintptr_t ownerToken
    ) const {
        DWORD processId = 0;
        const DWORD ownerThread = GetWindowThreadProcessId(window, &processId);
        std::lock_guard<std::mutex> lock(mutex_);
        if (ownerThread == 0) {
            return lifecycle_.IsInstallBlockedFor(
                reinterpret_cast<std::uintptr_t>(window),
                0,
                ownerToken);
        }
        return processId == GetCurrentProcessId() &&
               lifecycle_.IsInstallBlockedFor(
                   reinterpret_cast<std::uintptr_t>(window),
                   ownerThread,
                   ownerToken);
    }

    bool CanDisposeRenderer() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return lifecycle_.CanDisposeRenderer();
    }

    SubclassPhase Phase() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return lifecycle_.Phase();
    }

    bool ConsumeFailure(SubclassPhase& failedPhase, DWORD& errorCode) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!lifecycle_.ConsumeFailure()) {
            return false;
        }
        failedPhase = lifecycle_.Phase();
        errorCode = lastError_;
        return true;
    }

    void SetPublicationProbeForTesting(
        PublicationProbe probe,
        void* context
    ) {
        std::lock_guard<std::mutex> lock(mutex_);
        publicationProbe_ = probe;
        publicationProbeContext_ = context;
    }

private:
    static UINT ControlMessage() {
        static const UINT message =
            RegisterWindowMessageW(L"FalcomBGMInfo.OwnerThreadSubclass.v1");
        return message;
    }

    static LRESULT CALLBACK MarshalHook(
        int code,
        WPARAM wParam,
        LPARAM lParam
    ) {
        OwnerThreadSubclass* controller = activeController_.load();
        HHOOK hook = controller ? controller->HookSnapshot() : nullptr;
        const LRESULT nextResult =
            CallNextHookEx(hook, code, wParam, lParam);

        if (controller && code >= 0 && lParam) {
            const CWPSTRUCT* call =
                reinterpret_cast<const CWPSTRUCT*>(lParam);
            if (call->message == WM_NCDESTROY &&
                controller->TargetsWindow(call->hwnd)) {
                controller->NotifyDestroyed(call->hwnd);
            } else if (call->message == ControlMessage() &&
                       controller->TargetsWindow(call->hwnd)) {
                controller->ExecutePendingCommand(call->hwnd);
            }
        }
        return nextResult;
    }

    HHOOK HookSnapshot() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return marshalHook_;
    }

    bool TargetsWindow(HWND window) const {
        std::lock_guard<std::mutex> lock(mutex_);
        return window_ == window;
    }

    void DispatchPendingCommand(DWORD ownerThread) {
        if (ownerThread == GetCurrentThreadId()) {
            ExecutePendingCommand(window_);
            return;
        }

        const UINT controlMessage = ControlMessage();
        if (controlMessage == 0) {
            FailPendingCommand(GetLastError(), false);
            return;
        }

        bool requiresMarshalHook = false;
        HWND window = nullptr;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            requiresMarshalHook =
                lifecycle_.Phase() == SubclassPhase::InstallQueued;
            window = window_;
        }
        if (!requiresMarshalHook) {
            if (!SendNotifyMessageW(window, controlMessage, 0, 0)) {
                FailPendingCommand(GetLastError(), false);
            }
            return;
        }

        OwnerThreadSubclass* expected = nullptr;
        if (!activeController_.compare_exchange_strong(expected, this)) {
            FailPendingCommand(ERROR_BUSY, false);
            return;
        }

        // The owner thread belongs to this process, so hMod must be null.
        HHOOK hook = SetWindowsHookExW(
            WH_CALLWNDPROC,
            MarshalHook,
            nullptr,
            ownerThread
        );
        if (!hook) {
            FailPendingCommand(GetLastError(), true);
            return;
        }

        {
            std::lock_guard<std::mutex> lock(mutex_);
            marshalHook_ = hook;
            window = window_;
        }

        if (!SendNotifyMessageW(window, controlMessage, 0, 0)) {
            const DWORD errorCode = GetLastError();
            FailPendingCommand(errorCode, true);
        }
    }

    void ExecutePendingCommand(HWND messageWindow) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (messageWindow != window_) {
            return;
        }

        const SubclassPhase phase = lifecycle_.Phase();
        bool succeeded = true;
        if (phase == SubclassPhase::InstallQueued &&
            !lifecycle_.InstallWasCancelled()) {
            SetLastError(ERROR_SUCCESS);
            succeeded =
                SetWindowSubclass(
                    window_,
                    procedure_,
                    subclassId_,
                    0) != FALSE;
            lastError_ =
                succeeded ? ERROR_SUCCESS : GetLastError();
        } else if (phase == SubclassPhase::RemoveQueued) {
            SetLastError(ERROR_SUCCESS);
            succeeded =
                RemoveWindowSubclass(
                    window_,
                    procedure_,
                    subclassId_) != FALSE;
            lastError_ =
                succeeded ? ERROR_SUCCESS : GetLastError();
        } else if (phase != SubclassPhase::InstallQueued) {
            return;
        }

        ReleaseCommandSlotLocked();
        RunPublicationProbeLocked();
        if (phase == SubclassPhase::InstallQueued) {
            lifecycle_.CompleteInstall(succeeded);
        } else {
            lifecycle_.CompleteRemove(succeeded);
        }
    }

    void FailPendingCommand(
        DWORD errorCode,
        bool ownsCommandSlot
    ) {
        std::lock_guard<std::mutex> lock(mutex_);
        lastError_ = errorCode;
        if (ownsCommandSlot) {
            ReleaseCommandSlotLocked();
        }
        RunPublicationProbeLocked();
        if (lifecycle_.Phase() == SubclassPhase::InstallQueued) {
            lifecycle_.CompleteInstall(false);
        } else if (lifecycle_.Phase() == SubclassPhase::RemoveQueued) {
            lifecycle_.CompleteRemove(false);
        }
    }

    void ReleaseCommandSlotLocked() {
        if (marshalHook_) {
            UnhookWindowsHookEx(marshalHook_);
            marshalHook_ = nullptr;
        }
        OwnerThreadSubclass* expected = this;
        activeController_.compare_exchange_strong(
            expected,
            nullptr
        );
    }

    void RunPublicationProbeLocked() {
        PublicationProbe probe = publicationProbe_;
        void* context = publicationProbeContext_;
        publicationProbe_ = nullptr;
        publicationProbeContext_ = nullptr;
        if (probe) {
            probe(context);
        }
    }

    void NotifyDestroyedOnOwnerThread(
        HWND window,
        DWORD ownerThread
    ) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (reinterpret_cast<std::uintptr_t>(window) !=
                lifecycle_.Window() ||
            ownerThread != lifecycle_.OwnerThread()) {
            return;
        }

        const SubclassPhase phase = lifecycle_.Phase();
        if ((phase == SubclassPhase::Installed ||
             phase == SubclassPhase::RemoveQueued) &&
            procedure_) {
            RemoveWindowSubclass(
                window,
                procedure_,
                subclassId_
            );
        }
        ReleaseCommandSlotLocked();
        lifecycle_.NotifyDestroyed(
            reinterpret_cast<std::uintptr_t>(window),
            ownerThread
        );
        window_ = nullptr;
    }

    mutable std::mutex mutex_;
    SubclassLifecycle lifecycle_;
    HWND window_ = nullptr;
    HMODULE module_ = nullptr;
    SUBCLASSPROC procedure_ = nullptr;
    UINT_PTR subclassId_ = 0;
    HHOOK marshalHook_ = nullptr;
    DWORD lastError_ = ERROR_SUCCESS;
    PublicationProbe publicationProbe_ = nullptr;
    void* publicationProbeContext_ = nullptr;

    inline static std::atomic<OwnerThreadSubclass*> activeController_ = nullptr;
};

}
