#include "persistence.hh"
#include "State.hh"
#include <SKSE/SKSE.h>
#include <filesystem>
#include <fstream>
#include <toml.hpp>

namespace paired_anim::persistence {
    namespace {
        static std::filesystem::path DataFile() {
            return std::filesystem::path("Data/SKSE/Plugins") / SKSE::GetPluginName() / "config.toml";
        }
    }

    void Save() {
        auto& s = g_state;
        s.SyncFormIDs();

        // Read existing file so we don't clobber other plugins' sections.
        toml::value root;
        if (std::filesystem::exists(DataFile())) {
            try {
                root = toml::parse(DataFile().string());
            } catch (...) {
                root = toml::table{};
            }
        }

        toml::table tbl;
        tbl["scan_radius"] = static_cast<double>(s.scanRadius);
        tbl["preset_idx"] = s.presetIdx;
        tbl["use_custom"] = s.useCustom;
        tbl["custom_attacker"] = s.customAttacker;
        tbl["custom_victim"] = s.customVictim;
        tbl["attacker_form_id"] = static_cast<std::int64_t>(s.attackerFormID);
        tbl["victim_form_id"] = static_cast<std::int64_t>(s.victimFormID);
        tbl["reposition_before_fire"] = s.repositionBeforeFire;
        tbl["placement"] = static_cast<int>(s.placement);
        tbl["auto_input_victim"] = s.autoInputAttacker;

        root["paired_anim"] = tbl;

        std::ofstream ofs{ DataFile() };
        if (ofs)
            ofs << toml::format(toml::value{ root });
    }

    void Load() {
        if (!std::filesystem::exists(DataFile())) {
            return;
        }

        try {
            toml::value root = toml::parse(DataFile().string());

            auto& t = root["paired_anim"];

            auto& s = g_state;

            if (t.contains("scan_radius")) {
                s.scanRadius = static_cast<float>(t["scan_radius"].as_floating());
            }
            if (t.contains("preset_idx")) {
                s.presetIdx = static_cast<int>(t["preset_idx"].as_integer());
            }
            if (t.contains("use_custom")) {
                s.useCustom = t["use_custom"].as_boolean();
            }
            if (t.contains("custom_attacker")) {
                s.customAttacker = t["custom_attacker"].as_string();
            }
            if (t.contains("custom_victim")) {
                s.customVictim = t["custom_victim"].as_string();
            }
            if (t.contains("attacker_form_id")) {
                s.attackerFormID = static_cast<RE::FormID>(t["attacker_form_id"].as_integer());
            }
            if (t.contains("victim_form_id")) {
                s.victimFormID = static_cast<RE::FormID>(
                    t["victim_form_id"].as_integer());
            }
            if (t.contains("reposition_before_fire")) {
                s.repositionBeforeFire = t["reposition_before_fire"].as_boolean();
            }
            if (t.contains("placement")) {
                s.placement = static_cast<VictimPlacement>(
                    t["placement"].as_integer());
            }
            if (t.contains("auto_input_victim")) {
                s.autoInputAttacker = t["auto_input_victim"].as_boolean();
            }

            // Actor* resolution happens in kDataLoaded (forms are ready then).
            SPDLOG_INFO("PairedAnim: config loaded");
        } catch (toml::syntax_error& e) {
            SPDLOG_ERROR("PairedAnim: TOML parse error: {}", e.what());
            return;
        }
    }
}
