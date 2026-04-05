#include "anim_sender.hh"
#include "actor_scanner.hh"
#include "positioning.hh"
#include "state.hh"

namespace paired_anim {
    void Send(RE::Actor* attacker, RE::Actor* victim, std::string_view attackerEvent, std::string_view victimEvent, VictimPlacement placement) {
        if (!attacker || !victim) {
            SPDLOG_WARN("PairedAnim::Send — null actor, skipping");
            return;
        }

        if (g_state.repositionBeforeFire) {
            // Task #1: move victim into position (inside AlignActors)
            AlignActors(attacker, victim, placement);

            // Task #2: fire anim events — runs after #1 in the same frame pump
            auto ae = std::string(attackerEvent);
            auto ve = std::string(victimEvent);
            SKSE::GetTaskInterface()->AddTask(
                [attacker, victim, ae = std::move(ae), ve = std::move(ve)]() {
                    const bool aOk = attacker->NotifyAnimationGraph(ae);
                    const bool vOk = victim->NotifyAnimationGraph(ve);

                    g_state.attackerEventOk = aOk;
                    g_state.victimEventOk = vOk;
                    g_state.lastFireStatus =
                        (aOk && vOk) ? FireStatus::Ok :
                        (aOk || vOk) ? FireStatus::PartialFail :
                                       FireStatus::Fail;

                    SPDLOG_INFO(
                        "PairedAnim | attacker={:#010x} '{}' [{}]"
                        " | victim={:#010x} '{}' [{}]",
                        attacker->GetFormID(), ae, aOk ? "OK" : "FAIL",
                        victim->GetFormID(), ve, vOk ? "OK" : "FAIL");
                });
        } else {
            const bool aOk = attacker->NotifyAnimationGraph(attackerEvent);
            const bool vOk = victim->NotifyAnimationGraph(victimEvent);
            g_state.attackerEventOk = aOk;
            g_state.victimEventOk = vOk;
            g_state.lastFireStatus =
                (aOk && vOk) ? FireStatus::Ok :
                (aOk || vOk) ? FireStatus::PartialFail :
                               FireStatus::Fail;
            SPDLOG_INFO(
                "PairedAnim | attacker={:#010x} '{}' [{}]"
                " | victim={:#010x} '{}' [{}]",
                attacker->GetFormID(), attackerEvent, aOk ? "OK" : "FAIL",
                victim->GetFormID(), victimEvent, vOk ? "OK" : "FAIL");
        }
    }

    namespace {

        // Parse up to 3 kChar params from ScriptData using the StringChunk API.
        // Returns the number of params actually present.
        struct ParsedParams {
            std::string victimHex;
            std::string attackerEvent{ "killMoveStart" };
            std::string victimEvent{ "killMoveVictimStart" };
            int         count{ 0 };
        };

        ParsedParams ParseConsoleParams(RE::SCRIPT_FUNCTION::ScriptData* data) {
            ParsedParams out;
            if (!data || data->numParams < 1)
                return out;

            // First param — victim RefID (required)
            auto* chunk = data->GetStringChunk();
            if (!chunk)
                return out;
            out.victimHex = chunk->GetString();
            out.count = 1;

            if (data->numParams < 2)
                return out;

            // Second param — attacker event (optional)
            auto* next = chunk->GetNext();
            if (!next)
                return out;
            chunk = next->AsString();
            if (!chunk)
                return out;
            out.attackerEvent = chunk->GetString();
            out.count = 2;

            if (data->numParams < 3)
                return out;

            // Third param — victim event (optional)
            next = chunk->GetNext();
            if (!next)
                return out;
            chunk = next->AsString();
            if (!chunk)
                return out;
            out.victimEvent = chunk->GetString();
            out.count = 3;

            return out;
        }

        bool Exec(const RE::SCRIPT_PARAMETER*,
            RE::SCRIPT_FUNCTION::ScriptData* data,
            RE::TESObjectREFR*               thisObj,
            RE::TESObjectREFR*,
            RE::Script*, RE::ScriptLocals*,
            double&, std::uint32_t&) {
            auto* con = RE::ConsoleLog::GetSingleton();

            auto* attacker = thisObj ? thisObj->As<RE::Actor>() : nullptr;
            if (!attacker) {
                con->Print("PairedAnim: select an Actor first");
                return true;
            }

            const auto p = ParseConsoleParams(data);
            if (p.count < 1) {
                con->Print("Usage: pa <victimHex> [attackerEvent] [victimEvent]");
                return true;
            }

            const auto id = static_cast<RE::FormID>(
                std::strtoul(p.victimHex.c_str(), nullptr, 16));
            auto* form = RE::TESForm::LookupByID(id);
            if (!form || !form->Is(RE::FormType::ActorCharacter)) {
                con->Print("PairedAnim: victim not found");
                return true;
            }

            Send(attacker, form->As<RE::Actor>(), p.attackerEvent, p.victimEvent, VictimPlacement::FacingPlayer);
            con->Print(std::format("PairedAnim: fired '{}' / '{}'",
                p.attackerEvent, p.victimEvent)
                    .c_str());
            return true;
        }

    }  // namespace

    void RegisterConsoleCommand() {
        auto* cmd = RE::SCRIPT_FUNCTION::LocateConsoleCommand("TestSeenData");
        if (!cmd) {
            SPDLOG_ERROR("PairedAnim: no free console slot");
            return;
        }

        static RE::SCRIPT_PARAMETER params[3]{
            { .paramName = "Victim RefID", .paramType = RE::SCRIPT_PARAM_TYPE::kChar, .optional = 0 },
            { .paramName = "Attacker event", .paramType = RE::SCRIPT_PARAM_TYPE::kChar, .optional = 1 },
            { .paramName = "Victim event", .paramType = RE::SCRIPT_PARAM_TYPE::kChar, .optional = 1 },
        };

        cmd->functionName = "PairedAnim";
        cmd->shortName = "pa";
        cmd->helpString = "pa <victimHex> [attackerEvent] [victimEvent]";
        cmd->referenceFunction = false;
        cmd->executeFunction = Exec;
        cmd->conditionFunction = nullptr;
        cmd->SetParameters(params);

        SPDLOG_INFO("PairedAnim: 'pa' console command registered");
    }

}  // namespace paired_anim
