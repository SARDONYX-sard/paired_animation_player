#pragma once

namespace paired_anim {
    enum class VictimPlacement : int {
        FacingPlayer = 0,
        BehindPlayer = 1,
        SidewaysLeft = 2,
        SidewaysRight = 3,

    };

    inline constexpr std::array<const char*, 4> kPlacementLabels{
        "Facing player (front killmove)",
        "Behind player (back killmove)",
        "Left of player",
        "Right of player",
    };

    struct NearbyActor {
        RE::Actor*  actor{ nullptr };
        float       distanceSq{ 0.f };
        std::string label;  // "Name [RefID]"
    };

    // Collects all loaded Actor references within `radiusUnits` of the player.
    // Skyrim units ≈ 1.43 cm, so 3000 units ≈ 30 m.
    inline constexpr float kScanRadiusSq = 3000.f * 3000.f;

    [[nodiscard]] std::vector<NearbyActor> ScanNearbyActors(float scanRadius);
}
