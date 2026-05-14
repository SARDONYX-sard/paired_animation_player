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

    // All persistent + runtime state lives here. One global instance: PairedAnim::g_state
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
        inline void ResolveActors() {
            auto lookup = [](RE::FormID id) -> RE::Actor* {
                if (!id) {
                    return nullptr;
                }
                auto* form = RE::TESForm::LookupByID(id);
                return form ? form->As<RE::Actor>() : nullptr;
            };

            this->attacker = lookup(attackerFormID);
            this->victim = lookup(victimFormID);
        }

        // Sync FormIDs from Actor* (call before save and after UI selection).
        inline void SyncFormIDs() {
            this->attackerFormID = (this->attacker && this->attacker->get()) ? this->attacker->get()->GetFormID() : 0;
            this->victimFormID = (this->victim && this->victim->get()) ? this->victim->get()->GetFormID() : 0;
        }
    };

    inline State g_state;  // Single global instance.
}
