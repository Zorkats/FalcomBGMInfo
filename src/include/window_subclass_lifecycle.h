#pragma once

#include <cstdint>

namespace bgm_window {

enum class SubclassPhase {
    Detached,
    InstallQueued,
    Installed,
    RemoveQueued,
    Removed,
    InstallFailed,
    RemoveFailed,
    Destroyed
};

class SubclassLifecycle {
public:
    bool BeginInstall(
        std::uintptr_t window,
        std::uint32_t ownerThread,
        std::uintptr_t ownerToken
    ) {
        if (window == 0 ||
            ownerThread == 0 ||
            ownerToken == 0) {
            return false;
        }

        const bool sameIdentity =
            window_ == window &&
            ownerThread_ == ownerThread &&
            ownerToken_ == ownerToken;
        if (sameIdentity &&
            (phase_ == SubclassPhase::InstallQueued ||
             phase_ == SubclassPhase::Installed ||
             phase_ == SubclassPhase::RemoveQueued ||
             phase_ == SubclassPhase::InstallFailed ||
             phase_ == SubclassPhase::RemoveFailed ||
             phase_ == SubclassPhase::Destroyed)) {
            return false;
        }

        if (phase_ != SubclassPhase::Detached &&
            phase_ != SubclassPhase::Removed &&
            !(phase_ == SubclassPhase::InstallFailed && !sameIdentity) &&
            !(phase_ == SubclassPhase::Destroyed && !sameIdentity)) {
            return false;
        }

        window_ = window;
        ownerThread_ = ownerThread;
        ownerToken_ = ownerToken;
        phase_ = SubclassPhase::InstallQueued;
        cancelInstall_ = false;
        failureReported_ = false;
        return true;
    }

    bool BeginRemove() {
        switch (phase_) {
        case SubclassPhase::InstallQueued:
            cancelInstall_ = true;
            return true;
        case SubclassPhase::Installed:
            phase_ = SubclassPhase::RemoveQueued;
            failureReported_ = false;
            return true;
        default:
            return false;
        }
    }

    bool RejectInstall(
        std::uintptr_t window,
        std::uint32_t ownerThread,
        std::uintptr_t ownerToken
    ) {
        if (window == 0 ||
            ownerToken == 0 ||
            (phase_ != SubclassPhase::Detached &&
             phase_ != SubclassPhase::Removed &&
             phase_ != SubclassPhase::InstallFailed &&
             phase_ != SubclassPhase::Destroyed)) {
            return false;
        }
        window_ = window;
        ownerThread_ = ownerThread;
        ownerToken_ = ownerToken;
        phase_ = SubclassPhase::InstallFailed;
        cancelInstall_ = false;
        failureReported_ = false;
        return true;
    }

    void CompleteInstall(bool success) {
        if (phase_ != SubclassPhase::InstallQueued) {
            return;
        }
        if (cancelInstall_) {
            phase_ = SubclassPhase::Removed;
            cancelInstall_ = false;
            return;
        }
        phase_ = success ? SubclassPhase::Installed : SubclassPhase::InstallFailed;
    }

    void CompleteRemove(bool success) {
        if (phase_ != SubclassPhase::RemoveQueued) {
            return;
        }
        phase_ = success ? SubclassPhase::Removed : SubclassPhase::RemoveFailed;
    }

    void NotifyDestroyed(std::uintptr_t window, std::uint32_t ownerThread) {
        if (window_ == window && ownerThread_ == ownerThread) {
            phase_ = SubclassPhase::Destroyed;
            window_ = 0;
            ownerThread_ = 0;
            ownerToken_ = 0;
            cancelInstall_ = false;
            failureReported_ = false;
        }
    }

    bool ConsumeFailure() {
        const bool failed =
            phase_ == SubclassPhase::InstallFailed ||
            phase_ == SubclassPhase::RemoveFailed;
        if (!failed || failureReported_) {
            return false;
        }
        failureReported_ = true;
        return true;
    }

    bool IsInstalledFor(
        std::uintptr_t window,
        std::uint32_t ownerThread,
        std::uintptr_t ownerToken
    ) const {
        return
            phase_ == SubclassPhase::Installed &&
            window_ == window &&
            ownerThread_ == ownerThread &&
            ownerToken_ == ownerToken;
    }

    bool IsInstallBlockedFor(
        std::uintptr_t window,
        std::uint32_t ownerThread,
        std::uintptr_t ownerToken
    ) const {
        const bool sameIdentity =
            window_ == window &&
            ownerThread_ == ownerThread &&
            ownerToken_ == ownerToken;
        return
            sameIdentity &&
            (phase_ == SubclassPhase::InstallFailed ||
             phase_ == SubclassPhase::RemoveFailed ||
             phase_ == SubclassPhase::Destroyed);
    }

    bool CanDisposeRenderer() const {
        return
            phase_ == SubclassPhase::Detached ||
            phase_ == SubclassPhase::Removed ||
            phase_ == SubclassPhase::InstallFailed ||
            phase_ == SubclassPhase::Destroyed;
    }

    bool InstallWasCancelled() const {
        return phase_ == SubclassPhase::InstallQueued && cancelInstall_;
    }

    SubclassPhase Phase() const {
        return phase_;
    }

    std::uintptr_t Window() const {
        return window_;
    }

    std::uint32_t OwnerThread() const {
        return ownerThread_;
    }

    std::uintptr_t OwnerToken() const {
        return ownerToken_;
    }

private:
    SubclassPhase phase_ = SubclassPhase::Detached;
    std::uintptr_t window_ = 0;
    std::uint32_t ownerThread_ = 0;
    std::uintptr_t ownerToken_ = 0;
    bool cancelInstall_ = false;
    bool failureReported_ = false;
};

}
