#pragma once
#include <cstdint>

namespace core::memory {

class Address {
    uintptr_t m_addr = 0;
public:
    Address() = default;
    Address(uintptr_t addr) : m_addr(addr) {}
    Address(void* ptr) : m_addr(reinterpret_cast<uintptr_t>(ptr)) {}

    Address Add(ptrdiff_t offset) const { return m_addr + offset; }
    Address Sub(ptrdiff_t offset) const { return m_addr - offset; }

    Address Rip() const {
        auto rel = *reinterpret_cast<int32_t*>(m_addr);
        return m_addr + rel + 4;
    }

    Address Deref() const {
        return *reinterpret_cast<uintptr_t*>(m_addr);
    }

    template<typename T>
    T As() const { return reinterpret_cast<T>(m_addr); }

    uintptr_t Raw() const { return m_addr; }
    operator uintptr_t() const { return m_addr; }
    operator bool() const { return m_addr != 0; }

    bool operator==(const Address& o) const { return m_addr == o.m_addr; }
    bool operator!=(const Address& o) const { return m_addr != o.m_addr; }
    bool operator<(const Address& o) const { return m_addr < o.m_addr; }
};

} // namespace core::memory
