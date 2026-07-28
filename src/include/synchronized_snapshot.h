#pragma once

#include <mutex>
#include <utility>

template <typename T>
class SynchronizedSnapshot {
public:
    SynchronizedSnapshot() = default;

    explicit SynchronizedSnapshot(T value)
        : m_value(std::move(value)) {
    }

    T Load() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_value;
    }

    void Store(const T& value) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_value = value;
    }

private:
    mutable std::mutex m_mutex;
    T m_value{};
};
