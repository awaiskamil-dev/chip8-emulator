#include "Chip8.h"
#include "Display.h"
#include "Input.h"
#include "Frontend.h"
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
    state.debugMode = true;
    Display display;
    Chip8 snapshot = chip8;
    sf::RenderTexture target;
    check(target.resize({1280, 860}), "Could not create render target");
    display.drawGraphics(target, chip8, state);
    target.display();
    check(target.getTexture().copyToImage().saveToFile("test-output/debug.png"), "Could not save Debug screenshot");
    sameCore(chip8, snapshot);

    check(target.resize({960, 720}), "Could not resize render target");
    state.paused = true;
    display.drawGraphics(target, chip8, state);
    target.display();
    check(target.getTexture().copyToImage().saveToFile("test-output/debug-small.png"), "Could not save resized screenshot");

    check(target.resize({1280, 860}), "Could not restore render size");
    state.debugMode = false;
    display.drawGraphics(target, chip8, state);
    target.display();
    check(target.getTexture().copyToImage().saveToFile("test-output/play.png"), "Could not save Play screenshot");
    sameCore(chip8, snapshot);

    // A single framebuffer bit must produce exactly a 10x10 square here.
    Chip8 pixelTest;
    pixelTest.gfx[0] = 1;
    check(target.resize({688, 368}), "Pixel test target failed");
    display.drawGraphics(target, pixelTest, state);
    target.display();
    sf::Image image = target.getTexture().copyToImage();
    sf::Color on = image.getPixel({24, 24});
    check(on == sf::Color(140, 233, 190), "First pixel has wrong position/color");
    for (unsigned int y = 24; y < 34; ++y) {
        for (unsigned int x = 24; x < 34; ++x) {
            check(image.getPixel({x, y}) == on, "Framebuffer pixel is blurred or distorted");
        }
    }
    check(image.getPixel({34, 24}) != on && image.getPixel({24, 34}) != on,
          "Integer pixel scale is wrong");
    std::cout << "PASS: rendering leaves core intact, resize, Play/Debug and crisp integer pixels\n";
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
    window.setSize({960, 720});
    display.render(chip8, state);
    press(input, window, chip8, state, sf::Keyboard::Scancode::Escape);
    check(!window.isOpen(), "Escape failed to close the SFML window");
    check(display.setupGraphics(), "Could not reopen the test window");
    window.setVisible(false);
    input.handleEvent(sf::Event(sf::Event::Closed{}), window, chip8, state);
    check(!window.isOpen(), "Window close event was ignored");
    std::cout << "PASS: native window creation, Play/Debug rendering, resize, Esc and close event\n";
}

int main(int argc, char* argv[])
{
    try {
        if (argc > 1 && std::string(argv[1]) == "--window") {
            testWindow();
            return 0;
        }
        testInputAndTiming();
        testReset();
        testRendering();
        std::cout << "All frontend tests passed. Screenshots are in test-output/.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
