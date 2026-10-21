#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <bcrypt.h>
#include <cstdio>
#include <cstdint>
#include <vector>
#include <string>

#pragma comment(lib, "bcrypt.lib")

static bool ReadFile( const char* path, std::vector<uint8_t>& out )
{
    FILE* f = fopen( path, "rb" );
    if ( !f ) return false;
    fseek( f, 0, SEEK_END );
    long sz = ftell( f );
    fseek( f, 0, SEEK_SET );
    out.resize( sz );
    fread( out.data(), 1, sz, f );
    fclose( f );
    return sz > 0;
}

static bool GenRandom( uint8_t* buf, size_t len )
{
    return BCryptGenRandom( nullptr, buf, (ULONG)len, BCRYPT_USE_SYSTEM_PREFERRED_RNG ) == 0;
}

int main( int argc, char* argv[] )
{
    if ( argc < 2 )
    {
        printf( "Usage: embed_dll.exe <dll_path> [output.hpp]\n" );
        return 1;
    }

    const char* dllPath = argv[1];
    const char* outPath = argc > 2 ? argv[2] : "payload.hpp";

    std::vector<uint8_t> raw;
    if ( !ReadFile( dllPath, raw ) )
    {
        printf( "[!] Failed to read %s\n", dllPath );
        return 1;
    }

    uint8_t key[32];
    if ( !GenRandom( key, 32 ) )
    {
        printf( "[!] Failed to generate random key\n" );
        return 1;
    }

    std::vector<uint8_t> enc( raw.size() );
    for ( size_t i = 0; i < raw.size(); i++ )
        enc[i] = raw[i] ^ key[i % 32];

    FILE* f = fopen( outPath, "w" );
    if ( !f )
    {
        printf( "[!] Failed to open %s for writing\n", outPath );
        return 1;
    }

    fprintf( f, "#pragma once\n" );
    fprintf( f, "#include <cstdint>\n\n" );
    fprintf( f, "inline constexpr uint32_t g_PayloadSize = %uu;\n\n", (unsigned)raw.size() );

    fprintf( f, "inline constexpr uint8_t g_PayloadKey[32] = {\n    " );
    for ( int i = 0; i < 32; i++ )
    {
        fprintf( f, "0x%02X", key[i] );
        if ( i < 31 ) fprintf( f, ", " );
        if ( (i + 1) % 16 == 0 && i < 31 ) fprintf( f, "\n    " );
    }
    fprintf( f, "\n};\n\n" );

    fprintf( f, "inline const uint8_t g_PayloadData[] = {\n    " );
    for ( size_t i = 0; i < enc.size(); i++ )
    {
        fprintf( f, "0x%02X", enc[i] );
        if ( i < enc.size() - 1 ) fprintf( f, "," );
        if ( (i + 1) % 24 == 0 ) fprintf( f, "\n    " );
    }
    fprintf( f, "\n};\n" );

    fclose( f );

    printf( "Embedded %zu bytes (%.1f KB) -> %s\n", raw.size(), raw.size() / 1024.0, outPath );
    printf( "XOR key: " );
    for ( int i = 0; i < 32; i++ ) printf( "%02x", key[i] );
    printf( "\n" );

    return 0;
}
