#include "persistence.hh"

#include <SKSE/SKSE.h>

#include <filesystem>
#include <fstream>
#include <toml.hpp>

#include "State.hh"

namespace paired_anim::persistence {

    namespace {

        static std::filesystem::path DataFile() {
            return std::filesystem::path("Data/SKSE/Plugins") / SKSE::GetPluginName() / "config.toml";
        }

    }

    void Save() {
        auto& s = g_state;

        s.SyncFormIDs();

        toml::value root;

        // Preserve unrelated TOML sections
        if (std::filesystem::exists(DataFile())) {
            try {
                root = toml::parse(DataFile().string());
            } catch (...) {
                root = toml::table{};
            }
        }

        toml::table tbl;

        tbl["scan_radius"] = static_cast<double>(s.scanRadius);

        tbl["attacker_form_id"] = static_cast<std::int64_t>(s.attackerFormID);

        tbl["victim_form_id"] = static_cast<std::int64_t>(s.victimFormID);

        tbl["selected_idle"] = s.selectedIdle;

        root["paired_anim"] = tbl;

        std::filesystem::create_directories(DataFile().parent_path());

        std::ofstream ofs{ DataFile() };

        if (!ofs) {
            SPDLOG_ERROR(
                "PairedAnim: "
                "failed to open config "
                "for write");
            return;
        }

        ofs << toml::format(toml::value{ root });

        SPDLOG_INFO("PairedAnim: config saved");
    }

    void Load() {
        if (!std::filesystem::exists(DataFile())) {
            return;
        }

        try {
            toml::value root = toml::parse(DataFile().string());

            if (!root.contains("paired_anim")) {
                return;
            }

            auto& t = root["paired_anim"];

            auto& s = g_state;

            if (t.contains("scan_radius")) {
                s.scanRadius = static_cast<float>(t["scan_radius"].as_floating());
            }

            if (t.contains("attacker_form_id")) {
                s.attackerFormID = static_cast<RE::FormID>(t["attacker_form_id"].as_integer());
            }

            if (t.contains("victim_form_id")) {
                s.victimFormID = static_cast<RE::FormID>(t["victim_form_id"].as_integer());
            }

            if (t.contains("selected_idle")) {
                s.selectedIdle = t["selected_idle"].as_string();
            }

            SPDLOG_INFO("PairedAnim: config loaded");
        } catch (const toml::syntax_error& e) {
            SPDLOG_ERROR(
                "PairedAnim: "
                "TOML parse error: {}",
                e.what());
        }
    }

}
