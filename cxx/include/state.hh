#pragma once
#include <SKSEMenuFramework.h>
#include <string>

#include "RE/B/BSCoreTypes.h"
#include "actor_scanner.hh"

namespace paired_anim {
    enum class FireStatus : int {
        None = 0,         // not fired yet
        Ok = 1,           // both events succeeded
        PartialFail = 2,  // one of the two failed
        Fail = 3,         // both failed
    };

    inline constexpr std::array<const char*, 4> kFireStatusLabels{
        "—",
        "OK",
        "Partial FAIL",
        "FAIL",
    };

    // Status colors for ImGui (RGBA)
    inline constexpr std::array<ImGuiMCP::ImVec4, 4> kFireStatusColors{
        ImGuiMCP::ImVec4{ 0.5f, 0.5f, 0.5f, 1.f },  // None    — grey
        ImGuiMCP::ImVec4{ 0.2f, 1.0f, 0.2f, 1.f },  // Ok      — green
        ImGuiMCP::ImVec4{ 1.0f, 0.8f, 0.0f, 1.f },  // Partial — yellow
        ImGuiMCP::ImVec4{ 1.0f, 0.2f, 0.2f, 1.f },  // Fail    — red
    };

    // All persistent + runtime state lives here.
    // One global instance: PairedAnim::g_state
    struct State {
        // ---- Persistent (saved to TOML) -------------------------------------
        float scanRadius{ 2000.f };

        RE::FormID attackerFormID{ 0 };  // FormIDs survive save/load; Actor* do not.
        RE::FormID victimFormID{ 0 };

        std::string selectedIdle{ "pa_HugB" };  // CK Idle EditorID

        // ---- Runtime (not saved) --------------------------------------------
        // Resolved from FormIDs on load, and updated by UI selection.
        std::optional<RE::ActorHandle> attacker{ std::nullopt };
        std::optional<RE::ActorHandle> victim{ std::nullopt };

        // Nearby actor list rebuilt every kRescanInterval frames.
        std::vector<struct NearbyActor> nearbyActors{};
        int                             attackerIdx{ -1 };  // index into nearbyActors
        int                             victimIdx{ -1 };
        int                             frameCounter{ 0 };

        FireStatus lastFireStatus{ FireStatus::None };

        std::vector<std::string> idleCandidates{};
        int                      selectedIdleIdx{ -1 };
        char                     idleSearch[64]{};

        // ---- Helpers --------------------------------------------------------

        // Sync Actor* from current FormIDs (call after load).
        void ResolveActors() {
            auto lookup = [](RE::FormID id) -> RE::Actor* {
                if (!id) {
                    return nullptr;
                }
                auto* form = RE::TESForm::LookupByID(id);
                return form ? form->As<RE::Actor>() : nullptr;
            };

            attacker = lookup(attackerFormID);
            victim = lookup(victimFormID);
        }

        // Sync FormIDs from Actor* (call before save and after UI selection).
        void SyncFormIDs() {
            attackerFormID = (attacker && attacker->get()) ? attacker->get()->GetFormID() : 0;
            victimFormID = (victim && victim->get()) ? victim->get()->GetFormID() : 0;
        }
    };

    inline State g_state;  // Single global instance.
}
