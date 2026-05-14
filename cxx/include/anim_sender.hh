#pragma once

namespace paired_anim {
    // Dispatches NotifyAnimationGraph on both actors in the same call stack.
    bool PlayPairedIdle(RE::Actor* attacker, RE::Actor* victim, RE::TESIdleForm* idle);
}
