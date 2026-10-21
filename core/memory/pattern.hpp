#pragma once
#include <cstdint>
#include <vector>
#include <string>
#include <optional>
#include <Windows.h>

namespace core::memory {

class Pattern {
public:
    struct Element {
        uint8_t value;
        bool wildcard;
    };

    static std::vector<Element> Parse(const std::string& pattern) {
        std::vector<Element> elems;
        for (size_t i = 0; i < pattern.size(); ) {
            if (pattern[i] == ' ') { ++i; continue; }
            if (pattern[i] == '?') {
                elems.push_back({0, true});
                i += (i + 1 < pattern.size() && pattern[i + 1] == '?') ? 2 : 1;
            } else {
                auto byte = static_cast<uint8_t>(std::stoul(pattern.substr(i, 2), nullptr, 16));
                elems.push_back({byte, false});
                i += 2;
            }
        }
        return elems;
    }

    static std::optional<uintptr_t> Scan(const std::string& pattern, uintptr_t base, size_t size) {
        auto elems = Parse(pattern);
        if (elems.empty()) return std::nullopt;

        auto data = reinterpret_cast<const uint8_t*>(base);
        size_t patLen = elems.size();

        for (size_t i = 0; i <= size - patLen; ++i) {
            bool found = true;
            for (size_t j = 0; j < patLen; ++j) {
                if (!elems[j].wildcard && data[i + j] != elems[j].value) {
                    found = false;
                    break;
                }
            }
            if (found) return base + i;
        }
        return std::nullopt;
    }

    static std::vector<uintptr_t> ScanAll(const std::string& pattern, uintptr_t base, size_t size) {
        std::vector<uintptr_t> results;
        auto elems = Parse(pattern);
        if (elems.empty()) return results;

        auto data = reinterpret_cast<const uint8_t*>(base);
        size_t patLen = elems.size();

        for (size_t i = 0; i <= size - patLen; ++i) {
            bool found = true;
            for (size_t j = 0; j < patLen; ++j) {
                if (!elems[j].wildcard && data[i + j] != elems[j].value) {
                    found = false;
                    break;
                }
            }
            if (found) results.push_back(base + i);
        }
        return results;
    }
};

} // namespace core::memory
