#include "module.hpp"
#include "pattern.hpp"
#include <cstring>

namespace core::memory {

Module Module::s_Instance{};

Module& Module::Get() {
    return s_Instance;
}

bool Module::Init(const char* moduleName) {
    HMODULE hMod = GetModuleHandleA(moduleName);
    if (!hMod) return false;

    auto* dosHeader = reinterpret_cast<IMAGE_DOS_HEADER*>(hMod);
    auto* ntHeaders = reinterpret_cast<IMAGE_NT_HEADERS*>(reinterpret_cast<uint8_t*>(hMod) + dosHeader->e_lfanew);

    s_Instance.m_Base = reinterpret_cast<uintptr_t>(hMod);
    s_Instance.m_Size = ntHeaders->OptionalHeader.SizeOfImage;
    strncpy_s(s_Instance.m_Name, moduleName, sizeof(s_Instance.m_Name) - 1);

    return true;
}

void Module::Shutdown() {
    s_Instance = {};
}

void* Module::Scan(const char* pattern) const {
    if (!m_Base || !m_Size) return nullptr;
    return Pattern::Scan(reinterpret_cast<const uint8_t*>(m_Base), m_Size, pattern);
}

} // namespace core::memory
