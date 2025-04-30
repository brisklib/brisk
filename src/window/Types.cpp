/*
 * Brisk
 *
 * Cross-platform application framework
 * --------------------------------------------------------------
 *
 * Copyright (C) 2024 Brisk Developers
 *
 * This file is part of the Brisk library.
 *
 * Brisk is dual-licensed under the GNU General Public License, version 2 (GPL-2.0+),
 * and a commercial license. You may use, modify, and distribute this software under
 * the terms of the GPL-2.0+ license if you comply with its conditions.
 *
 * You should have received a copy of the GNU General Public License along with this program.
 * If not, see <http://www.gnu.org/licenses/>.
 *
 * If you do not wish to be bound by the GPL-2.0+ license, you must purchase a commercial
 * license. For commercial licensing options, please visit: https://brisklib.com
 */
#include <brisk/window/Types.hpp>

namespace Brisk {

const NameValueOrderedList<KeyCode> keyCodes{
    { "Spacebar", KeyCode::Space },
    { "'", KeyCode::Apostrophe },
    { ",", KeyCode::Comma },
    { "-", KeyCode::Minus },
    { ".", KeyCode::Period },
    { "/", KeyCode::Slash },
    { "0", KeyCode::Digit0 },
    { "1", KeyCode::Digit1 },
    { "2", KeyCode::Digit2 },
    { "3", KeyCode::Digit3 },
    { "4", KeyCode::Digit4 },
    { "5", KeyCode::Digit5 },
    { "6", KeyCode::Digit6 },
    { "7", KeyCode::Digit7 },
    { "8", KeyCode::Digit8 },
    { "9", KeyCode::Digit9 },
    { ";", KeyCode::Semicolon },
    { "=", KeyCode::Equal },
    { "A", KeyCode::A },
    { "B", KeyCode::B },
    { "C", KeyCode::C },
    { "D", KeyCode::D },
    { "E", KeyCode::E },
    { "F", KeyCode::F },
    { "G", KeyCode::G },
    { "H", KeyCode::H },
    { "I", KeyCode::I },
    { "J", KeyCode::J },
    { "K", KeyCode::K },
    { "L", KeyCode::L },
    { "M", KeyCode::M },
    { "N", KeyCode::N },
    { "O", KeyCode::O },
    { "P", KeyCode::P },
    { "Q", KeyCode::Q },
    { "R", KeyCode::R },
    { "S", KeyCode::S },
    { "T", KeyCode::T },
    { "U", KeyCode::U },
    { "V", KeyCode::V },
    { "W", KeyCode::W },
    { "X", KeyCode::X },
    { "Y", KeyCode::Y },
    { "Z", KeyCode::Z },
    { "[", KeyCode::LeftBracket },
    { "\\", KeyCode::Backslash },
    { "]", KeyCode::RightBracket },
    { "`", KeyCode::GraveAccent },
    { "", KeyCode::World1 },
    { "", KeyCode::World2 },
    { "Esc", KeyCode::Escape },
    { "Return", KeyCode::Enter },
    { "Enter", KeyCode::Enter },
    { "Tab", KeyCode::Tab },
    { "BkSp", KeyCode::Backspace },
    { "Ins", KeyCode::Insert },
    { "Del", KeyCode::Del },
    { "Right", KeyCode::Right },
    { "Left", KeyCode::Left },
    { "Down", KeyCode::Down },
    { "Up", KeyCode::Up },
    { "PgUp", KeyCode::PageUp },
    { "PgDn", KeyCode::PageDown },
    { "Home", KeyCode::Home },
    { "End", KeyCode::End },
    { "CapsLk", KeyCode::CapsLock },
    { "ScrLk", KeyCode::ScrollLock },
    { "NumLk", KeyCode::NumLock },
    { "PrnScr", KeyCode::PrintScreen },
    { "Pause", KeyCode::Pause },
    { "F1", KeyCode::F1 },
    { "F2", KeyCode::F2 },
    { "F3", KeyCode::F3 },
    { "F4", KeyCode::F4 },
    { "F5", KeyCode::F5 },
    { "F6", KeyCode::F6 },
    { "F7", KeyCode::F7 },
    { "F8", KeyCode::F8 },
    { "F9", KeyCode::F9 },
    { "F10", KeyCode::F10 },
    { "F11", KeyCode::F11 },
    { "F12", KeyCode::F12 },
    { "F13", KeyCode::F13 },
    { "F14", KeyCode::F14 },
    { "F15", KeyCode::F15 },
    { "F16", KeyCode::F16 },
    { "F17", KeyCode::F17 },
    { "F18", KeyCode::F18 },
    { "F19", KeyCode::F19 },
    { "F20", KeyCode::F20 },
    { "F21", KeyCode::F21 },
    { "F22", KeyCode::F22 },
    { "F23", KeyCode::F23 },
    { "F24", KeyCode::F24 },
    { "F25", KeyCode::F25 },
    { "Num 0", KeyCode::Num0 },
    { "Num 1", KeyCode::Num1 },
    { "Num 2", KeyCode::Num2 },
    { "Num 3", KeyCode::Num3 },
    { "Num 4", KeyCode::Num4 },
    { "Num 5", KeyCode::Num5 },
    { "Num 6", KeyCode::Num6 },
    { "Num 7", KeyCode::Num7 },
    { "Num 8", KeyCode::Num8 },
    { "Num 9", KeyCode::Num9 },
    { "Num .", KeyCode::NumDecimal },
    { "Num /", KeyCode::NumDivide },
    { "Num *", KeyCode::NumMultiply },
    { "Num -", KeyCode::NumSubtract },
    { "Num +", KeyCode::NumAdd },
    { "Num Return", KeyCode::NumEnter },
    { "Num =", KeyCode::NumEqual },
    { "Shift", KeyCode::LeftShift },
    { "Ctrl", KeyCode::LeftControl },
    { "Alt", KeyCode::LeftAlt },
    { "Super", KeyCode::LeftSuper },
    { "Shift", KeyCode::RightShift },
    { "Ctrl", KeyCode::RightControl },
    { "Alt", KeyCode::RightAlt },
    { "Super", KeyCode::RightSuper },
    { "Menu", KeyCode::Menu },
};

std::string keyModifiersToString(KeyModifiers mods, const std::string& joiner, bool finalJoiner) {
    std::string result;
#ifdef BRISK_APPLE
    if (mods && KeyModifiers::MacosControl)
        result += "Ctrl" + joiner;
    if (mods && KeyModifiers::MacosCommand)
        result += "Cmd" + joiner;
    if (mods && KeyModifiers::MacosOption)
        result += "Opt" + joiner;
    if (mods && KeyModifiers::Shift)
        result += "Shift" + joiner;
#else
    if (mods && KeyModifiers::WinWindows)
        result += "Win" + joiner;
    if (mods && KeyModifiers::WinControl)
        result += "Ctrl" + joiner;
    if (mods && KeyModifiers::Shift)
        result += "Shift" + joiner;
    if (mods && KeyModifiers::WinAlt)
        result += "Alt" + joiner;
#endif
    if (finalJoiner)
        return result;
    return result.substr(0, result.size() - joiner.size());
}

const NameValueOrderedList<MouseButton> mouseButtons{
    { "Left", MouseButton::Left }, { "Right", MouseButton::Right }, { "Middle", MouseButton::Middle },
    { "Btn4", MouseButton::Btn4 }, { "Btn5", MouseButton::Btn5 },   { "Btn6", MouseButton::Btn6 },
    { "Btn7", MouseButton::Btn7 }, { "Btn8", MouseButton::Btn8 },
};
} // namespace Brisk
