#pragma once

namespace paired_anim {
    struct NearbyActor {
        RE::ActorHandle actor{};
        float           distanceSq{ 0.f };
        std::string     label;  // "Name [RefID]"
    };

    // Collects all loaded Actor references within `radiusUnits` of the player.
    // Skyrim units ≈ 1.43 cm, so 3000 units ≈ 30 m.
    inline constexpr float kScanRadiusSq = 3000.f * 3000.f;

    [[nodiscard]] std::vector<NearbyActor> ScanNearbyActors(float scanRadius);

    [[nodiscard]] bool IsValidPairedTarget(RE::Actor* actor);

    [[nodiscard]] std::vector<std::string> BuildIdleList();
}
