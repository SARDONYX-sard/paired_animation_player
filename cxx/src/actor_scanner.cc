#include "actor_scanner.hh"

namespace paired_anim {
    std::vector<NearbyActor> ScanNearbyActors(float scanRadius) {
        std::vector<NearbyActor> result;

        auto* player = RE::PlayerCharacter::GetSingleton();
        if (!player)
            return result;

        const auto  playerPos = player->GetPosition();
        const float radiusSq = scanRadius * scanRadius;

        // ProcessLists holds every currently loaded Actor.
        auto* process_lists = RE::ProcessLists::GetSingleton();
        if (!process_lists)
            return result;

        // Iterate high / middle / low process lists — all loaded actors live here.
        auto collectFrom = [&](RE::BSTArray<RE::BSPointerHandle<RE::Actor>>& handleArr) {
            for (auto& handle : handleArr) {
                auto ptr = handle.get();
                if (!ptr) {
                    continue;
                }

                auto* actor = ptr->template As<RE::Actor>();
                if (!actor || actor == player) {
                    continue;
                }

                // Distance check before any heavier work.
                const float dsq = actor->GetPosition().GetSquaredDistance(playerPos);
                if (dsq > radiusSq) {
                    continue;
                }

                NearbyActor entry;
                entry.actor = actor;
                entry.distanceSq = dsq;
                entry.label = std::format("{} [{:#010x}]",
                    actor->GetDisplayFullName(),
                    actor->GetFormID());
                result.push_back(std::move(entry));
            }
        };

        collectFrom(process_lists->highActorHandles);
        collectFrom(process_lists->middleHighActorHandles);
        collectFrom(process_lists->middleLowActorHandles);

        // Sort nearest-first for the list UI.
        std::ranges::sort(result, {}, &NearbyActor::distanceSq);
        return result;
    }
}
