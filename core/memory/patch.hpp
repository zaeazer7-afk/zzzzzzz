#pragma once
#include <cstdint>
#include <vector>
#include <Windows.h>

namespace core::memory {

class Patch {
    uintptr_t m_addr;
    std::vector<uint8_t> m_original;
    size_t m_size;
public:
    Patch(uintptr_t addr, const void* bytes, size_t size)
        : m_addr(addr), m_size(size) {
        m_original.resize(size);
        memcpy(m_original.data(), reinterpret_cast<void*>(addr), size);

        DWORD old_protect;
        VirtualProtect(reinterpret_cast<void*>(addr), size, PAGE_EXECUTE_READWRITE, &old_protect);
        memcpy(reinterpret_cast<void*>(addr), bytes, size);
        VirtualProtect(reinterpret_cast<void*>(addr), size, old_protect, &old_protect);
    }

    void Restore() {
        DWORD old_protect;
        VirtualProtect(reinterpret_cast<void*>(m_addr), m_size, PAGE_EXECUTE_READWRITE, &old_protect);
        memcpy(reinterpret_cast<void*>(m_addr), m_original.data(), m_size);
        VirtualProtect(reinterpret_cast<void*>(m_addr), m_size, old_protect, &old_protect);
    }

    static void Write(uintptr_t addr, const void* bytes, size_t size) {
        DWORD old_protect;
        VirtualProtect(reinterpret_cast<void*>(addr), size, PAGE_EXECUTE_READWRITE, &old_protect);
        memcpy(reinterpret_cast<void*>(addr), bytes, size);
        VirtualProtect(reinterpret_cast<void*>(addr), size, old_protect, &old_protect);
    }

    template<typename T>
    static void Write(uintptr_t addr, T value) {
        Write(addr, &value, sizeof(T));
    }
};

} // namespace core::memory
