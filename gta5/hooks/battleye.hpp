#pragma once

namespace gta5::hooks {

class BattlEye {
public:
    static bool Init();
    static void Shutdown();

private:
    static bool PatchStatusUpdate();
    static bool PatchBEData();

    static void PatchBytes(void* address, const void* patch, size_t size);
    static void RestoreBytes(void* address, const void* original, size_t size);

    struct PatchInfo {
        void*         address;
        unsigned char original[16];
        unsigned char patched[16];
        size_t        size;
        bool          applied;
    };

    static PatchInfo s_StatusUpdatePatch;
    static PatchInfo s_BEDataPatch;
};

} // namespace gta5::hooks
