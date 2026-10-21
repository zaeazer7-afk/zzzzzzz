#pragma once
#include <cstdint>
#include <string>
#include <Windows.h>

namespace core::memory {

class Module {
    uintptr_t m_base = 0;
    size_t m_size = 0;
    std::string m_name;
public:
    Module() = default;

    Module(const std::string& name) : m_name(name) {
        auto handle = GetModuleHandleA(name.c_str());
        if (!handle) return;
        m_base = reinterpret_cast<uintptr_t>(handle);
        auto dos = reinterpret_cast<IMAGE_DOS_HEADER*>(m_base);
        auto nt = reinterpret_cast<IMAGE_NT_HEADERS*>(m_base + dos->e_lfanew);
        m_size = nt->OptionalHeader.SizeOfImage;
    }

    uintptr_t Base() const { return m_base; }
    size_t Size() const { return m_size; }
    const std::string& Name() const { return m_name; }
    operator bool() const { return m_base != 0; }
};

} // namespace core::memory
