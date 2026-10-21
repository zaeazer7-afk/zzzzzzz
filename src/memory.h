#pragma once
#include "pch.h"

namespace mem {

    inline u64 baseAddr = 0;
    inline u64 baseSize = 0;

    inline u64 gameBase() { return baseAddr; }

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

    inline u64 rip(u64 addr, i32 offset = 0, i32 instrLen = 0) {
        if (!addr) return 0;
        if (offset == 0 && instrLen == 0) {
            i32 rel = read<i32>(addr + 3);
            return addr + rel + 7;
        }
        i32 rel = read<i32>(addr + offset);
        return addr + rel + instrLen;
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

    inline void nop(u64 addr, size_t size) {
        DWORD old;
        VirtualProtect((void*)addr, size, PAGE_EXECUTE_READWRITE, &old);
        memset((void*)addr, 0x90, size);
        VirtualProtect((void*)addr, size, old, &old);
    }

    inline void erasePEHeaders(HMODULE hMod) {
        auto dos = (PIMAGE_DOS_HEADER)hMod;
        auto nt = (PIMAGE_NT_HEADERS)((u8*)hMod + dos->e_lfanew);
        DWORD old;
        auto headerSize = nt->OptionalHeader.SizeOfHeaders;
        VirtualProtect(hMod, headerSize, PAGE_READWRITE, &old);
        memset(hMod, 0, headerSize);
        VirtualProtect(hMod, headerSize, old, &old);
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

} // namespace mem
