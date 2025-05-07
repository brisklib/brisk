/*
 * Brisk
 *
 * Cross-platform application framework
 * --------------------------------------------------------------
 *
 * Copyright (C) 2025 Brisk Developers
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

/**
 * @enum KeyCode
 * @brief Keyboard key codes.
 *
 * Represents various keyboard keys with their corresponding integer values, used for processing keyboard
 * input events.
 */
enum class KeyCode : int32_t {
    Unknown      = -1,  ///< Unknown key
    Space        = 32,  ///< Space key
    Apostrophe   = 39,  ///< Apostrophe key (')
    Comma        = 44,  ///< Comma key (,)
    Minus        = 45,  ///< Minus key (-)
    Period       = 46,  ///< Period key (.)
    Slash        = 47,  ///< Slash key (/)
    Digit0       = 48,  ///< Digit 0 key
    Digit1       = 49,  ///< Digit 1 key
    Digit2       = 50,  ///< Digit 2 key
    Digit3       = 51,  ///< Digit 3 key
    Digit4       = 52,  ///< Digit 4 key
    Digit5       = 53,  ///< Digit 5 key
    Digit6       = 54,  ///< Digit 6 key
    Digit7       = 55,  ///< Digit 7 key
    Digit8       = 56,  ///< Digit 8 key
    Digit9       = 57,  ///< Digit 9 key
    Semicolon    = 59,  ///< Semicolon key (;)
    Equal        = 61,  ///< Equal key (=)
    A            = 65,  ///< A key
    B            = 66,  ///< B key
    C            = 67,  ///< C key
    D            = 68,  ///< D key
    E            = 69,  ///< E key
    F            = 70,  ///< F key
    G            = 71,  ///< G key
    H            = 72,  ///< H key
    I            = 73,  ///< I key
    J            = 74,  ///< J key
    K            = 75,  ///< K key
    L            = 76,  ///< L key
    M            = 77,  ///< M key
    N            = 78,  ///< N key
    O            = 79,  ///< O key
    P            = 80,  ///< P key
    Q            = 81,  ///< Q key
    R            = 82,  ///< R key
    S            = 83,  ///< S key
    T            = 84,  ///< T key
    U            = 85,  ///< U key
    V            = 86,  ///< V key
    W            = 87,  ///< W key
    X            = 88,  ///< X key
    Y            = 89,  ///< Y key
    Z            = 90,  ///< Z key
    LeftBracket  = 91,  ///< Left bracket key ([)
    Backslash    = 92,  ///< Backslash key (\)
    RightBracket = 93,  ///< Right bracket key (])
    GraveAccent  = 96,  ///< Grave accent key (`)
    World1       = 161, ///< Non-US keyboard key 1
    World2       = 162, ///< Non-US keyboard key 2
    Escape       = 256, ///< Escape key
    Enter        = 257, ///< Enter key
    Tab          = 258, ///< Tab key
    Backspace    = 259, ///< Backspace key
    Insert       = 260, ///< Insert key
    Del          = 261, ///< Delete key
    Right        = 262, ///< Right arrow key
    Left         = 263, ///< Left arrow key
    Down         = 264, ///< Down arrow key
    Up           = 265, ///< Up arrow key
    PageUp       = 266, ///< Page Up key
    PageDown     = 267, ///< Page Down key
    Home         = 268, ///< Home key
    End          = 269, ///< End key
    CapsLock     = 280, ///< Caps Lock key
    ScrollLock   = 281, ///< Scroll Lock key
    NumLock      = 282, ///< Num Lock key
    PrintScreen  = 283, ///< Print Screen key
    Pause        = 284, ///< Pause key
    F1           = 290, ///< F1 function key
    F2           = 291, ///< F2 function key
    F3           = 292, ///< F3 function key
    F4           = 293, ///< F4 function key
    F5           = 294, ///< F5 function key
    F6           = 295, ///< F6 function key
    F7           = 296, ///< F7 function key
    F8           = 297, ///< F8 function key
    F9           = 298, ///< F9 function key
    F10          = 299, ///< F10 function key
    F11          = 300, ///< F11 function key
    F12          = 301, ///< F12 function key
    F13          = 302, ///< F13 function key
    F14          = 303, ///< F14 function key
    F15          = 304, ///< F15 function key
    F16          = 305, ///< F16 function key
    F17          = 306, ///< F17 function key
    F18          = 307, ///< F18 function key
    F19          = 308, ///< F19 function key
    F20          = 309, ///< F20 function key
    F21          = 310, ///< F21 function key
    F22          = 311, ///< F22 function key
    F23          = 312, ///< F23 function key
    F24          = 313, ///< F24 function key
    F25          = 314, ///< F25 function key
    Num0         = 320, ///< Numpad 0 key
    Num1         = 321, ///< Numpad 1 key
    Num2         = 322, ///< Numpad 2 key
    Num3         = 323, ///< Numpad 3 key
    Num4         = 324, ///< Numpad 4 key
    Num5         = 325, ///< Numpad 5 key
    Num6         = 326, ///< Numpad 6 key
    Num7         = 327, ///< Numpad 7 key
    Num8         = 328, ///< Numpad 8 key
    Num9         = 329, ///< Numpad 9 key
    NumDecimal   = 330, ///< Numpad decimal key
    NumDivide    = 331, ///< Numpad divide key
    NumMultiply  = 332, ///< Numpad multiply key
    NumSubtract  = 333, ///< Numpad subtract key
    NumAdd       = 334, ///< Numpad add key
    NumEnter     = 335, ///< Numpad enter key
    NumEqual     = 336, ///< Numpad equal key
    LeftShift    = 340, ///< Left Shift key
    LeftControl  = 341, ///< Left Control key
    LeftAlt      = 342, ///< Left Alt key
    LeftSuper    = 343, ///< Left Super (Windows/Command) key
    RightShift   = 344, ///< Right Shift key
    RightControl = 345, ///< Right Control key
    RightAlt     = 346, ///< Right Alt key
    RightSuper   = 347, ///< Right Super (Windows/Command) key
    Menu         = 348, ///< Menu key
    Last         = 348, ///< Last valid key code
};

/**
 * @brief Converts KeyCode to its underlying integer type.
 * @param code The KeyCode to convert.
 * @return The underlying integer value of the KeyCode.
 */
constexpr std::underlying_type_t<KeyCode> operator+(KeyCode code) {
    return static_cast<std::underlying_type_t<KeyCode>>(code);
}

/**
 * @brief Number of key codes defined.
 */
constexpr inline size_t numKeyCodes  = +KeyCode::Last + 1;

/**
 * @brief Number of scan codes supported.
 */
constexpr inline size_t numScanCodes = 512;

/**
 * @brief Ordered list mapping KeyCode values to their names.
 */
extern const NameValueOrderedList<KeyCode> keyCodes;

/**
 * @brief Converts a KeyCode to its string representation.
 * @param code The KeyCode to convert.
 * @return The string name of the KeyCode, or an empty string if not found.
 */
inline std::string keyCodeToString(KeyCode code) {
    return valueToKey(keyCodes, code).value_or("");
}

/**
 * @brief Formats a KeyCode as a string.
 * @param code The KeyCode to format.
 * @return The string representation of the KeyCode.
 */
inline std::string format_as(KeyCode code) {
    return keyCodeToString(code);
}

/**
 * @brief Converts a string to a KeyCode.
 * @param str The string to convert.
 * @return The corresponding KeyCode, or std::nullopt if not found.
 */
inline std::optional<KeyCode> stringToKeyCode(const std::string& str) {
    return keyToValue(keyCodes, str);
}

/**
 * @enum KeyModifiers
 * @brief Enumerates modifier keys for keyboard shortcuts.
 *
 * Represents modifier keys (e.g., Shift, Control) as bit flags for use in keyboard shortcuts.
 */
enum class KeyModifiers {
    None         = 0x00,                          ///< No modifiers
    All          = 0x2F,                          ///< All modifiers
    Shift        = 0x01,                          ///< Shift key
    Control      = 0x02,                          ///< Control key
    Alt          = 0x04,                          ///< Alt key
    Super        = 0x08,                          ///< Super (Windows/Command) key
    CapsLock     = 0x10,                          ///< Caps Lock key
    NumLock      = 0x20,                          ///< Num Lock key
    Regular      = Shift | Control | Alt | Super, ///< Regular modifier combination
    MacosOption  = Alt,                           ///< macOS Option key (Alt)
    MacosControl = Control,                       ///< macOS Control key
    MacosCommand = Super,                         ///< macOS Command key (Super)
    WinAlt       = Alt,                           ///< Windows Alt key
    WinControl   = Control,                       ///< Windows Control key
    WinWindows   = Super,                         ///< Windows key (Super)
#ifdef BRISK_APPLE
    ControlOrCommand = MacosCommand, ///< Control or Command key (macOS)
#else
    ControlOrCommand = WinControl, ///< Control or Command key (Windows)
#endif
};

template <>
constexpr inline bool isBitFlags<KeyModifiers> = true;

/**
 * @brief Converts KeyModifiers to a string representation.
 * @param mods The KeyModifiers to convert.
 * @param joiner The string used to join modifier names (default: "+").
 * @param finalJoiner If true, appends the joiner at the end (default: true).
 * @return The string representation of the modifiers.
 */
std::string keyModifiersToString(KeyModifiers mods, const std::string& joiner = "+", bool finalJoiner = true);

/**
 * @struct Shortcut
 * @brief Represents a keyboard shortcut combining a key and modifiers.
 */
struct Shortcut {
    KeyModifiers modifiers                          = KeyModifiers::None; ///< Modifier keys
    KeyCode key                                     = KeyCode::Unknown;   ///< Primary key

    /**
     * @brief Compares two shortcuts for equality.
     * @param other The other Shortcut to compare with.
     * @return True if the shortcuts are equal, false otherwise.
     */
    bool operator==(const Shortcut&) const noexcept = default;
};

/**
 * @brief Formats a Shortcut as a string.
 * @param shortcut The Shortcut to format.
 * @return The string representation of the shortcut (modifiers + key).
 */
inline std::string format_as(Shortcut shortcut) {
    return keyModifiersToString(shortcut.modifiers) + fmt::to_string(shortcut.key);
}

/**
 * @enum KeyAction
 * @brief Enumerates possible keyboard key actions.
 */
enum class KeyAction {
    Release = 0, ///< Key released
    Press   = 1, ///< Key pressed
    Repeat  = 2, ///< Key held and repeating
};

/**
 * @enum MouseAction
 * @brief Enumerates possible mouse button actions.
 */
enum class MouseAction {
    Release = 0, ///< Mouse button released
    Press   = 1, ///< Mouse button pressed
};

/**
 * @enum MouseButton
 * @brief Enumerates mouse buttons.
 */
enum class MouseButton {
    Btn1   = 0, ///< Mouse button 1
    Btn2   = 1, ///< Mouse button 2
    Btn3   = 2, ///< Mouse button 3
    Btn4   = 3, ///< Mouse button 4
    Btn5   = 4, ///< Mouse button 5
    Btn6   = 5, ///< Mouse button 6
    Btn7   = 6, ///< Mouse button 7
    Btn8   = 7, ///< Mouse button 8
    Last   = 7, ///< Last valid mouse button
    Left   = 0, ///< Left mouse button (alias for Btn1)
    Right  = 1, ///< Right mouse button (alias for Btn2)
    Middle = 2, ///< Middle mouse button (alias for Btn3)
};

/**
 * @brief Converts MouseButton to its underlying integer type.
 * @param code The MouseButton to convert.
 * @return The underlying integer value of the MouseButton.
 */
constexpr std::underlying_type_t<MouseButton> operator+(MouseButton code) {
    return static_cast<std::underlying_type_t<MouseButton>>(code);
}

/**
 * @brief Number of mouse buttons defined.
 */
constexpr inline size_t numMouseButtons = +MouseButton::Last + 1;

/**
 * @brief Ordered list mapping MouseButton values to their names.
 */
extern const NameValueOrderedList<MouseButton> mouseButtons;

/**
 * @brief Converts a MouseButton to its string representation.
 * @param btn The MouseButton to convert.
 * @return The string name of the MouseButton, or an empty string if not found.
 */
inline std::string mouseButtonToString(MouseButton btn) {
    return valueToKey(mouseButtons, btn).value_or("");
}

/**
 * @brief Converts a string to a MouseButton.
 * @param str The string to convert.
 * @return The corresponding MouseButton, or std::nullopt if not found.
 */
inline std::optional<MouseButton> stringToMouseButton(const std::string& str) {
    return keyToValue(mouseButtons, str);
}

/**
 * @enum Cursor
 * @brief Enumerates cursor types for mouse pointer display.
 */
enum class Cursor : uint32_t {
    NotSet = 0,              ///< No cursor set
    Grab,                    ///< Grab cursor
    GrabDeny,                ///< Grab deny cursor
    GrabReady,               ///< Grab ready cursor
    Arrow      = 0x80000001, ///< Arrow cursor
    IBeam      = 0x80000002, ///< I-beam (text) cursor
    Crosshair  = 0x80000003, ///< Crosshair cursor
    Hand       = 0x80000004, ///< Hand cursor
    HResize    = 0x80000005, ///< Horizontal resize cursor
    VResize    = 0x80000006, ///< Vertical resize cursor
    NSResize   = 0x80000007, ///< North-South resize cursor
    EWResize   = 0x80000008, ///< East-West resize cursor
    NESWResize = 0x80000009, ///< Northeast-Southwest resize cursor
    NWSEResize = 0x8000000A, ///< Northwest-Southeast resize cursor
    AllResize  = 0x8000000B, ///< All directions resize cursor
    NotAllowed = 0x8000000C, ///< Not allowed cursor
};

/**
 * @enum CloseAction
 * @brief Enumerates actions to take when a window close is requested.
 */
enum class CloseAction {
    Nothing, ///< No action
    Hide,    ///< Hide the window
    Close,   ///< Close the window
};

/**
 * @enum WindowStyle
 * @brief Enumerates window style flags.
 */
enum class WindowStyle : int32_t {
    None        = 0,          ///< No style
    Undecorated = 1 << 0,     ///< Window without decorations
    Resizable   = 1 << 1,     ///< Resizable window
    TopMost     = 1 << 2,     ///< Always-on-top window
    ToolWindow  = 1 << 3,     ///< Tool window
    Disabled    = 1 << 5,     ///< Disabled window
    Normal      = Resizable,  ///< Normal resizable window
    Dialog      = ToolWindow, ///< Dialog-style window
};

template <>
constexpr inline bool isBitFlags<WindowStyle> = true;

/**
 * @enum WindowState
 * @brief Enumerates window states.
 */
enum class WindowState : int32_t {
    Normal    = 0, ///< Normal window state
    Maximized = 1, ///< Maximized window state
    Minimized = 2, ///< Minimized window state
};

template <>
constexpr inline bool isBitFlags<WindowState> = true;

/**
 * @enum DragEvent
 * @brief Enumerates drag-and-drop event types.
 */
enum class DragEvent : int32_t {
    None     = -1, ///< No drag event
    Started  = 0,  ///< Drag started
    Dragging = 1,  ///< Dragging in progress
    Dropped  = 2,  ///< Drag dropped
};

/**
 * @struct SvgCursor
 * @brief Represents a custom SVG-based cursor.
 */
struct SvgCursor {
    std::string svg;       ///< SVG content for the cursor
    Point hotspot{ 0, 0 }; ///< Hotspot position for the cursor

    /**
     * @brief Nominal size for the SVG cursor.
     */
    constexpr static Size size{ 24, 24 };
};

} // namespace Brisk
