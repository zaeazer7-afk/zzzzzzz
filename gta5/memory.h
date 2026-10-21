#pragma once
#include "pch.h"

namespace mem {

    inline u64 baseAddr = 0;
    inline u64 baseSize = 0;

    inline void init() {
        auto hMod = GetModuleHandleA(nullptr);
        baseAddr = (u64)hMod;
        auto dos = (IMAGE_DOS_HEADER*)baseAddr;
        auto nt  = (IMAGE_NT_HEADERS*)(baseAddr + dos->e_lfanew);
        baseSize = nt->OptionalHeader.SizeOfImage;
    }

    template<typename T>
    inline T read(u64 addr) {
        if (!addr) return T{};
        return *reinterpret_cast<T*>(addr);
    }

    template<typename T>
    inline void write(u64 addr, T value) {
        if (!addr) return;
        *reinterpret_cast<T*>(addr) = value;
    }

    inline u64 rip(u64 addr) {
        if (!addr) return 0;
        i32 rel = read<i32>(addr);
        return addr + rel + 4;
    }

    inline void patch(u64 addr, const void* bytes, size_t size) {
        DWORD old;
        VirtualProtect((void*)addr, size, PAGE_EXECUTE_READWRITE, &old);
        memcpy((void*)addr, bytes, size);
        VirtualProtect((void*)addr, size, old, &old);
    }

    template<typename T>
    inline void patch(u64 addr, T value) {
        patch(addr, &value, sizeof(T));
    }

    inline u64 scanIDA(const char* pattern, u64 base = 0, u64 size = 0) {
        if (!base) { base = baseAddr; size = baseSize; }
        if (!base || !size) return 0;

        struct Elem { u8 val; bool wild; };
        std::vector<Elem> elems;

        for (const char* p = pattern; *p; ) {
            if (*p == ' ') { p++; continue; }
            if (*p == '?') {
                elems.push_back({0, true});
                p += (p[1] == '?') ? 2 : 1;
            } else {
                char hex[3] = { p[0], p[1], 0 };
                elems.push_back({(u8)strtoul(hex, nullptr, 16), false});
                p += 2;
            }
        }
        if (elems.empty()) return 0;

        auto data = (const u8*)base;
        size_t patLen = elems.size();

        for (size_t i = 0; i <= size - patLen; i++) {
            bool found = true;
            for (size_t j = 0; j < patLen; j++) {
                if (!elems[j].wild && data[i + j] != elems[j].val) {
                    found = false;
                    break;
                }
            }
            if (found) return base + i;
        }
        return 0;
    }

    inline std::vector<u64> scanAll(const char* pattern, u64 base = 0, u64 size = 0) {
        if (!base) { base = baseAddr; size = baseSize; }
        std::vector<u64> results;

        struct Elem { u8 val; bool wild; };
        std::vector<Elem> elems;

        for (const char* p = pattern; *p; ) {
            if (*p == ' ') { p++; continue; }
            if (*p == '?') {
                elems.push_back({0, true});
                p += (p[1] == '?') ? 2 : 1;
            } else {
                char hex[3] = { p[0], p[1], 0 };
                elems.push_back({(u8)strtoul(hex, nullptr, 16), false});
                p += 2;
            }
        }
        if (elems.empty()) return results;

        auto data = (const u8*)base;
        size_t patLen = elems.size();

        for (size_t i = 0; i <= size - patLen; i++) {
            bool found = true;
            for (size_t j = 0; j < patLen; j++) {
                if (!elems[j].wild && data[i + j] != elems[j].val) {
                    found = false;
                    break;
                }
            }
            if (found) results.push_back(base + i);
        }
        return results;
    }

} // namespace mem
