#include "actor_scanner.hh"

namespace paired_anim {

    bool IsValidPairedTarget(RE::Actor* actor) {
        if (!actor) {
            return false;
        }

        auto* player = RE::PlayerCharacter::GetSingleton();

        if (actor == player) {
            return false;
        }

        if (actor->IsDead()) {
            return false;
        }

        if (actor->IsDisabled()) {
            return false;
        }

        if (actor->IsDeleted()) {
            return false;
        }

        if (!actor->Is3DLoaded()) {
            return false;
        }

        if (actor->IsInKillMove()) {
            return false;
        }

        if (actor->IsOnMount()) {
            return false;
        }

        if (actor->IsChild()) {
            return false;
        }

        const auto* actor_state = actor->AsActorState();
        if (actor_state && actor_state->GetSitSleepState() != RE::SIT_SLEEP_STATE::kNormal) {
            return false;
        }

        if (!actor->GetActorRuntimeData().currentProcess) {
            return false;
        }

        return true;
    }

    std::vector<NearbyActor> ScanNearbyActors(float radius) {
        std::vector<NearbyActor> result;

        auto* player = RE::PlayerCharacter::GetSingleton();
        auto* process = RE::ProcessLists::GetSingleton();

        if (!player || !process) {
            return result;
        }

        const auto  playerPos = player->GetPosition();
        const float radiusSq = radius * radius;

        auto collect = [&](RE::BSTArray<RE::BSPointerHandle<RE::Actor>>& arr) {
            for (auto& handle : arr) {
                auto  ptr = handle.get();
                auto* actor = ptr ? ptr->As<RE::Actor>() : nullptr;

                if (!IsValidPairedTarget(actor)) {
                    continue;
                }

                const float dsq = actor->GetPosition().GetSquaredDistance(playerPos);

                if (dsq > radiusSq) {
                    continue;
                }

                NearbyActor e;
                e.actor = actor->GetHandle();
                e.distanceSq = dsq;

                e.label = std::format("{} [{:08X}]", actor->GetDisplayFullName(), actor->GetFormID());

                result.push_back(std::move(e));
            }
        };

        collect(process->highActorHandles);
        collect(process->middleHighActorHandles);
        collect(process->middleLowActorHandles);

        std::ranges::sort(result, {}, &NearbyActor::distanceSq);

        const auto uniqueRange =
            std::ranges::unique(result, [](const NearbyActor& a, const NearbyActor& b) { return a.actor == b.actor; });
        result.erase(uniqueRange.begin(), uniqueRange.end());

        return result;
    }

    std::vector<std::string> BuildIdleList() {
        std::vector<std::string> result;

        const auto [map, lock] = RE::TESForm::GetAllForms();
        const auto _guard(lock.get());

        for (const auto [id, form] : *map) {
            auto* idle = form ? form->As<RE::TESIdleForm>() : nullptr;

            if (!idle) {
                continue;
            }

            const auto* editorID = idle->GetFormEditorID();

            if (!editorID) {
                continue;
            }

            std::string_view eid = editorID;

            if (!eid.starts_with("pa_")) {
                continue;
            }

            result.emplace_back(eid);
        }

        std::ranges::sort(result);

        auto uniqueRange = std::ranges::unique(result);

        result.erase(uniqueRange.begin(), uniqueRange.end());

        return result;
    }
}
