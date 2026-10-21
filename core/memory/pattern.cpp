#include "pattern.hpp"
#include <cstring>

namespace core::memory {

std::vector<Pattern::Byte> Pattern::Parse(const char* pattern) {
    std::vector<Byte> bytes;
    const char* p = pattern;
    while (*p) {
        if (*p == ' ') { p++; continue; }
        if (*p == '?') {
            bytes.push_back({0, true});
            p++;
            if (*p == '?') p++;
            continue;
        }
        auto hexChar = [](char c) -> uint8_t {
            if (c >= '0' && c <= '9') return c - '0';
            if (c >= 'A' && c <= 'F') return c - 'A' + 10;
            if (c >= 'a' && c <= 'f') return c - 'a' + 10;
            return 0;
        };
        uint8_t val = (hexChar(p[0]) << 4) | hexChar(p[1]);
        bytes.push_back({val, false});
        p += 2;
    }
    return bytes;
}

void* Pattern::Scan(const uint8_t* data, size_t size, const std::vector<Byte>& pattern) {
    size_t patLen = pattern.size();
    if (patLen == 0 || patLen > size) return nullptr;

    size_t limit = size - patLen;
    for (size_t i = 0; i <= limit; i++) {
        bool found = true;
        for (size_t j = 0; j < patLen; j++) {
            if (!pattern[j].wildcard && data[i + j] != pattern[j].value) {
                found = false;
                break;
            }
        }
        if (found) return const_cast<uint8_t*>(data + i);
    }
    return nullptr;
}

void* Pattern::Scan(const uint8_t* data, size_t size, const char* pattern) {
    return Scan(data, size, Parse(pattern));
}

} // namespace core::memory
