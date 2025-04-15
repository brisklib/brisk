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
#pragma once

#include <brisk/core/Brisk.h>
#include <brisk/graphics/Geometry.hpp>
#include <brisk/core/BasicTypes.hpp>
#include <brisk/core/Utilities.hpp>
#include <brisk/core/internal/Optional.hpp>

namespace Brisk {

enum class KeyCode : int32_t {
    Unknown      = -1,
    Space        = 32,
    Apostrophe   = 39,
    Comma        = 44,
    Minus        = 45,
    Period       = 46,
    Slash        = 47,
    Digit0       = 48,
    Digit1       = 49,
    Digit2       = 50,
    Digit3       = 51,
    Digit4       = 52,
    Digit5       = 53,
    Digit6       = 54,
    Digit7       = 55,
    Digit8       = 56,
    Digit9       = 57,
    Semicolon    = 59,
    Equal        = 61,
    A            = 65,
    B            = 66,
    C            = 67,
    D            = 68,
    E            = 69,
    F            = 70,
    G            = 71,
    H            = 72,
    I            = 73,
    J            = 74,
    K            = 75,
    L            = 76,
    M            = 77,
    N            = 78,
    O            = 79,
    P            = 80,
    Q            = 81,
    R            = 82,
    S            = 83,
    T            = 84,
    U            = 85,
    V            = 86,
    W            = 87,
    X            = 88,
    Y            = 89,
    Z            = 90,
    LeftBracket  = 91,
    Backslash    = 92,
    RightBracket = 93,
    GraveAccent  = 96,
    World1       = 161,
    World2       = 162,
    Escape       = 256,
    Enter        = 257,
    Tab          = 258,
    Backspace    = 259,
    Insert       = 260,
    Del          = 261,
    Right        = 262,
    Left         = 263,
    Down         = 264,
    Up           = 265,
    PageUp       = 266,
    PageDown     = 267,
    Home         = 268,
    End          = 269,
    CapsLock     = 280,
    ScrollLock   = 281,
    NumLock      = 282,
    PrintScreen  = 283,
    Pause        = 284,
    F1           = 290,
    F2           = 291,
    F3           = 292,
    F4           = 293,
    F5           = 294,
    F6           = 295,
    F7           = 296,
    F8           = 297,
    F9           = 298,
    F10          = 299,
    F11          = 300,
    F12          = 301,
    F13          = 302,
    F14          = 303,
    F15          = 304,
    F16          = 305,
    F17          = 306,
    F18          = 307,
    F19          = 308,
    F20          = 309,
    F21          = 310,
    F22          = 311,
    F23          = 312,
    F24          = 313,
    F25          = 314,
    KP0          = 320,
    KP1          = 321,
    KP2          = 322,
    KP3          = 323,
    KP4          = 324,
    KP5          = 325,
    KP6          = 326,
    KP7          = 327,
    KP8          = 328,
    KP9          = 329,
    KPDecimal    = 330,
    KPDivide     = 331,
    KPMultiply   = 332,
    KPSubtract   = 333,
    KPAdd        = 334,
    KPEnter      = 335,
    KPEqual      = 336,
    LeftShift    = 340,
    LeftControl  = 341,
    LeftAlt      = 342,
    LeftSuper    = 343,
    RightShift   = 344,
    RightControl = 345,
    RightAlt     = 346,
    RightSuper   = 347,
    Menu         = 348,
    Last         = 348,
};

constexpr std::underlying_type_t<KeyCode> operator+(KeyCode code) {
    return static_cast<std::underlying_type_t<KeyCode>>(code);
}

constexpr inline size_t numKeyCodes  = +KeyCode::Last + 1;
constexpr inline size_t numScanCodes = 512;

extern const NameValueOrderedList<KeyCode> keyCodes;

inline std::string keyCodeToString(KeyCode code) {
    return valueToKey(keyCodes, code).value_or("");
}

inline std::optional<KeyCode> stringToKeyCode(const std::string& str) {
    return keyToValue(keyCodes, str);
}

enum class KeyModifiers {
    None         = 0x00,
    All          = 0x2F,
    Shift        = 0x01,
    Control      = 0x02,
    Alt          = 0x04,
    Super        = 0x08,
    CapsLock     = 0x10,
    NumLock      = 0x20,

    Regular      = Shift | Control | Alt | Super,

    MacosOption  = Alt,
    MacosControl = Control,
    MacosCommand = Super,

    WinAlt       = Alt,
    WinControl   = Control,
    WinWindows   = Super,

#ifdef BRISK_APPLE
    ControlOrCommand = MacosCommand,
#else
    ControlOrCommand = WinControl,
#endif
};
template <>
constexpr inline bool isBitFlags<KeyModifiers> = true;

std::string keyModifiersToString(KeyModifiers mods, const std::string& joiner = "+", bool finalJoiner = true);

enum class KeyAction {
    Release = 0,
    Press   = 1,
    Repeat  = 2,
};

enum class MouseAction {
    Release = 0,
    Press   = 1,
};

enum class MouseButton {
    Btn1   = 0,
    Btn2   = 1,
    Btn3   = 2,
    Btn4   = 3,
    Btn5   = 4,
    Btn6   = 5,
    Btn7   = 6,
    Btn8   = 7,
    Last   = 7,
    Left   = 0,
    Right  = 1,
    Middle = 2,
};

constexpr std::underlying_type_t<MouseButton> operator+(MouseButton code) {
    return static_cast<std::underlying_type_t<MouseButton>>(code);
}

constexpr inline size_t numMouseButtons = +MouseButton::Last + 1;

extern const NameValueOrderedList<MouseButton> mouseButtons;

inline std::string mouseButtonToString(MouseButton btn) {
    return valueToKey(mouseButtons, btn).value_or("");
}

inline std::optional<MouseButton> stringToMouseButton(const std::string& str) {
    return keyToValue(mouseButtons, str);
}

enum class Cursor : uint32_t {
    NotSet = 0,
    Grab,
    GrabDeny,
    GrabReady,

    // System cursors
    Arrow      = 0x80000001,
    IBeam      = 0x80000002,
    Crosshair  = 0x80000003,
    Hand       = 0x80000004,
    HResize    = 0x80000005,
    VResize    = 0x80000006,
    NSResize   = 0x80000007,
    EWResize   = 0x80000008,
    NESWResize = 0x80000009,
    NWSEResize = 0x8000000A,
    AllResize  = 0x8000000B,
    NotAllowed = 0x8000000C,
};

enum class CloseAction {
    Nothing,
    Hide,
    Close,
};

enum class WindowStyle : int32_t {
    None        = 0,
    Undecorated = 1 << 0,
    Resizable   = 1 << 1,
    TopMost     = 1 << 2,
    ToolWindow  = 1 << 3,
    Disabled    = 1 << 5,
    Normal      = Resizable,
    Dialog      = ToolWindow,
};
template <>
constexpr inline bool isBitFlags<WindowStyle> = true;

enum class WindowState : int32_t {
    Normal    = 0,
    Maximized = 1,
    Minimized = 2,
};
template <>
constexpr inline bool isBitFlags<WindowState> = true;

enum class DragEvent : int32_t {
    None     = -1,
    Started  = 0,
    Dragging = 1,
    Dropped  = 2,
};

inline std::string hotKeyToString(KeyCode key, KeyModifiers mods) {
    return keyModifiersToString(mods) + keyCodeToString(key);
}

struct SVGCursor {
    std::string svg;
    Point hotspot{ 0, 0 };

    constexpr static Size size{ 24, 24 }; // nominal size for cursor svg
};

} // namespace Brisk
