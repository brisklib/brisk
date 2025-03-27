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

consteval std::array<KeyCode, numScanCodes> genScanCodeToKeyCode() {
    std::array<KeyCode, numScanCodes> result{};

    result[0x1D] = KeyCode::Digit0;
    result[0x12] = KeyCode::Digit1;
    result[0x13] = KeyCode::Digit2;
    result[0x14] = KeyCode::Digit3;
    result[0x15] = KeyCode::Digit4;
    result[0x17] = KeyCode::Digit5;
    result[0x16] = KeyCode::Digit6;
    result[0x1A] = KeyCode::Digit7;
    result[0x1C] = KeyCode::Digit8;
    result[0x19] = KeyCode::Digit9;
    result[0x00] = KeyCode::A;
    result[0x0B] = KeyCode::B;
    result[0x08] = KeyCode::C;
    result[0x02] = KeyCode::D;
    result[0x0E] = KeyCode::E;
    result[0x03] = KeyCode::F;
    result[0x05] = KeyCode::G;
    result[0x04] = KeyCode::H;
    result[0x22] = KeyCode::I;
    result[0x26] = KeyCode::J;
    result[0x28] = KeyCode::K;
    result[0x25] = KeyCode::L;
    result[0x2E] = KeyCode::M;
    result[0x2D] = KeyCode::N;
    result[0x1F] = KeyCode::O;
    result[0x23] = KeyCode::P;
    result[0x0C] = KeyCode::Q;
    result[0x0F] = KeyCode::R;
    result[0x01] = KeyCode::S;
    result[0x11] = KeyCode::T;
    result[0x20] = KeyCode::U;
    result[0x09] = KeyCode::V;
    result[0x0D] = KeyCode::W;
    result[0x07] = KeyCode::X;
    result[0x10] = KeyCode::Y;
    result[0x06] = KeyCode::Z;
    result[0x27] = KeyCode::Apostrophe;
    result[0x2A] = KeyCode::Backslash;
    result[0x2B] = KeyCode::Comma;
    result[0x18] = KeyCode::Equal;
    result[0x32] = KeyCode::GraveAccent;
    result[0x21] = KeyCode::LeftBracket;
    result[0x1B] = KeyCode::Minus;
    result[0x2F] = KeyCode::Period;
    result[0x1E] = KeyCode::RightBracket;
    result[0x29] = KeyCode::Semicolon;
    result[0x2C] = KeyCode::Slash;
    result[0x0A] = KeyCode::World1;
    result[0x33] = KeyCode::Backspace;
    result[0x39] = KeyCode::CapsLock;
    result[0x75] = KeyCode::Del;
    result[0x7D] = KeyCode::Down;
    result[0x77] = KeyCode::End;
    result[0x24] = KeyCode::Enter;
    result[0x35] = KeyCode::Escape;
    result[0x7A] = KeyCode::F1;
    result[0x78] = KeyCode::F2;
    result[0x63] = KeyCode::F3;
    result[0x76] = KeyCode::F4;
    result[0x60] = KeyCode::F5;
    result[0x61] = KeyCode::F6;
    result[0x62] = KeyCode::F7;
    result[0x64] = KeyCode::F8;
    result[0x65] = KeyCode::F9;
    result[0x6D] = KeyCode::F10;
    result[0x67] = KeyCode::F11;
    result[0x6F] = KeyCode::F12;
    result[0x69] = KeyCode::PrintScreen;
    result[0x6B] = KeyCode::F14;
    result[0x71] = KeyCode::F15;
    result[0x6A] = KeyCode::F16;
    result[0x40] = KeyCode::F17;
    result[0x4F] = KeyCode::F18;
    result[0x50] = KeyCode::F19;
    result[0x5A] = KeyCode::F20;
    result[0x73] = KeyCode::Home;
    result[0x72] = KeyCode::Insert;
    result[0x7B] = KeyCode::Left;
    result[0x3A] = KeyCode::LeftAlt;
    result[0x3B] = KeyCode::LeftControl;
    result[0x38] = KeyCode::LeftShift;
    result[0x37] = KeyCode::LeftSuper;
    result[0x6E] = KeyCode::Menu;
    result[0x47] = KeyCode::NumLock;
    result[0x79] = KeyCode::PageDown;
    result[0x74] = KeyCode::PageUp;
    result[0x7C] = KeyCode::Right;
    result[0x3D] = KeyCode::RightAlt;
    result[0x3E] = KeyCode::RightControl;
    result[0x3C] = KeyCode::RightShift;
    result[0x36] = KeyCode::RightSuper;
    result[0x31] = KeyCode::Space;
    result[0x30] = KeyCode::Tab;
    result[0x7E] = KeyCode::Up;
    result[0x52] = KeyCode::KP0;
    result[0x53] = KeyCode::KP1;
    result[0x54] = KeyCode::KP2;
    result[0x55] = KeyCode::KP3;
    result[0x56] = KeyCode::KP4;
    result[0x57] = KeyCode::KP5;
    result[0x58] = KeyCode::KP6;
    result[0x59] = KeyCode::KP7;
    result[0x5B] = KeyCode::KP8;
    result[0x5C] = KeyCode::KP9;
    result[0x45] = KeyCode::KPAdd;
    result[0x41] = KeyCode::KPDecimal;
    result[0x4B] = KeyCode::KPDivide;
    result[0x4C] = KeyCode::KPEnter;
    result[0x51] = KeyCode::KPEqual;
    result[0x43] = KeyCode::KPMultiply;
    result[0x4E] = KeyCode::KPSubtract;
    return result;
}

constexpr std::array<KeyCode, numScanCodes> scanCodeToKeyCodeTable = genScanCodeToKeyCode();

consteval std::array<int16_t, numKeyCodes> genKeyCodeToScanCode() {
    std::array<int16_t, numKeyCodes> result{};
    for (size_t i = 0; i < numScanCodes; ++i) {
        result[static_cast<int16_t>(scanCodeToKeyCodeTable[i])] = i;
    }
    return result;
}

constexpr std::array<int16_t, numKeyCodes> keyCodeToScanCodeTable = genKeyCodeToScanCode();

KeyCode scanCodeToKeyCode(int scanCode) {
    if (scanCode < 0 || scanCode >= std::size(scanCodeToKeyCodeTable))
        return KeyCode::Unknown;
    return scanCodeToKeyCodeTable[scanCode];
}

int keyCodeToScanCode(KeyCode keyCode) {
    if (keyCode < KeyCode(0) || keyCode > KeyCode::Last)
        return -1;
    return keyCodeToScanCodeTable[+keyCode];
}

} // namespace Brisk
