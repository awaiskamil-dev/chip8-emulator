# CHIP-8 emulator + SFML frontend

This project uses your existing CHIP-8 core with an SFML 3 frontend. Play Mode shows the game; Debug Mode shows the same game alongside the live CPU state and keypad.

The frontend does not implement any opcodes. It reads `gfx`, `V`, `key`, `pc`, `I`, `sp`, `opcode`, and the timers from your existing `Chip8` object. The core files and their public interface were left unchanged.

## Build and run on this machine

SFML **3.0.2** and MinGW are already installed under `C:\msys64\mingw64`. These commands use that toolchain (not an SFML 2 tutorial or an MSVC-built library).

From the project folder in PowerShell:

```powershell
New-Item -ItemType Directory -Force build
g++ -std=c++17 -Wall -Wextra -Wpedantic src/*.cpp -o build/chip8.exe -lsfml-graphics -lsfml-window -lsfml-system -lsfml-audio
./build/chip8.exe "roms/your-game.ch8"
```

Replace the example path with your own ROM. No game ROMs are bundled. ROMs now start paused: click the centered Play triangle or press Space to begin. `--paused` remains accepted for compatibility.

```powershell
# Open directly in Debug Mode, paused before the first instruction:
./build/chip8.exe "roms/your-game.ch8" --debug --paused

# Choose a CPU rate (instructions per second):
./build/chip8.exe "roms/your-game.ch8" --hz 900

# Inspect the frontend without a ROM, or print help:
./build/chip8.exe
./build/chip8.exe --help
```

An invalid ROM path prints an error and exits. Launching without a ROM opens the debugger with a clear no-ROM message; pressing F5 in that state is harmless. Supply a ROM path when starting the application to play.

If `g++` or SFML DLLs are not found, put `C:\msys64\mingw64\bin` on your terminal's PATH. Keep the SFML/compiler toolchains matched. The executable depends on the installed SFML/runtime DLLs; the `.exe` alone is not a standalone distributable.

## Controls

| Control | Action |
| --- | --- |
| F1 | Switch Play / Debug layout; keeps the current program and CPU state |
| Space / centered Play or Pause button | Pause / resume; Pause appears when hovering over the running game |
| Expand icon in Debug / Debug button in Play | Switch layouts while preserving execution and pause state |
| Top-right - / X | Minimize / close the window |
| F2 | Execute exactly one normal CPU cycle while paused |
| F5 | Reload the same ROM using the existing `load()` / `init()`; preserve mode and pause setting |
| Esc | Minimize the window; restore from the taskbar or Alt+Tab |
| + / - | Change CPU rate by 100 Hz, limited to 100-2000 Hz |

The main keyboard's `=` key also increases speed without needing Shift; numpad +/- work too. Holding F2 does not repeat steps: release and press it again.

```text
Physical keyboard       CHIP-8 key
1 2 3 4                 1 2 3 C
Q W E R                 4 5 6 D
A S D F                 7 8 9 E
Z X C V                 A 0 B F
```

Debug Mode highlights the real `key[16]` entries. The small character on each tile is its physical keyboard key. Mappings use physical key positions (scancodes).

## Game-specific controls

Debug Mode now shows 01 Display, 02 Controls, 03 Keypad, 04 Registers,
05 Current Opcode, and 06 Timers. The controls and keypad share the bottom row.
Play Mode keeps the large game display.

`roms/Pong.ch8` loads `controls/Pong.txt`; `roms/tetris.rom` loads
`controls/tetris.txt`. The descriptions use this emulator's physical keyboard:
Pong uses 1/Q for the left paddle and 4/R for the right paddle; Tetris uses
Q to rotate, W/E to move, and held A to drop faster.

Edit these plain text files to adjust the descriptions. Press F5 to reload the
text **and reset the game**, or restart the app. Missing descriptions do not stop
the game. See [controls/README.md](controls/README.md) for the format, lookup
rules, and documentation source. The text files never execute commands or remap keys.

Esc and the top-right minus button both minimize; the X button still exits.

## What each frontend file does

| File | Responsibility |
| --- | --- |
| `src/Display.h/.cpp` | Own the window; render the existing framebuffer, Play/Debug layouts, registers, keypad, last fetched opcode and timers |
| `src/Input.h/.cpp` | Handle SFML events, write physical key states into `chip8.key`, and request UI actions |
| `src/UiLayout.h` | Fixed screen and button coordinates shared by rendering and mouse input |
| `src/Audio.h/.cpp` | Generate the looping buzzer and gate playback using the existing sound timer, pause and focus state |
| `src/Controls.h/.cpp` | Read the matching game controls text file when loading or resetting a ROM |
| `src/Frontend.h/.cpp` | Small frontend settings structure and helpers for scheduling core calls / reloading a ROM |
| `src/main.cpp` | Read arguments, create the one core object, then process input, update emulation and render |
| `CMakeLists.txt` | Build with SFML 3; optionally build integration tests |
| `tests/FrontendTests.cpp` | Exercise frontend controls/timing and generate layout screenshots |

`FrontendState` only stores UI settings, the ROM path, messages and timing fractions. It does not copy registers, memory, pixels or keypad state. `Display` takes a read-only reference to `Chip8`.

The existing core ownership stays the same: Awais's initialization/arithmetic/drawing/keypad code is in `Chip8.cpp`; Mustafa's cycle, flow-control, ROM/memory and timer code is in the other `Chip8*.cpp` files.

## Timing and display behavior

- Default CPU speed is 700 instructions/second. Rendering is capped at 60 frames/second; a frame can execute several instructions.
- Timer scheduling calls the existing `update_timers()` at 60 Hz, independently of CPU speed. The core still performs the decrements.
- Explicit pause freezes CPU execution and timers. F2 steps the CPU only, leaving timers frozen for predictable debugging.
- An FX0A key wait is different from frontend pause: normal cycles keep calling the core and timers continue running.
- Losing window focus temporarily freezes execution and clears held keys. Refocusing resumes unless you explicitly paused; press game keys again. Paused time is not replayed in a catch-up burst.
- A long frame stall catches up at most 100 ms. This keeps the UI responsive.
- F1 only changes rendering. It does not recreate the window or reload the ROM.
- The borderless window is fixed at 1366 x 768, positioned at the top-left of the primary monitor. It is designed for your 1366 x 768 monitor at 100% scaling with the taskbar hidden; it is not resizable.
- Play Mode uses a centered 1344 x 672 game display (21 pixels per CHIP-8 pixel), leaving room for window controls. Debug Mode uses 576 x 288 (9 pixels per CHIP-8 pixel), with the complete keypad visible. Pixels stay square and the game keeps its 2:1 aspect ratio.
- The minimize button uses the Windows API because SFML has no minimize operation. Restore the window from the taskbar or with Alt+Tab. Focus loss freezes execution and releases keys; restoring keeps your explicit pause setting.
- The UI is redrawn even when `drawFlag` is false so controls and debug values stay visible. Main clears `drawFlag` after rendering.
- The opcode panel shows the core's last fetched `opcode`; PC is the core's current PC. Before the first cycle the opcode is 0000.
- Registers, PC, I, SP and timer values are hexadecimal. In the stack indicator, `10` means 16 slots.
- A generated 480 Hz square-wave buzzer plays while the sound timer is positive. It uses SFML Audio, with no sound files or downloads. The sample amplitude is 8000 and SFML volume is 20/100.
- Audio is silent while paused (including F2 stepping), unfocused/minimized, with no ROM, or after a core fault. Refocusing/resuming sounds the buzzer again if the timer is still positive. Reset normally clears the timer; closing the app stops playback.
- The frontend applies elapsed 60 Hz ticks before running the frame's CPU instructions so a new one-tick beep is not immediately erased. Audio is synchronized once per frame, so beep timing has frame-sized granularity.
- The sound timer display shows the core's value even when frontend pause/focus rules silence playback. A game must use FX18 to request a beep.

The UI uses a small built-in pixel alphabet, so no `.ttf` file, download, system-font lookup or font configuration is necessary. Unsupported filename characters are displayed as `?` in the pixel UI; the native window title keeps the filename text.

Core errors appear in the terminal and as a stopped status in the UI. F5 can retry the existing ROM. Frontend errors such as window-creation failure are reported before exiting.

## Sound

The buzzer uses the system's default audio output. If you see a positive ST while
running but hear nothing, check Windows volume/mute, the app's volume in the
Volume Mixer, and the selected output device. Keep `libsfml-audio-3.dll` and its
runtime dependencies available through `C:/msys64/mingw64/bin`, just like the
other SFML libraries. Sound is intentionally silent while paused or minimized.

## Optional CMake build

With CMake installed and the same MinGW toolchain available:

```powershell
cmake -S . -B build/cmake -G "MinGW Makefiles" -DCMAKE_PREFIX_PATH=C:/msys64/mingw64
cmake --build build/cmake
./build/cmake/chip8.exe "roms/your-game.ch8" --debug
```

CMake is not required for the direct `g++` command above. This fixed presentation layout and its minimize control target Windows.

## Frontend integration tests

The tests use small synthetic programs, not downloaded game ROMs. They deliberately trigger a core fault and a missing ROM to check error handling; those error messages are expected.

```powershell
$coreFiles = (Get-ChildItem src/Chip8*.cpp).FullName
g++ -std=c++17 -Wall -Wextra -Wpedantic -Isrc tests/FrontendTests.cpp src/Display.cpp src/Input.cpp src/Frontend.cpp src/Controls.cpp src/Audio.cpp $coreFiles -o build/frontend_tests.exe -lsfml-graphics -lsfml-window -lsfml-system -lsfml-audio
Push-Location build
./frontend_tests.exe
Pop-Location
```

Checks cover all 16 key mappings, simultaneous keys, focus loss, mode switching without core changes, pause/resume, one-cycle steps, CPU rate limits, 60 Hz timers, key waits, reset/reload failures, mouse controls, paused startup, controls-file parsing/reloading, and pixel scaling. Screenshots are saved under `build/test-output/` (`debug.png`, `debug-hover.png`, `play.png`). The rendering tests need an OpenGL-capable environment even though they draw offscreen.

For a real-window lifecycle check, run `./frontend_tests.exe --window` from the build folder. It briefly creates a window, hides it, renders both modes and checks the fixed size, borderless style, minimize and close controls.

To check the documented keys against your exact local Pong and Tetris ROMs:

```powershell
Push-Location build
./frontend_tests.exe --rom-controls ../roms/Pong.ch8 ../roms/tetris.rom
Pop-Location
```

This optional check executes the ROMs' own movement/rotation instructions through
the existing core and saves `build/test-output/tetris-controls.png`. It targets
the ROM versions identified in the controls files; it is not a full gameplay test.

With CMake, enable tests with `-DCHIP8_BUILD_TESTS=ON`, build, then run `ctest --test-dir build/cmake --output-on-failure`.

The optional audio check briefly plays the generated tone and requires a working
audio output device. From `build`, run `./frontend_tests.exe --audio`. It checks
short beeps, looping, timer expiry, pause/step, focus loss, reset and core faults.
It verifies SFML playback state; actual speaker volume still depends on Windows.
