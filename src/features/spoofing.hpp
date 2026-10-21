#pragma once
#include <core/scheduler.hpp>
#include <string>

namespace Features
{
    class Spoofing
    {
    public:
        static Spoofing& Get() { static Spoofing inst; return inst; }

        bool        SpoofName      = false;
        std::string FakeName       = "Player";
        bool        SpoofModel     = false;
        std::string FakeModel      = "a_m_m_business_01";
        bool        VoiceProximity = false;
        float       FakeVoiceRange = 100.f;

        void Update()
        {
            if (SpoofModel)
            {
                std::string code = R"(
                    local model = GetHashKey(')" + FakeModel + R"(')
                    if not HasModelLoaded(model) then
                        RequestModel(model)
                    elseif GetEntityModel(PlayerPedId()) ~= model then
                        SetPlayerModel(PlayerId(), model)
                        SetModelAsNoLongerNeeded(model)
                    end
                )";
                Core::g_Scheduler.Execute(code.c_str(), "spoof_model");
            }

            if (VoiceProximity)
            {
                std::string code = "NetworkSetTalkerProximity(" + std::to_string(FakeVoiceRange) + ")";
                Core::g_Scheduler.Execute(code.c_str(), "voice_prox");
            }
        }

        void SetFakeName(const char* name)
        {
            FakeName = name;
            SpoofName = true;
        }

        void SetModel(const char* model)
        {
            std::string code = R"(
                local model = GetHashKey(')" + std::string(model) + R"(')
                RequestModel(model)
                while not HasModelLoaded(model) do Wait(100) end
                SetPlayerModel(PlayerId(), model)
                SetModelAsNoLongerNeeded(model)
            )";
            Core::g_Scheduler.ExecuteSafe(code.c_str(), "set_model");
        }

        void ResetModel()
        {
            Core::g_Scheduler.ExecuteSafe(R"(
                local model = GetHashKey('mp_m_freemode_01')
                RequestModel(model)
                while not HasModelLoaded(model) do Wait(100) end
                SetPlayerModel(PlayerId(), model)
                SetModelAsNoLongerNeeded(model)
            )", "reset_model");
        }

        void SpoofVoiceProximity()
        {
            std::string code = "NetworkSetTalkerProximity(" + std::to_string(FakeVoiceRange) + ")";
            Core::g_Scheduler.ExecuteSafe(code.c_str(), "spoof_voice");
        }

    private:
        Spoofing() = default;
    };

    inline Spoofing& g_Spoofing = Spoofing::Get();
}
