#include "Chip8.h"
#include "Audio.h"
#include "Display.h"
#include "Input.h"
#include "Frontend.h"
#include "Controls.h"
#include "UiLayout.h"
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif
#include <SFML/Graphics.hpp>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

void check(bool condition, const std::string& message)
{
    if (!condition) throw std::runtime_error(message);
}

void press(Input& input, sf::RenderWindow& window, Chip8& chip8,
           FrontendState& state, sf::Keyboard::Scancode code)
{
    sf::Event::KeyPressed event;
    event.scancode = code;
    input.handleEvent(sf::Event(event), window, chip8, state);
}

void release(Input& input, sf::RenderWindow& window, Chip8& chip8,
             FrontendState& state, sf::Keyboard::Scancode code)
{
    sf::Event::KeyReleased event;
    event.scancode = code;
    input.handleEvent(sf::Event(event), window, chip8, state);
}

void click(Input& input, sf::RenderWindow& window, Chip8& chip8,
           FrontendState& state, const sf::FloatRect& area,
           sf::Mouse::Button button = sf::Mouse::Button::Left)
{
    sf::Event::MouseButtonPressed event;
    event.button = button;
    event.position = sf::Vector2i(area.position + area.size / 2.f);
    input.handleEvent(sf::Event(event), window, chip8, state);
}

void sameCore(const Chip8& a, const Chip8& b)
{
    check(std::equal(a.memory, a.memory + 4096, b.memory), "Mode switch changed memory");
    check(std::equal(a.V, a.V + 16, b.V), "Mode switch changed registers");
    check(std::equal(a.gfx, a.gfx + 2048, b.gfx), "Mode switch changed framebuffer");
    check(std::equal(a.key, a.key + 16, b.key), "Mode switch changed keys");
    check(std::equal(a.stack, a.stack + 16, b.stack), "Mode switch changed stack");
    check(a.pc == b.pc && a.I == b.I && a.opcode == b.opcode && a.sp == b.sp,
          "Mode switch changed CPU state");
    check(a.delay_timer == b.delay_timer && a.sound_timer == b.sound_timer &&
          a.running == b.running && a.drawFlag == b.drawFlag,
          "Mode switch changed timers/flags");
}

void testInputAndTiming()
{
    Chip8 chip8;
    chip8.init();
    FrontendState state;
    state.romLoaded = true;
    sf::RenderWindow window; // Event handler can be exercised without opening it.
    Input input;
    const sf::Keyboard::Scancode codes[16] = {
        sf::Keyboard::Scancode::Num1, sf::Keyboard::Scancode::Num2,
        sf::Keyboard::Scancode::Num3, sf::Keyboard::Scancode::Num4,
        sf::Keyboard::Scancode::Q, sf::Keyboard::Scancode::W,
        sf::Keyboard::Scancode::E, sf::Keyboard::Scancode::R,
        sf::Keyboard::Scancode::A, sf::Keyboard::Scancode::S,
        sf::Keyboard::Scancode::D, sf::Keyboard::Scancode::F,
        sf::Keyboard::Scancode::Z, sf::Keyboard::Scancode::X,
        sf::Keyboard::Scancode::C, sf::Keyboard::Scancode::V
    };
    const int indices[16] = {1,2,3,12, 4,5,6,13, 7,8,9,14, 10,0,11,15};
    for (int i = 0; i < 16; ++i) {
        press(input, window, chip8, state, codes[i]);
        for (int key = 0; key < 16; ++key) {
            check(chip8.key[key] == (key == indices[i] ? 1 : 0), "Wrong keypad mapping");
        }
        release(input, window, chip8, state, codes[i]);
        check(chip8.key[indices[i]] == 0, "Key release was lost");
    }
    press(input, window, chip8, state, sf::Keyboard::Scancode::Num1);
    press(input, window, chip8, state, sf::Keyboard::Scancode::X);
    check(chip8.key[1] && chip8.key[0], "Simultaneous keys were lost");
    chip8.V[3] = 0xAB;
    chip8.memory[0x300] = 0xFF;
    chip8.delay_timer = 60;
    Chip8 snapshot = chip8; // Test-only snapshot, never used by the frontend.
    press(input, window, chip8, state, sf::Keyboard::Scancode::F1);
    check(state.debugMode, "F1 did not enter Debug Mode");
    sameCore(chip8, snapshot);
    press(input, window, chip8, state, sf::Keyboard::Scancode::F1);
    check(!state.debugMode, "F1 did not return to Play Mode");
    sameCore(chip8, snapshot);

    // A tiny synthetic program: add 1 to V0, then jump back.
    chip8.key[0] = chip8.key[1] = 0;
    chip8.memory[0x200] = 0x70; chip8.memory[0x201] = 0x01;
    chip8.memory[0x202] = 0x12; chip8.memory[0x203] = 0x00;
    check(state.paused, "ROM must start paused");
    updateEmulation(chip8, state, 1);
    check(chip8.V[0] == 0 && chip8.pc == 0x200 && chip8.delay_timer == 60,
          "Startup executed instructions or timers before Play");
    click(input, window, chip8, state, UiLayout::pauseButton(false));
    check(!state.paused && state.clearTiming, "Click did not start playback");
    updateEmulation(chip8, state, 1); // Discard time spent paused.
    for (int frame = 0; frame < 10; ++frame) updateEmulation(chip8, state, 0.1);
    check(chip8.V[0] == 94 && chip8.pc == 0x200, "700 Hz CPU schedule is wrong");
    check(chip8.delay_timer == 0, "Timers did not run at 60 Hz");

    press(input, window, chip8, state, sf::Keyboard::Scancode::Space);
    chip8.delay_timer = 20;
    int original = chip8.V[0];
    updateEmulation(chip8, state, 0.1);
    check(chip8.V[0] == original && chip8.delay_timer == 20, "Pause did not freeze CPU/timers");
    press(input, window, chip8, state, sf::Keyboard::Scancode::F2);
    updateEmulation(chip8, state, 0.1);
    check(chip8.V[0] == original + 1 && chip8.pc == 0x202, "F2 did not execute exactly one cycle");
    updateEmulation(chip8, state, 0.1);
    check(chip8.pc == 0x202 && chip8.delay_timer == 20, "Step repeated or changed timers");
    press(input, window, chip8, state, sf::Keyboard::Scancode::F2);
    press(input, window, chip8, state, sf::Keyboard::Scancode::F2);
    updateEmulation(chip8, state, 0);
    check(chip8.pc == 0x202 && chip8.V[0] == original + 2, "Two step presses did not execute twice");

    press(input, window, chip8, state, sf::Keyboard::Scancode::Q);
    press(input, window, chip8, state, sf::Keyboard::Scancode::V);
    check(chip8.key[4] && chip8.key[15], "Focus test did not start with held keys");
    input.handleEvent(sf::Event(sf::Event::FocusLost{}), window, chip8, state);
    for (uint8_t key : chip8.key) check(key == 0, "Focus loss left a stuck key");
    int pcBeforeFocus = chip8.pc;
    updateEmulation(chip8, state, 0.1);
    check(chip8.pc == pcBeforeFocus, "CPU ran without focus");
    press(input, window, chip8, state, sf::Keyboard::Scancode::Q);
    check(chip8.key[4] == 0, "Unfocused window accepted key input");
    input.handleEvent(sf::Event(sf::Event::FocusGained{}), window, chip8, state);
    press(input, window, chip8, state, sf::Keyboard::Scancode::Space);
    check(!state.paused, "Space did not resume");
    press(input, window, chip8, state, sf::Keyboard::Scancode::F2);
    check(state.stepRequests == 0, "F2 queued while running");
    updateEmulation(chip8, state, 5);
    check(chip8.pc == pcBeforeFocus, "Resume caught up paused time");

    for (int i = 0; i < 30; ++i) press(input, window, chip8, state, sf::Keyboard::Scancode::Equal);
    check(state.cpuHz == 2000, "CPU upper speed limit failed");
    for (int i = 0; i < 30; ++i) press(input, window, chip8, state, sf::Keyboard::Scancode::Hyphen);
    check(state.cpuHz == 100, "CPU lower speed limit failed");

    // FX0A waits in the core, but 60 Hz timers must keep ticking while running.
    chip8.pc = 0x200;
    chip8.memory[0x200] = 0xF0; chip8.memory[0x201] = 0x0A;
    chip8.delay_timer = 10;
    state.clearTiming = false;
    state.cycleAccumulator = state.timerAccumulator = 0;
    updateEmulation(chip8, state, 0.1);
    check(chip8.pc == 0x200 && chip8.delay_timer == 4, "Key wait blocked timer ticks");

    // An invalid opcode must leave the UI alive and expose a stopped status.
    chip8.memory[0x200] = 0xFF; chip8.memory[0x201] = 0xFF;
    updateEmulation(chip8, state, 0.1);
    check(!chip8.running && !state.message.empty(), "Core failure was not surfaced");
    std::cout << "PASS: keypad, mode isolation, timing, pause/step, focus, speed, faults\n";
}

void testMouseControls()
{
    Chip8 chip8;
    chip8.init();
    FrontendState state;
    state.romLoaded = true;
    sf::RenderWindow window;
    Input input;
    Chip8 snapshot = chip8;
    click(input, window, chip8, state, UiLayout::debugButton);
    check(state.debugMode && state.paused, "Debug button changed pause or failed");
    sameCore(chip8, snapshot);
    click(input, window, chip8, state, UiLayout::expandButton);
    check(!state.debugMode && state.paused, "Expand button changed pause or failed");
    sameCore(chip8, snapshot);
    click(input, window, chip8, state, UiLayout::pauseButton(false), sf::Mouse::Button::Right);
    check(state.paused, "Right click activated Play");
    click(input, window, chip8, state, sf::FloatRect({0, 0}, {2, 2}));
    check(state.paused, "Click outside button activated Play");
    click(input, window, chip8, state, UiLayout::pauseButton(false));
    check(!state.paused, "Play button failed");
    click(input, window, chip8, state, UiLayout::debugButton);
    check(state.debugMode && !state.paused, "Running mode switch paused execution");
    click(input, window, chip8, state, UiLayout::pauseButton(true));
    check(state.paused && state.clearTiming, "Debug pause button failed");
    sameCore(chip8, snapshot);
    input.handleEvent(sf::Event(sf::Event::MouseMoved{{300, 200}}), window, chip8, state);
    check(state.mousePosition == sf::Vector2f(300, 200), "Hover position was lost");
    input.handleEvent(sf::Event(sf::Event::MouseLeft{}), window, chip8, state);
    check(state.mousePosition == sf::Vector2f(-1, -1), "Mouse exit left hover active");
    state.romLoaded = false;
    click(input, window, chip8, state, UiLayout::pauseButton(true));
    check(state.paused, "No-ROM button changed playback");
    state.romLoaded = true;
    state.focused = false;
    click(input, window, chip8, state, UiLayout::pauseButton(true));
    check(state.paused, "Unfocused window accepted a click");
    std::cout << "PASS: mouse play/pause, mode isolation, hit areas, hover and focus\n";
}

void testReset()
{
    sf::RenderWindow resetWindow;
    Input resetInput;
    std::filesystem::create_directories("test-output");
    const char program[] = {0x60, 0x2A, 0x12, 0x00};
    std::ofstream rom("test-output/reset.ch8", std::ios::binary);
    rom.write(program, sizeof(program));
    rom.close();

    Chip8 chip8;
    FrontendState state;
    check(!loadCurrentRom(chip8, state) && !state.message.empty(), "No-ROM reset was not handled");
    state.romPath = "test-output/reset.ch8";
    check(loadCurrentRom(chip8, state), "Test ROM failed to load");
    updateEmulation(chip8, state, 1);
    check(state.paused && chip8.pc == 0x200, "Loaded ROM did not stay paused");
    chip8.emulate_cycle();
    check(chip8.V[0] == 42, "Test ROM did not run through the existing core");
    chip8.running = false;
    state.debugMode = true;
    state.paused = true;
    press(resetInput, resetWindow, chip8, state, sf::Keyboard::Scancode::F5);
    state.stepRequests = 1;
    updateEmulation(chip8, state, 1);
    check(chip8.running && chip8.pc == 0x200 && chip8.V[0] == 0, "F5 failed to reset the core");
    check(chip8.memory[0x200] == 0x60 && state.romLoaded, "F5 did not reload the ROM");
    check(state.paused && state.debugMode && state.stepRequests == 0,
          "Reset changed UI mode/pause or executed a pending step");
    check(state.message.empty(), "Reset left a stale error message");
    state.romPath = "test-output/does-not-exist.ch8";
    press(resetInput, resetWindow, chip8, state, sf::Keyboard::Scancode::F5);
    updateEmulation(chip8, state, 0.1);
    check(!state.romLoaded && !chip8.running && !state.message.empty(), "Failed reset was not reported");
    state.romPath = "test-output/reset.ch8";
    press(resetInput, resetWindow, chip8, state, sf::Keyboard::Scancode::F5);
    updateEmulation(chip8, state, 0.1);
    check(state.romLoaded && chip8.running, "F5 could not recover from failed reload");
    std::cout << "PASS: no-ROM reset, reload, paused reset, failure and recovery\n";
}

void testControlsFiles()
{
    std::filesystem::path root = std::filesystem::absolute("test-output/controls-fixture");
    std::filesystem::create_directories(root / "roms");
    std::filesystem::create_directories(root / "controls");
    FrontendState state;
    state.romPath = (root / "roms/game.rom").string();
    {
        std::ofstream file(root / "controls/game.txt", std::ios::binary);
        file << "\xEF\xBB\xBF# Source comment\r\n\r\n  Q - ROTATE  \r\nW - LEFT\r\n";
    }
    loadGameControls(state);
    check(state.controls.size() == 2 && state.controls[0] == "Q - ROTATE" &&
          state.controls[1] == "W - LEFT", "Controls parser or absolute ROM lookup failed");
    state.romPath = (root / "roms/missing.ch8").string();
    loadGameControls(state);
    check(state.controls.empty(), "Missing controls left stale game descriptions");
    state.romPath = (root / "roms/game.rom").string();
    {
        std::ofstream file(root / "controls/game.txt");
        file << "# Empty description\n\n";
    }
    loadGameControls(state);
    check(state.controls.empty(), "Empty file should show unavailable controls");
    {
        std::ofstream file(root / "controls/game.txt");
        for (int i = 0; i < 8; ++i) file << "KEY " << i << " - ACTION\n";
        std::ofstream rom(root / "roms/game.rom", std::ios::binary);
        const char program[] = {0x12, 0x00};
        rom.write(program, sizeof(program));
    }
    Chip8 chip8;
    check(loadCurrentRom(chip8, state), "Controls fixture ROM load failed");
    check(state.controls.size() == 6, "Controls exceeded the six-line panel");
    {
        std::ofstream file(root / "controls/game.txt");
        file << "E - UPDATED ACTION\n";
    }
    state.resetRequested = true;
    updateEmulation(chip8, state, 1);
    check(state.controls.size() == 1 && state.controls[0] == "E - UPDATED ACTION",
          "F5 did not reload edited controls");
    check(state.paused && chip8.pc == 0x200, "Reload changed paused startup");
    state.romPath = (root / "roms/missing.ch8").string();
    check(!loadCurrentRom(chip8, state) && state.controls.empty(),
          "Failed ROM load kept controls from the previous ROM");
    // A ROM elsewhere can still use controls under the current working directory.
    auto originalFolder = std::filesystem::current_path();
    std::filesystem::create_directories(root / "external/roms");
    state.romPath = (root / "external/roms/game.ch8").string();
    std::filesystem::current_path(root);
    loadGameControls(state);
    std::filesystem::current_path(originalFolder);
    check(state.controls.size() == 1 && state.controls[0] == "E - UPDATED ACTION",
          "Working-directory controls fallback failed");
    state.romPath.clear();
    loadGameControls(state);
    check(state.controls.empty(), "No-ROM state kept stale descriptions");
    std::cout << "PASS: controls lookup, comments, missing/empty files, line limit and F5 reload\n";
}

void runUntil(Chip8& chip8, uint16_t address)
{
    for (int cycle = 0; cycle < 20000 && chip8.running; ++cycle) {
        if (chip8.pc == address) return;
        chip8.emulate_cycle();
        if (cycle % 10 == 0) chip8.update_timers();
    }
    check(false, "ROM did not reach the expected input point");
}

// Optional checks for the exact two local ROM versions documented in controls/.
void testRomControls(const char* pongPath, const char* tetrisPath)
{
    Input input;
    sf::RenderWindow window;
    Chip8 pong;
    FrontendState state;
    state.romPath = pongPath;
    check(loadCurrentRom(pong, state) && state.controls.size() == 4, "Pong controls not loaded");
    runUntil(pong, 0x232);
    const sf::Keyboard::Scancode keys[] = {sf::Keyboard::Scancode::Num1,
        sf::Keyboard::Scancode::Q, sf::Keyboard::Scancode::Num4, sf::Keyboard::Scancode::R};
    const uint16_t addresses[] = {0x232, 0x238, 0x244, 0x24A};
    for (int i = 0; i < 4; ++i) {
        Chip8 game = pong;
        game.pc = addresses[i];
        int paddle = i < 2 ? 0xB : 0xD;
        game.V[paddle] = 12;
        game.V[0] = i == 0 ? 1 : i == 1 ? 4 : i == 2 ? 12 : 13;
        press(input, window, game, state, keys[i]);
        game.emulate_cycle(); // Key test from the ROM.
        game.emulate_cycle(); // Paddle movement from the ROM.
        check(game.V[paddle] == (i % 2 == 0 ? 10 : 14), "Pong documented direction failed");
    }
    Chip8 tetris;
    state.romPath = tetrisPath;
    check(loadCurrentRom(tetris, state) && state.controls.size() == 4, "Tetris controls not loaded");
    runUntil(tetris, 0x23C);
    check(tetris.V[7] == 5 && tetris.V[8] == 6 && tetris.V[9] == 4 && tetris.V[2] == 7,
          "Unexpected Tetris ROM key bindings");
    const sf::Keyboard::Scancode tetrisKeys[] = {sf::Keyboard::Scancode::W,
        sf::Keyboard::Scancode::E, sf::Keyboard::Scancode::Q, sf::Keyboard::Scancode::A};
    const uint16_t entry[] = {0x23C, 0x240, 0x244, 0x248};
    const uint16_t finish[] = {0x240, 0x244, 0x248, 0x250};
    for (int i = 0; i < 4; ++i) {
        Chip8 game = tetris;
        game.pc = entry[i];
        game.delay_timer = 20;
        int oldX = game.V[0];
        int oldRotation = game.V[3];
        press(input, window, game, state, tetrisKeys[i]);
        // Stop before other keys or the automatic fall are processed.
        // The ROM includes a busy-wait loop after moving or rotating.
        for (int cycle = 0; cycle < 1000 && game.pc != finish[i] && game.running; ++cycle)
            game.emulate_cycle();
        check(game.pc == finish[i], "Tetris key action " + std::to_string(i) + " stopped at PC " + std::to_string(game.pc));
        if (i == 0) check(game.V[0] == oldX - 1, "W did not move Tetris left");
        if (i == 1) check(game.V[0] == oldX + 1, "E did not move Tetris right");
        if (i == 2) check(game.V[3] == (oldRotation + 1) % 4, "Q did not rotate Tetris");
        if (i == 3) check(game.delay_timer == 0, "A did not remove Tetris fall delay");
    }
    // Preview the real game's controls and initial board in the fixed Debug UI.
    state.debugMode = true;
    Display display;
    sf::RenderTexture target;
    check(target.resize({1366, 768}), "ROM screenshot target failed");
    display.drawGraphics(target, tetris, state);
    target.display();
    check(target.getTexture().copyToImage().saveToFile("test-output/tetris-controls.png"),
          "Could not save controls screenshot");
    std::cout << "PASS: local Pong paddle directions and Tetris move/rotate/faster-drop controls\n";
}

void testRendering()
{
    Chip8 chip8;
    chip8.init();
    // Have the existing core draw its own font; the renderer only sees gfx.
    for (int digit = 0; digit < 16; ++digit) {
        chip8.V[0] = static_cast<uint8_t>(3 + (digit % 8) * 7);
        chip8.V[1] = static_cast<uint8_t>(8 + (digit / 8) * 10);
        chip8.I = static_cast<uint16_t>(0x50 + digit * 5);
        chip8.opcode = 0xD015;
        chip8.drawing();
    }
    for (int i = 0; i < 16; ++i) chip8.V[i] = static_cast<uint8_t>(i * 17);
    chip8.pc = 0x024A;
    chip8.I = 0x0310;
    chip8.delay_timer = 0x2A;
    chip8.sound_timer = 0x08;
    chip8.key[5] = chip8.key[15] = 1;
    FrontendState state;
    state.romLoaded = true;
    state.romPath = "roms/FRONTEND-CHECK.ch8";
    state.controls = {"Q - ROTATE PIECE", "W - MOVE LEFT", "E - MOVE RIGHT", "A - DROP FASTER (HOLD)"};
    state.debugMode = true;
    Display display;
    Chip8 snapshot = chip8;
    sf::RenderTexture target;
    check(target.resize({1366, 768}), "Could not create render target");
    display.drawGraphics(target, chip8, state);
    target.display();
    check(target.getTexture().copyToImage().saveToFile("test-output/debug.png"), "Could not save Debug screenshot");
    sameCore(chip8, snapshot);

    state.paused = false;
    state.mousePosition = {515, 292};
    display.drawGraphics(target, chip8, state);
    target.display();
    check(target.getTexture().copyToImage().saveToFile("test-output/debug-hover.png"), "Could not save hover screenshot");

    check(target.resize({1366, 768}), "Could not restore render size");
    state.debugMode = false;
    state.paused = true;
    state.mousePosition = {-1, -1};
    display.drawGraphics(target, chip8, state);
    target.display();
    check(target.getTexture().copyToImage().saveToFile("test-output/play.png"), "Could not save Play screenshot");
    sameCore(chip8, snapshot);

    // A single framebuffer bit must produce exactly a 21x21 square here.
    Chip8 pixelTest;
    pixelTest.gfx[0] = 1;
    state.paused = false;
    display.drawGraphics(target, pixelTest, state);
    target.display();
    sf::Image image = target.getTexture().copyToImage();
    sf::Color on = image.getPixel({11, 64});
    check(on == sf::Color(140, 233, 190), "First pixel has wrong position/color");
    for (unsigned int y = 64; y < 85; ++y) {
        for (unsigned int x = 11; x < 32; ++x) {
            check(image.getPixel({x, y}) == on, "Framebuffer pixel is blurred or distorted");
        }
    }
    check(image.getPixel({32, 64}) != on && image.getPixel({11, 85}) != on,
          "Integer pixel scale is wrong");
    std::cout << "PASS: rendering leaves core intact, fixed Play/Debug, pause/hover and crisp integer pixels\n";
}

void testShortSoundTimer()
{
    Chip8 chip8;
    chip8.init();
    FrontendState state;
    state.romLoaded = true;
    state.paused = false;
    // Set ST=1, then loop without setting it again.
    chip8.memory[0x200] = 0x60; chip8.memory[0x201] = 0x01;
    chip8.memory[0x202] = 0xF0; chip8.memory[0x203] = 0x18;
    chip8.memory[0x204] = 0x12; chip8.memory[0x205] = 0x04;
    updateEmulation(chip8, state, 1.0 / 60);
    check(chip8.sound_timer == 1, "New one-tick sound expired before audio could see it");
    updateEmulation(chip8, state, 1.0 / 60);
    check(chip8.sound_timer == 0, "One-tick sound did not expire on the next timer tick");
    std::cout << "PASS: FX18 one-tick beep survives until the next 60 Hz tick\n";
}

void testAudio()
{
    Audio audio;
    check(audio.setupAudio(), "Could not generate SFML buzzer samples");
    Chip8 chip8;
    chip8.init();
    FrontendState state;
    state.romLoaded = true;
    state.paused = false;
    chip8.sound_timer = 6;
    Chip8 snapshot = chip8;
    audio.update(chip8, state);
    check(audio.isPlaying(), "Positive timer did not start the buzzer");
    sameCore(chip8, snapshot);
    // The generated sample lasts 100 ms; playback must loop beyond its end.
    sf::sleep(sf::milliseconds(150));
    check(audio.isPlaying(), "Buzzer stopped instead of looping");
    state.debugMode = true;
    audio.update(chip8, state);
    check(audio.isPlaying(), "Switching to Debug silenced the buzzer");
    for (int tick = 0; tick < 6; ++tick) {
        chip8.update_timers();
        audio.update(chip8, state);
        check(audio.isPlaying() == (chip8.sound_timer > 0), "Timer expiry/audio state mismatch");
    }
    Input input;
    sf::RenderWindow window;
    chip8.sound_timer = 10;
    audio.update(chip8, state);
    press(input, window, chip8, state, sf::Keyboard::Scancode::Space);
    audio.update(chip8, state);
    check(!audio.isPlaying() && chip8.sound_timer == 10, "Pause did not silence sound");
    press(input, window, chip8, state, sf::Keyboard::Scancode::Space);
    audio.update(chip8, state);
    check(audio.isPlaying(), "Resume did not restore a still-active sound timer");
    input.handleEvent(sf::Event(sf::Event::FocusLost{}), window, chip8, state);
    audio.update(chip8, state);
    check(!audio.isPlaying(), "Focus loss did not silence sound");
    input.handleEvent(sf::Event(sf::Event::FocusGained{}), window, chip8, state);
    audio.update(chip8, state);
    check(audio.isPlaying(), "Refocus did not restore sound");
    chip8.running = false;
    audio.update(chip8, state);
    check(!audio.isPlaying(), "Core fault left the buzzer running");
    chip8.running = true;
    state.romLoaded = false;
    audio.update(chip8, state);
    check(!audio.isPlaying(), "No-ROM state enabled audio");

    // Load through the real frontend; F2 may set ST while paused but stays silent.
    std::filesystem::create_directories("test-output");
    {
        std::ofstream rom("test-output/buzzer.ch8", std::ios::binary);
        const char program[] = {0x60, 0x06, static_cast<char>(0xF0), 0x18, 0x12, 0x04};
        rom.write(program, sizeof(program));
    }
    state.romPath = "test-output/buzzer.ch8";
    state.paused = true;
    check(loadCurrentRom(chip8, state), "Audio fixture ROM failed to load");
    press(input, window, chip8, state, sf::Keyboard::Scancode::F2);
    press(input, window, chip8, state, sf::Keyboard::Scancode::F2);
    updateEmulation(chip8, state, 0);
    audio.update(chip8, state);
    check(chip8.sound_timer == 6 && !audio.isPlaying(), "Paused F2 step produced sound");
    press(input, window, chip8, state, sf::Keyboard::Scancode::Space);
    audio.update(chip8, state);
    check(audio.isPlaying(), "Stepped sound timer did not play on resume");
    press(input, window, chip8, state, sf::Keyboard::Scancode::F5);
    updateEmulation(chip8, state, 1);
    audio.update(chip8, state);
    check(chip8.sound_timer == 0 && !audio.isPlaying(), "Reset left old audio playing");
    // Exercise the same updateEmulation -> audio.update sequence as main.
    chip8.memory[0x201] = 1;
    updateEmulation(chip8, state, 1.0 / 60);
    audio.update(chip8, state);
    check(audio.isPlaying(), "A one-tick FX18 beep did not reach audio output");
    updateEmulation(chip8, state, 1.0 / 60);
    audio.update(chip8, state);
    check(!audio.isPlaying(), "Expired one-tick beep kept playing");
    chip8.sound_timer = 6;
    audio.update(chip8, state);
    audio.stop();
    check(!audio.isPlaying(), "Explicit shutdown did not stop audio");
    std::cout << "PASS: SFML buzzer playback, loop, expiry, pause/step, focus, reset, faults and shutdown\n";
}

void testWindow()
{
    Display display;
    check(display.setupGraphics(), "Native SFML window creation failed");
    sf::RenderWindow& window = display.getWindow();
    window.setVisible(false);
    check(window.isOpen(), "Native window is not open");
    Chip8 chip8;
    chip8.init();
    FrontendState state;
    state.romLoaded = true;
    state.romPath = "window-smoke.ch8";
    Input input;
    display.render(chip8, state);
    press(input, window, chip8, state, sf::Keyboard::Scancode::F1);
    display.render(chip8, state);
    check(window.getSize() == sf::Vector2u(1366, 768), "Wrong window size");
    check(window.getPosition() == sf::Vector2i(0, 0), "Window does not fill monitor from origin");
#ifdef _WIN32
    check((GetWindowLongPtr(window.getNativeHandle(), GWL_STYLE) & WS_CAPTION) == 0,
          "Window still has a native title bar");
    chip8.key[1] = 1;
    click(input, window, chip8, state, UiLayout::minimizeButton);
    check(IsIconic(window.getNativeHandle()) != 0, "Minimize button did not minimize");
    check(!state.focused && chip8.key[1] == 0 && state.clearTiming,
          "Minimize did not freeze execution and release keys");
    ShowWindow(window.getNativeHandle(), SW_RESTORE);
    window.setVisible(false);
    input.handleEvent(sf::Event(sf::Event::FocusGained{}), window, chip8, state);
    chip8.key[4] = 1;
    state.paused = false;
    press(input, window, chip8, state, sf::Keyboard::Scancode::Escape);
    check(window.isOpen() && IsIconic(window.getNativeHandle()) != 0,
          "Escape must minimize without closing");
    check(!state.focused && chip8.key[4] == 0 && !state.paused,
          "Escape did not release keys or changed explicit pause state");
    ShowWindow(window.getNativeHandle(), SW_HIDE);
    input.handleEvent(sf::Event(sf::Event::FocusGained{}), window, chip8, state);
#endif
    click(input, window, chip8, state, UiLayout::closeButton);
    check(!window.isOpen(), "Clickable X failed to close the window");
    check(display.setupGraphics(), "Could not reopen for close event");
    window.setVisible(false);
    input.handleEvent(sf::Event(sf::Event::Closed{}), window, chip8, state);
    check(!window.isOpen(), "Window close event was ignored");
    std::cout << "PASS: native window creation, Play/Debug rendering, fixed borderless size, minimize, Esc, X and close event\n";
}

int main(int argc, char* argv[])
{
    try {
        if (argc > 1 && std::string(argv[1]) == "--audio") {
            testAudio();
            return 0;
        }
        if (argc > 1 && std::string(argv[1]) == "--window") {
            testWindow();
            return 0;
        }
        if (argc == 4 && std::string(argv[1]) == "--rom-controls") {
            testRomControls(argv[2], argv[3]);
            return 0;
        }
        testShortSoundTimer();
        testInputAndTiming();
        testMouseControls();
        testReset();
        testControlsFiles();
        testRendering();
        std::cout << "All frontend tests passed. Screenshots are in test-output/.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
