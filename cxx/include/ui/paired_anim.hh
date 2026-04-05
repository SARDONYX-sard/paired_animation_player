#pragma once

namespace paired_anim::ui {
    // Called once at kPostPostLoad.
    void Register();

    // Render callback — signature required by SKSEMenuFramework.
    void __stdcall Render();

    // Input callback — signature required by SKSEMenuFramework.
    // Returns true to consume (block) the input event.
    bool __stdcall OnInput(RE::InputEvent* event);
}
