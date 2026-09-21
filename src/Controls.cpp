#include "Controls.h"
#include <filesystem>
#include <fstream>

void loadGameControls(FrontendState& state)
{
    state.controls.clear();
    if (state.romPath.empty()) return;

    std::filesystem::path romPath(state.romPath);
    std::filesystem::path filename = romPath.stem();
    filename += ".txt";
    // For project/roms/Game.ch8, look for project/controls/Game.txt.
    std::filesystem::path folder = romPath.parent_path().parent_path() / "controls";
    std::ifstream file(folder / filename);
    if (!file.is_open()) {
        file.clear();
        file.open(std::filesystem::path("controls") / filename);
    }
    if (!file.is_open()) return;

    std::string line;
    while (state.controls.size() < 6 && std::getline(file, line)) {
        // Some Windows text editors save a UTF-8 byte-order mark.
        if (line.compare(0, 3, "\xEF\xBB\xBF") == 0) line.erase(0, 3);
        std::size_t first = line.find_first_not_of(" \t\r");
        if (first == std::string::npos || line[first] == '#') continue;
        std::size_t last = line.find_last_not_of(" \t\r");
        state.controls.push_back(line.substr(first, last - first + 1));
    }
}
