#pragma once

#include <cstddef>
#include <cstdint>
#include <array>

namespace Utils
{
    template <size_t N, uint32_t Key>
    class XorStr
    {
    public:
        constexpr XorStr( const char( &str )[N] )
        {
            for ( size_t i = 0; i < N; i++ )
                m_data[i] = str[i] ^ static_cast<char>( ( Key + i * 37 ) & 0xFF );
        }

        const char* Decrypt() const
        {
            for ( size_t i = 0; i < N; i++ )
                m_decrypted[i] = m_data[i] ^ static_cast<char>( ( Key + i * 37 ) & 0xFF );
            m_decrypted[N - 1] = '\0';
            return m_decrypted.data();
        }

        operator const char*() const { return Decrypt(); }

    private:
        std::array<char, N> m_data{};
        mutable std::array<char, N> m_decrypted{};
    };
}

#define XS( str ) ( Utils::XorStr<sizeof(str), __COUNTER__ * 2654435761u + __LINE__>( str ).Decrypt() )
