#include <SKSEMenuFramework.h>
#include <optional>

#include "actor_scanner.hh"
#include "anim_sender.hh"
#include "persistence.hh"
#include "state.hh"

#include <SKSEMenuFramework.h>
#include <optional>

namespace paired_anim::ui {
    namespace {
        static RE::Actor* ResolveActor(const std::optional<RE::ActorHandle>& handle) {
            if (!handle) {
                return nullptr;
            }

            return handle->get().get();
        }

        static RE::TESIdleForm* ResolveIdle(std::string_view editorID) {
            auto* form = RE::TESForm::LookupByEditorID(editorID);

            return form ? form->As<RE::TESIdleForm>() : nullptr;
        }

        static std::string GetActorLabel(const std::optional<RE::ActorHandle>& handle) {
            auto* actor = ResolveActor(handle);

            if (!actor) {
                return "—";
            }

            return std::format("{} [{:08X}]", actor->GetDisplayFullName(), actor->GetFormID());
        }

        static void RebuildActorList() {
            auto& s = g_state;

            s.nearbyActors = ScanNearbyActors(s.scanRadius);

            if (auto* player = RE::PlayerCharacter::GetSingleton()) {
                NearbyActor entry;
                entry.actor = player->GetHandle();

                entry.distanceSq = 0.f;

                entry.label = std::format("[Player] {:08X}", player->GetFormID());

                s.nearbyActors.insert(s.nearbyActors.begin(), std::move(entry));
            }

            const int sz = static_cast<int>(s.nearbyActors.size());

            if (s.attackerIdx >= sz) {
                s.attackerIdx = -1;
            }

            if (s.victimIdx >= sz) {
                s.victimIdx = -1;
            }
        }

        static std::optional<RE::ActorHandle> ResolveSlot(int idx) {
            const auto& actors = g_state.nearbyActors;

            if (idx < 0 || idx >= static_cast<int>(actors.size())) {
                return std::nullopt;
            }

            return actors[idx].actor;
        }

        static void DrawActorList(const char* label, int& selectedIdx) {
            ImGuiMCP::TextUnformatted(label);

            const float listH = ImGuiMCP::GetTextLineHeightWithSpacing() * 8.f;

            if (ImGuiMCP::BeginListBox(std::format("##{}", label).c_str(), ImGuiMCP::ImVec2{ 0.f, listH })) {
                const auto& actors = g_state.nearbyActors;

                for (int i = 0; i < static_cast<int>(actors.size()); ++i) {
                    const bool sel = (selectedIdx == i);

                    if (ImGuiMCP::Selectable(actors[i].label.c_str(), sel)) {
                        selectedIdx = i;
                    }

                    if (sel) {
                        ImGuiMCP::SetItemDefaultFocus();
                    }
                }

                ImGuiMCP::EndListBox();
            }
        }

        [[nodiscard]]
        inline const char* GetFireStatusLabel(FireStatus status) {
            switch (status) {
            case FireStatus::None:
                return "—";

            case FireStatus::Ok:
                return "OK";

            case FireStatus::PartialFail:
                return "Partial FAIL";

            case FireStatus::Fail:
                return "FAIL";

            default:
                return "Unknown";
            }
        }

        [[nodiscard]]
        inline ImGuiMCP::ImVec4 GetFireStatusColor(FireStatus status) {
            switch (status) {
            case FireStatus::None:
                return { 0.5f, 0.5f, 0.5f, 1.f };  // grey

            case FireStatus::Ok:
                return { 0.2f, 1.f, 0.2f, 1.f };  // green

            case FireStatus::PartialFail:
                return { 1.f, 0.8f, 0.f, 1.f };  // yellow

            case FireStatus::Fail:
                return { 1.f, 0.2f, 0.2f, 1.f };  // red

            default:
                return { 1.f, 1.f, 1.f, 1.f };  // white
            }
        }

        void RenderIdleList(State& s) {
            ImGuiMCP::SeparatorText("Paired Idle");

            ImGuiMCP::SetNextItemWidth(300.f);

            ImGuiMCP::InputText("Search", s.idleSearch, sizeof(s.idleSearch));

            const float listH = ImGuiMCP::GetTextLineHeightWithSpacing() * 10.f;

            if (ImGuiMCP::BeginListBox("##IdleList", { 0.f, listH })) {
                for (int i = 0; i < static_cast<int>(s.idleCandidates.size()); ++i) {
                    const auto& name = s.idleCandidates[i];

                    if (strlen(s.idleSearch) > 0) {
                        if (!std::string_view(name).contains(s.idleSearch)) {
                            continue;
                        }
                    }

                    const bool selected = s.selectedIdleIdx == i;

                    if (ImGuiMCP::Selectable(name.c_str(), selected)) {
                        s.selectedIdleIdx = i;
                        s.selectedIdle = name;

                        persistence::Save();
                    }

                    if (selected) {
                        ImGuiMCP::SetItemDefaultFocus();
                    }
                }

                ImGuiMCP::EndListBox();
            }
        }

        void __stdcall Render() {
            auto& s = g_state;

            // ------------------------------------------------------------
            // Scan radius
            // ------------------------------------------------------------
            ImGuiMCP::SetNextItemWidth(220.f);

            if (ImGuiMCP::SliderFloat("Scan radius (units)", &s.scanRadius, 500.f, 10000.f, "%.0f")) {
                RebuildActorList();
                persistence::Save();
            }
            ImGuiMCP::SameLine();

            if (ImGuiMCP::Button("Rescan")) {
                RebuildActorList();
            }

            if (s.idleCandidates.empty()) {
                s.idleCandidates = BuildIdleList();
            }

            ImGuiMCP::SameLine();

            ImGuiMCP::TextDisabled("(%.0f m)", s.scanRadius * 0.01428f);

            // ------------------------------------------------------------
            // Actor selection
            // ------------------------------------------------------------

            ImGuiMCP::SeparatorText("Actors");

            ImGuiMCP::ImVec2 outPos{};

            ImGuiMCP::GetContentRegionAvail(&outPos);

            const float halfW = (outPos.x - 8.f) * 0.5f;

            ImGuiMCP::BeginGroup();

            ImGuiMCP::SetNextItemWidth(halfW);

            DrawActorList("Attacker", s.attackerIdx);

            if (ImGuiMCP::Button("Player##A")) {
                s.attackerIdx = 0;
            }

            ImGuiMCP::EndGroup();

            ImGuiMCP::SameLine(0.f, 8.f);

            ImGuiMCP::BeginGroup();

            ImGuiMCP::SetNextItemWidth(halfW);

            DrawActorList("Victim", s.victimIdx);

            if (ImGuiMCP::Button("Player##V")) {
                s.victimIdx = 0;
            }

            ImGuiMCP::EndGroup();

            // ------------------------------------------------------------
            // Animation
            // ------------------------------------------------------------
            RenderIdleList(s);

            // ------------------------------------------------------------
            // Runtime actor resolve
            // ------------------------------------------------------------

            s.attacker = ResolveSlot(s.attackerIdx);
            s.victim = ResolveSlot(s.victimIdx);

            const auto attackerLabel = GetActorLabel(s.attacker);
            const auto victimLabel = GetActorLabel(s.victim);

            ImGuiMCP::Separator();

            ImGuiMCP::TextDisabled("Attacker : %s", attackerLabel.c_str());
            ImGuiMCP::TextDisabled("Victim   : %s", victimLabel.c_str());

            // ------------------------------------------------------------
            // Fire
            // ------------------------------------------------------------

            auto* attackerPtr = ResolveActor(s.attacker);
            auto* victimPtr = ResolveActor(s.victim);

            const bool canFire = attackerPtr && victimPtr && attackerPtr != victimPtr;
            if (!canFire) {
                ImGuiMCP::BeginDisabled();
            }

            if (ImGuiMCP::Button("Fire Paired Animation", ImGuiMCP::ImVec2{ -1.f, 0.f })) {
                auto* idle = ResolveIdle(s.selectedIdle);

                if (!idle) {
                    SPDLOG_ERROR("Idle '{}' not found", s.selectedIdle);
                } else {
                    auto ok = paired_anim::PlayPairedIdle(attackerPtr, victimPtr, idle);
                    g_state.lastFireStatus = (ok) ? FireStatus::Ok : FireStatus::Fail;
                }
            }

            if (!canFire) {
                ImGuiMCP::EndDisabled();

                if (ImGuiMCP::IsItemHovered(ImGuiMCP::ImGuiHoveredFlags_AllowWhenDisabled)) {
                    ImGuiMCP::SetTooltip("Select different actors");
                }
            }

            // Status
            ImGuiMCP::TextColored(GetFireStatusColor(s.lastFireStatus), "Status: %s",
                GetFireStatusLabel(s.lastFireStatus));
        }

        bool __stdcall OnInput(RE::InputEvent*) { return false; }
    }

    void Register() {
        if (!SKSEMenuFramework::IsInstalled()) {
            SPDLOG_WARN("PairedAnim: SKSEMenuFramework not installed");
            return;
        }

        SKSEMenuFramework::SetSection("PairedAnimHelper");
        SKSEMenuFramework::AddSectionItem("Paired Anim", Render);
        SKSEMenuFramework::AddInputEvent(OnInput);
        SPDLOG_INFO("PairedAnim: UI registered");
    }
}
