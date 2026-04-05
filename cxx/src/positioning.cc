#include <SKSE/SKSE.h>
#include <numbers>

#include "RE/N/NiPoint3.h"
#include "actor_scanner.hh"
#include "positioning.hh"

namespace paired_anim {
    namespace {
        // Actors must be nearly touching for paired anim sync nodes to trigger.
        inline constexpr float kContactOffset = 50.f;

        static RE::NiPoint3 ForwardFromAngleZ(float z) {
            return { std::sin(z), std::cos(z), 0.f };
        }

        // Returns the yaw offset (radians) that victim should be rotated by
        // relative to attacker's facing, and the position offset direction.
        struct PlacementConfig {
            float positionAngleOffset;  // offset from attacker forward for victim pos
            float victimAngleOffset;    // victim facing relative to attacker
        };

        static constexpr PlacementConfig ResolvePlacement(VictimPlacement p) {
            using enum VictimPlacement;
            using std::numbers::pi;
            switch (p) {
            case FacingPlayer:
                return { .positionAngleOffset = 0.f, .victimAngleOffset = static_cast<float>(pi) };
            case BehindPlayer:
                return { .positionAngleOffset = static_cast<float>(pi), .victimAngleOffset = 0.f };
            case SidewaysLeft:
                return { .positionAngleOffset = static_cast<float>(pi * 0.5), .victimAngleOffset = static_cast<float>(pi * 1.5) };
            case SidewaysRight:
                return { .positionAngleOffset = static_cast<float>(pi * 1.5), .victimAngleOffset = static_cast<float>(pi * 0.5) };

            default:
                return { .positionAngleOffset = 0.f, .victimAngleOffset = static_cast<float>(pi) };
            }
        }
    }

    void AlignActors(RE::Actor* attacker, RE::Actor* victim, VictimPlacement placement) {
        if (!attacker || !victim) {
            return;
        }
        if (!attacker || !victim)
            return;

        const auto  basePos = attacker->GetPosition();
        const float baseAngleZ = attacker->GetAngleZ();
        const auto  cfg = ResolvePlacement(placement);

        const float posAngle = baseAngleZ + cfg.positionAngleOffset;
        const auto  fwd = ForwardFromAngleZ(posAngle);

        RE::NiPoint3 targetPos{
            basePos.x + fwd.x * kContactOffset,
            basePos.y + fwd.y * kContactOffset,
            basePos.z,  // keep victim's Z on same ground level
        };

        const float targetAngleZ = baseAngleZ + cfg.victimAngleOffset;

        auto* tasks = SKSE::GetTaskInterface();
        if (!tasks) {
            SPDLOG_WARN("PairedAnim: SKSE task interface unavailable");
            return;
        }

        RE::NiPoint3 rot{ 0.f, 0.f, targetAngleZ };
        victim->SetPosition(targetPos, /*forceUpdate=*/true);
        victim->SetAngle(rot);
        SPDLOG_INFO(
            "PairedAnim: victim={:#010x} moved to ({:.1f},{:.1f},{:.1f}) angleZ={:.3f}",
            victim->GetFormID(),
            targetPos.x, targetPos.y, targetPos.z, targetAngleZ);
    }
}
