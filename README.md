# CHIP-8 emulator + SFML frontend

This project uses your existing CHIP-8 core with an SFML 3 frontend. Play Mode shows the game; Debug Mode shows the same game alongside the live CPU state and keypad.

The frontend does not implement any opcodes. It reads `gfx`, `V`, `key`, `pc`, `I`, `sp`, `opcode`, and the timers from your existing `Chip8` object. The core files and their public interface were left unchanged.

## Build and run on this machine

SFML **3.0.2** and MinGW are already installed under `C:\msys64\mingw64`. These commands use that toolchain (not an SFML 2 tutorial or an MSVC-built library).

From the project folder in PowerShell:

```powershell
New-Item -ItemType Directory -Force build
g++ -std=c++17 -Wall -Wextra -Wpedantic src/*.cpp -o build/chip8.exe -lsfml-graphics -lsfml-window -lsfml-system
./build/chip8.exe "roms/your-game.ch8"
```

Replace the example path with your own ROM. No game ROMs are bundled.

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

| Key | Action |
| --- | --- |
| F1 | Switch Play / Debug layout; keeps the current program and CPU state |
| Space | Pause / resume |
| F2 | Execute exactly one normal CPU cycle while paused |
| F5 | Reload the same ROM using the existing `load()` / `init()`; preserve mode and pause setting |
| Esc | Close the window |
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

## What each frontend file does

| File | Responsibility |
| --- | --- |
| `src/Display.h/.cpp` | Own the window; render the existing framebuffer, Play/Debug layouts, registers, keypad, last fetched opcode and timers |
| `src/Input.h/.cpp` | Handle SFML events, write physical key states into `chip8.key`, and request UI actions |
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
- A long window drag/stall catches up at most 100 ms. This keeps the UI responsive.
- F1 only changes rendering. It does not recreate the window or reload the ROM.
- Pixels use the largest whole-number scale that fits, remain square, and are centered. The minimum window is 960 x 720; both modes adapt to resizing.
- The UI is redrawn even when `drawFlag` is false so controls and debug values stay visible. Main clears `drawFlag` after rendering.
- The opcode panel shows the core's last fetched `opcode`; PC is the core's current PC. Before the first cycle the opcode is 0000.
- Registers, PC, I, SP and timer values are hexadecimal. In the stack indicator, `10` means 16 slots.
- The sound timer and active sound signal are displayed. Audible sound is not implemented in this frontend.

The UI uses a small built-in pixel alphabet, so no `.ttf` file, download, system-font lookup or font configuration is necessary. Unsupported filename characters are displayed as `?` in the pixel UI; the native window title keeps the filename text.

Core errors appear in the terminal and as a stopped status in the UI. F5 can retry the existing ROM. Frontend errors such as window-creation failure are reported before exiting.

## Optional CMake build

With CMake installed and the same MinGW toolchain available:

```powershell
cmake -S . -B build/cmake -G "MinGW Makefiles" -DCMAKE_PREFIX_PATH=C:/msys64/mingw64
cmake --build build/cmake
./build/cmake/chip8.exe "roms/your-game.ch8" --debug
```

CMake is not required for the direct `g++` command above. On other platforms, install SFML 3 for your compiler, then use `cmake -S . -B build/cmake` and `cmake --build build/cmake`.

## Frontend integration tests

The tests use small synthetic programs, not downloaded game ROMs. They deliberately trigger a core fault and a missing ROM to check error handling; those error messages are expected.

```powershell
$coreFiles = (Get-ChildItem src/Chip8*.cpp).FullName
g++ -std=c++17 -Wall -Wextra -Wpedantic -Isrc tests/FrontendTests.cpp src/Display.cpp src/Input.cpp src/Frontend.cpp $coreFiles -o build/frontend_tests.exe -lsfml-graphics -lsfml-window -lsfml-system
Push-Location build
./frontend_tests.exe
Pop-Location
```

Checks cover all 16 key mappings, simultaneous keys, focus loss, mode switching without core changes, pause/resume, one-cycle steps, CPU rate limits, 60 Hz timers, key waits, reset/reload failures, and pixel scaling. Screenshots are saved under `build/test-output/` (`debug.png`, `debug-small.png`, `play.png`). The rendering tests need an OpenGL-capable environment even though they draw offscreen.

For a real-window lifecycle check, run `./frontend_tests.exe --window` from the build folder. It briefly creates a window, hides it, renders both modes and checks close controls.

With CMake, enable tests with `-DCHIP8_BUILD_TESTS=ON`, build, then run `ctest --test-dir build/cmake --output-on-failure`.
