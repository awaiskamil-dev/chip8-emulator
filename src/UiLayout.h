#ifndef UI_LAYOUT_H
#define UI_LAYOUT_H

#include <SFML/Graphics/Rect.hpp>

// Fixed coordinates for the presentation monitor: 1366 x 768 at 100% scaling.
// Display and Input share these rectangles so buttons match their click areas.
namespace UiLayout
{
inline constexpr unsigned int width = 1366;
inline constexpr unsigned int height = 768;
inline const sf::FloatRect minimizeButton({1246, 20}, {48, 32});
inline const sf::FloatRect closeButton({1302, 20}, {48, 32});
inline const sf::FloatRect debugButton({1080, 20}, {120, 32});
inline const sf::FloatRect expandButton({950, 110}, {40, 32});
inline const sf::FloatRect playScreen({11, 64}, {1344, 672});
inline const sf::FloatRect debugScreen({227, 148}, {576, 288});

inline sf::FloatRect screen(bool debugMode)
{
    return debugMode ? debugScreen : playScreen;
}

inline sf::FloatRect pauseButton(bool debugMode)
{
    sf::FloatRect area = screen(debugMode);
    return sf::FloatRect(area.position + area.size / 2.f - sf::Vector2f(32, 32), {64, 64});
}
}

#endif
