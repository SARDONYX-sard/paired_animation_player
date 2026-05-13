#include "anim_sender.hh"
#include "actor_scanner.hh"

namespace paired_anim {
    bool PlayPairedIdle(RE::Actor* attacker, RE::Actor* victim, RE::TESIdleForm* idle) {
        if (!attacker || !victim || !idle) {
            return false;
        }

        if (!IsValidPairedTarget(victim)) {
            return false;
        }

        auto* process = attacker->GetActorRuntimeData().currentProcess;
        if (!process) {
            return false;
        }
        const bool ok = process->PlayIdle(attacker, idle, victim);
        SPDLOG_INFO("PlayPairedIdle attacker={:08X} victim={:08X} idle={:08X} result={}", attacker->GetFormID(),
            victim->GetFormID(), idle->GetFormID(), ok);
        return ok;
    }
}  // namespace paired_anim
