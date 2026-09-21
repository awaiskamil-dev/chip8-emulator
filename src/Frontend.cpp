#include "Frontend.h"
#include "Controls.h"
#include <algorithm>
#include <iostream>

bool loadCurrentRom(Chip8& chip8, FrontendState& state)
{
    state.controls.clear();
    state.resetRequested = false;
    state.stepRequests = 0;
    state.clearTiming = true;
    state.cycleAccumulator = 0;
    state.timerAccumulator = 0;

    if (state.romPath.empty()) {
        state.message = "NO ROM LOADED. START WITH A ROM FILE PATH.";
        return false;
    }

    // load() already calls init(). Do not reset/reimplement core state here.
    state.romLoaded = chip8.load(state.romPath.c_str());
    if (!state.romLoaded) {
        state.message = "ROM LOAD FAILED. CHECK THE TERMINAL; F5 RETRIES.";
        std::cerr << state.message << '\n';
        return false;
    }

    loadGameControls(state);
    state.message.clear();
    return true;
}

void updateEmulation(Chip8& chip8, FrontendState& state, double elapsedSeconds)
{
    if (state.resetRequested) {
        loadCurrentRom(chip8, state);
    }

    // No accumulated burst of instructions after pause, focus loss or reset.
    if (state.clearTiming) {
        state.cycleAccumulator = 0;
        state.timerAccumulator = 0;
        elapsedSeconds = 0;
        state.clearTiming = false;
    }

    if (!state.romLoaded || !chip8.running || !state.focused) {
        state.cycleAccumulator = 0;
        state.timerAccumulator = 0;
        state.stepRequests = 0;
        return;
    }

    if (state.paused) {
        // One F2 key press means one call, including an FX0A waiting for a key.
        while (state.stepRequests > 0 && chip8.running) {
            chip8.emulate_cycle();
            --state.stepRequests;
        }
        state.stepRequests = 0;
        state.cycleAccumulator = 0;
        state.timerAccumulator = 0;
    } else {
        state.stepRequests = 0;
        // Cap catch-up work after a window drag / debugger stall to 100 ms.
        elapsedSeconds = std::clamp(elapsedSeconds, 0.0, 0.1);
        state.cycleAccumulator += elapsedSeconds * state.cpuHz;
        state.timerAccumulator += elapsedSeconds * 60.0;

        // These ticks belong to elapsed time before this frame's instructions.
        // A newly written ST=1 must survive until the next tick to make a beep.
        while (state.timerAccumulator >= 1.0 && chip8.running) {
            chip8.update_timers();
            state.timerAccumulator -= 1.0;
        }
        while (state.cycleAccumulator >= 1.0 && chip8.running) {
            chip8.emulate_cycle();
            state.cycleAccumulator -= 1.0;
        }
    }

    if (!chip8.running) {
        state.message = "CORE STOPPED. CHECK THE TERMINAL; F5 RELOADS THE ROM.";
        state.stepRequests = 0;
        state.cycleAccumulator = 0;
        state.timerAccumulator = 0;
    }
}
