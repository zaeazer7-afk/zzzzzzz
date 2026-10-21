#include "battleye.h"
#include "memory.h"
#include "game.h"

namespace {
    bool patchStatusUpdate() {
        auto addr = mem::scanIDA(sigs::BE_STATUS_UPDATE);
        if (!addr) return false;

        // Add(11).Rip().Add(1).Rip() — resolve to the function, patch first byte to ret
        auto step1 = mem::rip(addr + 11, 0, 0);  // default rip: read i32 at +3, add +7
        if (!step1) return false;
        auto target = mem::rip(step1 + 1, 0, 0);
        if (!target) return false;

        mem::patch<u8>(target, 0xC3);
        return true;
    }

    bool patchBEData() {
        auto addr = mem::scanIDA(sigs::BE_DATA);
        if (!addr) return false;

        // Add(3).Rip().Add(8).Add(4) — resolve to data pointer, zero 16 bytes
        auto ripped = mem::rip(addr, 3, 7 + 3);  // rip at offset 3, instrLen = 10
        if (!ripped) return false;
        auto target = ripped + 8 + 4;

        u8 zeros[16] = {};
        mem::patch(target, zeros, sizeof(zeros));
        return true;
    }

    bool patchProcessJoin() {
        auto addr = mem::scanIDA(sigs::BE_PROCESS_JOIN);
        if (!addr) return false;

        // Sub(4).Rip().As<PVOID*>()[1] — go back 4, rip, then index [1]
        auto base = addr - 4;
        auto ripped = mem::rip(base);
        if (!ripped) return false;
        auto target = mem::read<uintptr_t>(ripped + 8);
        if (!target) return false;

        mem::patch<u8>(target, 0xC3);
        return true;
    }
}

bool battleye::init() {
    int patched = 0;
    if (patchStatusUpdate()) patched++;
    if (patchBEData())       patched++;
    if (patchProcessJoin())  patched++;
    return patched > 0;
}
