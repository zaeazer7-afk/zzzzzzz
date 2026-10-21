#pragma once

#include <Windows.h>
#include <ShlObj.h>
#include <string>
#include <unordered_map>
#include <fstream>
#include <sstream>
#include <mutex>
#include <filesystem>

namespace Core
{
    class Config
    {
    public:
        static Config& Get()
        {
            static Config inst;
            return inst;
        }

        void Init()
        {
            m_path = GetConfigPath();
            std::filesystem::create_directories( std::filesystem::path( m_path ).parent_path() );
            Load();
        }

        // ── Getters ──

        bool GetBool( const std::string& section, const std::string& key, bool def = false ) const
        {
            auto v = GetRaw( section, key );
            if ( v.empty() ) return def;
            return v == "1" || v == "true";
        }

        int GetInt( const std::string& section, const std::string& key, int def = 0 ) const
        {
            auto v = GetRaw( section, key );
            if ( v.empty() ) return def;
            try { return std::stoi( v ); } catch ( ... ) { return def; }
        }

        float GetFloat( const std::string& section, const std::string& key, float def = 0.f ) const
        {
            auto v = GetRaw( section, key );
            if ( v.empty() ) return def;
            try { return std::stof( v ); } catch ( ... ) { return def; }
        }

        std::string GetString( const std::string& section, const std::string& key, const std::string& def = "" ) const
        {
            auto v = GetRaw( section, key );
            return v.empty() ? def : v;
        }

        // ── Setters ──

        void SetBool( const std::string& section, const std::string& key, bool val )
        {
            SetRaw( section, key, val ? "1" : "0" );
        }

        void SetInt( const std::string& section, const std::string& key, int val )
        {
            SetRaw( section, key, std::to_string( val ) );
        }

        void SetFloat( const std::string& section, const std::string& key, float val )
        {
            std::ostringstream ss;
            ss << val;
            SetRaw( section, key, ss.str() );
        }

        void SetString( const std::string& section, const std::string& key, const std::string& val )
        {
            SetRaw( section, key, val );
        }

        // ── IO ──

        void Load()
        {
            std::lock_guard<std::mutex> lk( m_mtx );
            m_data.clear();

            std::ifstream f( m_path );
            if ( !f.is_open() ) return;

            std::string currentSection = "General";
            std::string line;
            while ( std::getline( f, line ) )
            {
                if ( line.empty() || line[0] == '#' || line[0] == ';' ) continue;

                if ( line[0] == '[' )
                {
                    auto end = line.find( ']' );
                    if ( end != std::string::npos )
                        currentSection = line.substr( 1, end - 1 );
                    continue;
                }

                auto eq = line.find( '=' );
                if ( eq == std::string::npos ) continue;

                std::string key = Trim( line.substr( 0, eq ) );
                std::string val = Trim( line.substr( eq + 1 ) );
                m_data[currentSection + "." + key] = val;
            }
        }

        void Save()
        {
            std::lock_guard<std::mutex> lk( m_mtx );

            // Group by section.
            std::unordered_map<std::string, std::vector<std::pair<std::string, std::string>>> sections;
            for ( auto& [fullKey, val] : m_data )
            {
                auto dot = fullKey.find( '.' );
                std::string sec = ( dot != std::string::npos ) ? fullKey.substr( 0, dot ) : "General";
                std::string key = ( dot != std::string::npos ) ? fullKey.substr( dot + 1 ) : fullKey;
                sections[sec].push_back( { key, val } );
            }

            std::ofstream f( m_path );
            if ( !f.is_open() ) return;

            f << "; Trident Internal Configuration\n\n";
            for ( auto& [sec, kvs] : sections )
            {
                f << "[" << sec << "]\n";
                for ( auto& [k, v] : kvs )
                    f << k << " = " << v << "\n";
                f << "\n";
            }
        }

        // ── Bind helpers for ImGui toggles ──

        bool* BindBool( const std::string& section, const std::string& key, bool def = false )
        {
            std::string full = section + "." + key;
            if ( m_boolCache.find( full ) == m_boolCache.end() )
                m_boolCache[full] = GetBool( section, key, def );
            return &m_boolCache[full];
        }

        float* BindFloat( const std::string& section, const std::string& key, float def = 0.f )
        {
            std::string full = section + "." + key;
            if ( m_floatCache.find( full ) == m_floatCache.end() )
                m_floatCache[full] = GetFloat( section, key, def );
            return &m_floatCache[full];
        }

        int* BindInt( const std::string& section, const std::string& key, int def = 0 )
        {
            std::string full = section + "." + key;
            if ( m_intCache.find( full ) == m_intCache.end() )
                m_intCache[full] = GetInt( section, key, def );
            return &m_intCache[full];
        }

        void SyncToFile()
        {
            for ( auto& [full, val] : m_boolCache )
            {
                auto dot = full.find( '.' );
                SetRaw( full.substr( 0, dot ), full.substr( dot + 1 ), val ? "1" : "0" );
            }
            for ( auto& [full, val] : m_floatCache )
            {
                auto dot = full.find( '.' );
                std::ostringstream ss; ss << val;
                SetRaw( full.substr( 0, dot ), full.substr( dot + 1 ), ss.str() );
            }
            for ( auto& [full, val] : m_intCache )
            {
                auto dot = full.find( '.' );
                SetRaw( full.substr( 0, dot ), full.substr( dot + 1 ), std::to_string( val ) );
            }
            Save();
        }

    private:
        Config() = default;

        std::string GetConfigPath()
        {
            char appdata[MAX_PATH];
            if ( SUCCEEDED( SHGetFolderPathA( nullptr, CSIDL_APPDATA, nullptr, 0, appdata ) ) )
                return std::string( appdata ) + "\\Trident\\config.ini";
            return "trident_config.ini";
        }

        std::string GetRaw( const std::string& section, const std::string& key ) const
        {
            std::lock_guard<std::mutex> lk( m_mtx );
            auto it = m_data.find( section + "." + key );
            return ( it != m_data.end() ) ? it->second : "";
        }

        void SetRaw( const std::string& section, const std::string& key, const std::string& val )
        {
            std::lock_guard<std::mutex> lk( m_mtx );
            m_data[section + "." + key] = val;
        }

        static std::string Trim( const std::string& s )
        {
            auto start = s.find_first_not_of( " \t\r\n" );
            if ( start == std::string::npos ) return "";
            auto end = s.find_last_not_of( " \t\r\n" );
            return s.substr( start, end - start + 1 );
        }

        mutable std::mutex m_mtx;
        std::string        m_path;
        std::unordered_map<std::string, std::string> m_data;
        std::unordered_map<std::string, bool>        m_boolCache;
        std::unordered_map<std::string, float>       m_floatCache;
        std::unordered_map<std::string, int>         m_intCache;
    };

    inline Config& g_Config = Config::Get();
}
