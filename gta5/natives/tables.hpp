#pragma once
#include <cstdint>
#include <cstring>
#include "invoker.hpp"

namespace gta5::natives {

// Pattern: "EB 2A 0F 1F 40 00 48 8B 54 17 10" (InitNativeTables from game.h)
// Resolution: Sub(0x2A).As() — the pattern is 0x2A bytes into the function

// The game calls this function for each script program to register its native
// handlers. The registration table is a flat array of (hash, handler) pairs.
// We hook this to intercept every registration and build our own lookup map.

// RAGE native registration table entry
struct NativeRegistrationEntry {
    uint64_t hash;
    void*    handler;
};

// Original function signature:
// void __fastcall InitNativeTables(void* registrationTable, int entryCount)
using InitNativeTables_t = void(*)(void* registrationTable, uint64_t entryCount);

class NativeTable {
public:
    // Call during init after pattern scanning resolves InitNativeTables
    static void Hook(void* initNativeTablesAddr) {
        s_OriginalFn = reinterpret_cast<InitNativeTables_t>(initNativeTablesAddr);
    }

    // Hook callback — called in place of original InitNativeTables
    // The game passes a table pointer and count; each entry is {hash, handler}
    static void OnRegister(void* registrationTable, uint64_t entryCount) {
        auto entries = reinterpret_cast<NativeRegistrationEntry*>(registrationTable);

        for (uint64_t i = 0; i < entryCount; i++) {
            NativeInvoker::RegisterHandler(entries[i].hash, entries[i].handler);
        }

        // Call original so the game's own dispatch table is also populated
        if (s_OriginalFn)
            s_OriginalFn(registrationTable, entryCount);
    }

    // Alternative: walk the game's native registration table directly.
    // Enhanced uses a 256-bucket open-addressing table at a global pointer.
    // Each bucket is a linked list of {nextPtr, handlerCount, hashes[7], handlers[7]}.
    static void CacheFromRegistrationTable(uintptr_t tableBase) {
        if (!tableBase)
            return;

        auto table = reinterpret_cast<uintptr_t*>(tableBase);
        for (int bucket = 0; bucket < 256; bucket++) {
            auto entry = table[bucket];
            while (entry) {
                auto next         = *reinterpret_cast<uintptr_t*>(entry);
                auto handlerCount = *reinterpret_cast<uint32_t*>(entry + 8);

                // Hashes stored at entry+16, handlers at entry+16+8*7=entry+72
                auto hashes   = reinterpret_cast<uint64_t*>(entry + 16);
                auto handlers = reinterpret_cast<void**>(entry + 16 + 8 * 7);

                for (uint32_t i = 0; i < handlerCount && i < 7; i++) {
                    if (hashes[i] && handlers[i])
                        NativeInvoker::RegisterHandler(hashes[i], handlers[i]);
                }

                entry = next;
            }
        }
    }

    static InitNativeTables_t GetOriginal() { return s_OriginalFn; }

private:
    inline static InitNativeTables_t s_OriginalFn = nullptr;
};

} // namespace gta5::natives
