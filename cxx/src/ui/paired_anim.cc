#include <SKSEMenuFramework.h>

#include "actor_scanner.hh"
#include "anim_sender.hh"
#include "persistence.hh"
#include "state.hh"

namespace paired_anim::ui {
    namespace {

        inline constexpr int kRescanInterval = 60;

        static void RebuildActorList() {
            auto& s = g_state;
            s.nearbyActors = ScanNearbyActors(s.scanRadius);

            if (auto* player = RE::PlayerCharacter::GetSingleton()) {
                NearbyActor entry;
                entry.actor = player;
                entry.distanceSq = 0.f;
                entry.label = std::format("[Player] {:#010x}", player->GetFormID());
                s.nearbyActors.insert(s.nearbyActors.begin(), std::move(entry));
            }

            const int sz = static_cast<int>(s.nearbyActors.size());
            if (s.attackerIdx >= sz)
                s.attackerIdx = -1;
            if (s.victimIdx >= sz)
                s.victimIdx = -1;
        }

        static RE::Actor* ResolveSlot(int idx) {
            const auto& actors = g_state.nearbyActors;
            if (idx < 0 || idx >= static_cast<int>(actors.size()))
                return nullptr;
            return actors[idx].actor;
        }

        static void DrawActorList(const char* label, int& selectedIdx) {
            ImGuiMCP::TextUnformatted(label);
            const float listH = ImGuiMCP::GetTextLineHeightWithSpacing() * 8.f;
            if (ImGuiMCP::BeginListBox(std::format("##{}", label).c_str(),
                    ImGuiMCP::ImVec2{ 0.f, listH })) {
                const auto& actors = g_state.nearbyActors;
                for (int i = 0; i < static_cast<int>(actors.size()); ++i) {
                    const bool sel = (selectedIdx == i);
                    if (ImGuiMCP::Selectable(actors[i].label.c_str(), sel))
                        selectedIdx = i;
                    if (sel)
                        ImGuiMCP::SetItemDefaultFocus();
                }
                ImGuiMCP::EndListBox();
            }
        }

        void __stdcall Render() {
            auto& s = g_state;

            // Throttled rescan.
            if (++s.frameCounter >= kRescanInterval) {
                RebuildActorList();
                s.frameCounter = 0;
            }

            // ---- Scan radius ---------------------------------------------------
            ImGuiMCP::SetNextItemWidth(220.f);
            if (ImGuiMCP::SliderFloat("Scan radius (units)", &s.scanRadius,
                    500.f, 10000.f, "%.0f")) {
                s.frameCounter = kRescanInterval;
                paired_anim::persistence::Save();
            }
            ImGuiMCP::SameLine();
            ImGuiMCP::TextDisabled("(%.0f m)", s.scanRadius * 0.01428f);

            // ---- Actor selection -----------------------------------------------
            ImGuiMCP::SeparatorText("Actors  (refreshed every ~1 s)");

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

            // ---- Animation selection -------------------------------------------
            ImGuiMCP::SeparatorText("Animation");
            if (ImGuiMCP::Checkbox("Custom events", &s.useCustom))
                persistence::Save();

            if (s.useCustom) {
                // InputText needs a mutable char buffer — use local copies backed by std::string.
                std::array<char, 64> aBuf{}, vBuf{};
                strncpy_s(aBuf.data(), sizeof(aBuf), s.customAttacker.c_str(), sizeof(aBuf) - 1);
                strncpy_s(vBuf.data(), sizeof(vBuf), s.customVictim.c_str(), sizeof(vBuf) - 1);

                ImGuiMCP::SetNextItemWidth(300.f);

                if (s.autoInputAttacker) {
                    ImGuiMCP::BeginDisabled();

                    // Auto-fill attack event by replacing common attacker prefixes.
                    // E.g. "KillMoveSneakBackA" -> "pa_KillMoveSneakBackA"
                    if (s.customVictim.starts_with("pa_")) {
                        s.customAttacker = s.customAttacker.substr(3);
                    } else {
                        s.customAttacker = "pa_" + s.customVictim;
                    }
                    aBuf.fill('\0');
                    strncpy_s(aBuf.data(), sizeof(aBuf), s.customAttacker.c_str(), sizeof(aBuf) - 1);
                }
                if (ImGuiMCP::InputText("Attacker event", aBuf.data(), sizeof(aBuf))) {
                    s.customAttacker = aBuf.data();
                    persistence::Save();
                }

                if (s.autoInputAttacker) {
                    ImGuiMCP::EndDisabled();
                }
                ImGuiMCP::SameLine();
                ImGuiMCP::Checkbox("Auto##A", &s.autoInputAttacker);
                if (ImGuiMCP::IsItemHovered()) {
                    ImGuiMCP::SetTooltip("Auto-fill attack event by replacing common attacker prefixes. e.g., `KillMoveSneakBackA` -> `pa_KillMoveSneakBackA` ");
                }

                ImGuiMCP::SetNextItemWidth(300.f);
                if (ImGuiMCP::InputText("Victim event", vBuf.data(), sizeof(vBuf))) {
                    s.customVictim = vBuf.data();
                    persistence::Save();
                }
            } else {
                static constexpr auto kLabels = []() {
                    std::array<const char*, kPresets.size()> arr{};
                    for (std::size_t i = 0; i < kPresets.size(); ++i)
                        arr[i] = kPresets[i].label;
                    return arr;
                }();

                ImGuiMCP::SetNextItemWidth(300.f);
                if (ImGuiMCP::Combo("Preset", &s.presetIdx,
                        kLabels.data(), static_cast<int>(kLabels.size()))) {
                    persistence::Save();
                }
            }

            // ---- Status + Fire -------------------------------------------------
            ImGuiMCP::Separator();

            // Resolve from list selection (runtime only, not saved here).
            s.attacker = ResolveSlot(s.attackerIdx);
            s.victim = ResolveSlot(s.victimIdx);

            ImGuiMCP::TextDisabled("Attacker : %s",
                s.attacker ? s.attacker->GetDisplayFullName() : "—");
            ImGuiMCP::TextDisabled("Victim   : %s",
                s.victim ? s.victim->GetDisplayFullName() : "—");

            // ---- Positioning -----------------------------------------------------------
            ImGuiMCP::SeparatorText("Positioning");

            if (ImGuiMCP::Checkbox("Reposition victim before fire", &s.repositionBeforeFire)) {
                persistence::Save();
            }

            if (s.repositionBeforeFire) {
                ImGuiMCP::SetNextItemWidth(300.f);
                if (ImGuiMCP::Combo("Victim placement",
                        reinterpret_cast<int*>(&s.placement),
                        kPlacementLabels.data(),
                        static_cast<int>(kPlacementLabels.size()))) {
                    persistence::Save();
                }

                // Visual hint — show the relative geometry as ASCII
                ImGuiMCP::SameLine();
                const char* diagram = nullptr;

                switch (s.placement) {
                    using enum VictimPlacement;
                case FacingPlayer:
                    diagram = "[A]→ ←[V]";
                    break;
                case BehindPlayer:
                    diagram = "[V]→ [A]→";
                    break;
                case SidewaysLeft:
                    diagram = "[V]↓ [A]→";
                    break;
                case SidewaysRight:
                    diagram = "[A]→ [V]↑";
                    break;
                }

                ImGuiMCP::TextDisabled("%s", diagram);
            }

            // ---- Bottom exec button -----------------------------------------------------------
            const bool canFire = s.attacker && s.victim && s.attacker != s.victim;
            if (!canFire)
                ImGuiMCP::BeginDisabled();

            if (ImGuiMCP::Button("Fire Paired Animation", ImGuiMCP::ImVec2{ -1.f, 0.f })) {
                const auto& [lbl, ae, ve] = s.useCustom ?
                                                AnimPair{
                                                    .label = "custom",
                                                    .attackerEvent = s.customAttacker.c_str(),
                                                    .victimEvent = s.customVictim.c_str(),
                                                } :
                                                kPresets[s.presetIdx];
                paired_anim::Send(s.attacker, s.victim, ae, ve, s.placement);
            }

            if (!canFire) {
                ImGuiMCP::EndDisabled();
                if (ImGuiMCP::IsItemHovered(ImGuiMCP::ImGuiHoveredFlags_AllowWhenDisabled)) {
                    ImGuiMCP::SetTooltip("Select different Attacker and Victim");
                }
            }

            // ---- Status display --------------------------------------------------------
            const int si = static_cast<int>(s.lastFireStatus);
            ImGuiMCP::TextColored(kFireStatusColors[si], "Status: %s", kFireStatusLabels[si]);

            // Breakdown is useful when PartialFail
            if (s.lastFireStatus == FireStatus::PartialFail ||
                s.lastFireStatus == FireStatus::Fail) {
                ImGuiMCP::SameLine();
                ImGuiMCP::TextDisabled("(attacker:%s  victim:%s)", s.attackerEventOk ? "OK" : "FAIL", s.victimEventOk ? "OK" : "FAIL");
            }
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
