#pragma once

#include <algorithm>
#include <cstddef>
#include <string>

namespace bgm_path {

template <typename CharT>
size_t Length(const CharT* value) {
    if (!value) {
        return 0;
    }

    size_t length = 0;
    while (value[length] != static_cast<CharT>(0)) {
        ++length;
    }
    return length;
}

template <typename CharT>
CharT ToLowerAscii(CharT value) {
    if (value >= static_cast<CharT>('A') && value <= static_cast<CharT>('Z')) {
        return static_cast<CharT>(value + (static_cast<CharT>('a') - static_cast<CharT>('A')));
    }
    return value;
}

template <typename CharT>
bool EndsWithIgnoreCase(const CharT* value, const CharT* suffix) {
    if (!value || !suffix) {
        return false;
    }

    const size_t valueLength = Length(value);
    const size_t suffixLength = Length(suffix);

    if (suffixLength > valueLength) {
        return false;
    }

    const size_t offset = valueLength - suffixLength;
    for (size_t index = 0; index < suffixLength; ++index) {
        if (ToLowerAscii(value[offset + index]) != ToLowerAscii(suffix[index])) {
            return false;
        }
    }

    return true;
}

inline std::string NormalizeLookupPath(std::string path) {
    std::replace(path.begin(), path.end(), '/', '\\');
    return path;
}

inline bool HasExactSuffix(const std::string& value, const std::string& suffix) {
    return value.length() >= suffix.length() &&
           value.compare(value.length() - suffix.length(), suffix.length(), suffix) == 0;
}

inline bool IsModernAudioFile(const char* filename, bool useWav, bool useOpus) {
    if (Length(filename) < 5) {
        return false;
    }
    if (useWav && EndsWithIgnoreCase(filename, ".wav")) {
        return true;
    }
    return useOpus &&
           (EndsWithIgnoreCase(filename, ".opus") ||
            EndsWithIgnoreCase(filename, ".ogg"));
}

inline bool IsModernAudioFile(const wchar_t* filename, bool useWav, bool useOpus) {
    if (Length(filename) < 5) {
        return false;
    }
    if (useWav && EndsWithIgnoreCase(filename, L".wav")) {
        return true;
    }
    return useOpus &&
           (EndsWithIgnoreCase(filename, L".opus") ||
            EndsWithIgnoreCase(filename, L".ogg"));
}

inline bool IsRetroAudioFile(const char* filename) {
    return Length(filename) >= 4 &&
           (EndsWithIgnoreCase(filename, ".ogg") ||
            EndsWithIgnoreCase(filename, ".wav"));
}

inline bool IsRetroAudioFile(const wchar_t* filename) {
    return Length(filename) >= 4 &&
           (EndsWithIgnoreCase(filename, L".ogg") ||
            EndsWithIgnoreCase(filename, L".wav"));
}

}
