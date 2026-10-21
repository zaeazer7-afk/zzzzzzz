#include "game.h"
#include "memory.h"

namespace {
    uintptr_t g_worldPtr = 0;
    std::unordered_map<u64, u64> g_crossmap;
    std::unordered_map<u64, NativeHandler> g_handlerCache;
    NativeRegistration** g_registrationTable = nullptr;

    void buildCrossmap() {
        auto knownHash = 0x3FEF770D40960D5AULL; // GET_ENTITY_COORDS

        auto textStart = mem::gameBase();
        auto pe = (PIMAGE_NT_HEADERS)(textStart + ((PIMAGE_DOS_HEADER)textStart)->e_lfanew);
        auto sections = IMAGE_FIRST_SECTION(pe);
        uintptr_t rdataStart = 0, rdataEnd = 0;
        for (int i = 0; i < pe->FileHeader.NumberOfSections; i++) {
            if (strncmp((char*)sections[i].Name, ".rdata", 6) == 0) {
                rdataStart = textStart + sections[i].VirtualAddress;
                rdataEnd   = rdataStart + sections[i].Misc.VirtualSize;
                break;
            }
        }
        if (!rdataStart) return;

        uintptr_t found = 0;
        for (auto addr = rdataStart; addr + 8 <= rdataEnd; addr += 8) {
            if (mem::read<u64>(addr) == knownHash) { found = addr; break; }
        }
        if (!found) return;

        auto start = found;
        while (start >= rdataStart + 16) {
            auto prev = mem::read<u64>(start - 16);
            if (prev == 0) break;
            start -= 16;
        }

        for (auto addr = start; addr + 16 <= rdataEnd; addr += 16) {
            auto oldH = mem::read<u64>(addr);
            auto newH = mem::read<u64>(addr + 8);
            if (oldH == 0) break;
            g_crossmap[oldH] = newH;
        }
    }

    void findRegistrationTable() {
        auto addr = mem::scanIDA(sigs::NATIVE_REG);
        if (addr) {
            g_registrationTable = (NativeRegistration**)mem::rip(addr + 3, 4, 7 + 3);
        }
        if (!g_registrationTable) {
            addr = mem::scanIDA("4C 8D 05 ? ? ? ? 0F B7 FA 48 8B D9");
            if (addr) g_registrationTable = (NativeRegistration**)mem::rip(addr, 3, 7);
        }
    }

    NativeHandler lookupHandler(u64 hash) {
        if (!g_registrationTable) return nullptr;
        auto reg = g_registrationTable[hash & 0xFF];
        while (reg) {
            for (u32 i = 0; i < reg->count; i++) {
                if (reg->hashes[i] == hash) return reg->handlers[i];
            }
            reg = reg->next;
        }
        return nullptr;
    }
}

bool game::init() {
    auto worldSig = mem::scanIDA(sigs::WORLD);
    if (worldSig) g_worldPtr = mem::rip(worldSig);
    if (!g_worldPtr) return false;

    buildCrossmap();
    findRegistrationTable();
    return g_registrationTable != nullptr;
}

void game::shutdown() {
    g_handlerCache.clear();
    g_crossmap.clear();
}

NativeHandler game::getHandler(u64 hash) {
    auto it = g_handlerCache.find(hash);
    if (it != g_handlerCache.end()) return it->second;

    auto translated = hash;
    auto cm = g_crossmap.find(hash);
    if (cm != g_crossmap.end()) translated = cm->second;

    auto handler = lookupHandler(translated);
    if (!handler && translated != hash)
        handler = lookupHandler(hash);

    if (handler) g_handlerCache[hash] = handler;
    return handler;
}

uintptr_t game::localPed() {
    auto world = mem::read<uintptr_t>(g_worldPtr);
    if (!world) return 0;
    return mem::read<uintptr_t>(world + 8);
}

uintptr_t game::localPlayerInfo() {
    auto ped = localPed();
    if (!ped) return 0;
    return mem::read<uintptr_t>(ped + off::Ped_PlayerInfo);
}

uintptr_t game::localVehicle() {
    auto ped = localPed();
    if (!ped) return 0;
    return mem::read<uintptr_t>(ped + off::Ped_Vehicle);
}

Vec3 game::localPos() {
    auto ped = localPed();
    if (!ped) return {};
    auto nav = mem::read<uintptr_t>(ped + off::Ped_Navigation);
    if (!nav) return {};
    return {
        mem::read<f32>(nav + off::Nav_PosX),
        mem::read<f32>(nav + off::Nav_PosY),
        mem::read<f32>(nav + off::Nav_PosZ)
    };
}
