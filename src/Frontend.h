#ifndef FRONTEND_H
#define FRONTEND_H

#include "Chip8.h"
#include <string>
#include <vector>
#include <SFML/System/Vector2.hpp>

// Only window/control settings live here. CPU state stays in Chip8.
struct FrontendState
{
    bool debugMode = false;
    bool paused = true;
    bool focused = true;
    bool romLoaded = false;
    bool resetRequested = false;
    bool clearTiming = false;
    unsigned int stepRequests = 0;
    int cpuHz = 700;
    sf::Vector2f mousePosition = {-1, -1};
    std::string romPath;
    std::string message;
    std::vector<std::string> controls;

    // Fractions of a CPU cycle / timer tick carried into the next frame.
    double cycleAccumulator = 0;
    double timerAccumulator = 0;
};

// Main calls these helpers; they only call the existing core functions.
bool loadCurrentRom(Chip8& chip8, FrontendState& state);
void updateEmulation(Chip8& chip8, FrontendState& state, double elapsedSeconds);

#endif
