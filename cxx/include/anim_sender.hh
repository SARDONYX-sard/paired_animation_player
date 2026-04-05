#pragma once
#include "actor_scanner.hh"
#include <array>
#include <string_view>

namespace paired_anim {
    struct AnimPair {
        const char* label;
        const char* attackerEvent;
        const char* victimEvent;
    };

    // Preset table — extend freely.
    // Event names sourced from vanilla behavior graphs.
    inline constexpr std::array<AnimPair, 4> kPresets{
        AnimPair{ .label = "killMove", .attackerEvent = "pa_KillMove", .victimEvent = "KillMove" },
        AnimPair{ .label = "KillMoveSneakBackA", .attackerEvent = "pa_KillMoveSneakBackA", .victimEvent = "KillMoveSneakBackA" },
        AnimPair{ .label = "pa_KillMove2HMStabA", .attackerEvent = "pa_KillMove2HMStabA", .victimEvent = "KillMove2HMStabA" },
        AnimPair{ .label = "HugA", .attackerEvent = "pa_HugA", .victimEvent = "HugA" },
    };

    // Dispatches NotifyAnimationGraph on both actors in the same call stack.
    void Send(RE::Actor* attacker, RE::Actor* victim, std::string_view attackerEvent, std::string_view victimEvent, VictimPlacement placement);

    void RegisterConsoleCommand();
}
