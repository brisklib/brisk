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
#include "KeyCodes.hpp"

#include "windows.h"

namespace Brisk {

consteval std::array<KeyCode, numScanCodes> genScanCodeToKeyCode() {
    std::array<KeyCode, numScanCodes> result{};
    result[0x00B] = KeyCode::Digit0;
    result[0x002] = KeyCode::Digit1;
    result[0x003] = KeyCode::Digit2;
    result[0x004] = KeyCode::Digit3;
    result[0x005] = KeyCode::Digit4;
    result[0x006] = KeyCode::Digit5;
    result[0x007] = KeyCode::Digit6;
    result[0x008] = KeyCode::Digit7;
    result[0x009] = KeyCode::Digit8;
    result[0x00A] = KeyCode::Digit9;
    result[0x01E] = KeyCode::A;
    result[0x030] = KeyCode::B;
    result[0x02E] = KeyCode::C;
    result[0x020] = KeyCode::D;
    result[0x012] = KeyCode::E;
    result[0x021] = KeyCode::F;
    result[0x022] = KeyCode::G;
    result[0x023] = KeyCode::H;
    result[0x017] = KeyCode::I;
    result[0x024] = KeyCode::J;
    result[0x025] = KeyCode::K;
    result[0x026] = KeyCode::L;
    result[0x032] = KeyCode::M;
    result[0x031] = KeyCode::N;
    result[0x018] = KeyCode::O;
    result[0x019] = KeyCode::P;
    result[0x010] = KeyCode::Q;
    result[0x013] = KeyCode::R;
    result[0x01F] = KeyCode::S;
    result[0x014] = KeyCode::T;
    result[0x016] = KeyCode::U;
    result[0x02F] = KeyCode::V;
    result[0x011] = KeyCode::W;
    result[0x02D] = KeyCode::X;
    result[0x015] = KeyCode::Y;
    result[0x02C] = KeyCode::Z;
    result[0x028] = KeyCode::Apostrophe;
    result[0x02B] = KeyCode::Backslash;
    result[0x033] = KeyCode::Comma;
    result[0x00D] = KeyCode::Equal;
    result[0x029] = KeyCode::GraveAccent;
    result[0x01A] = KeyCode::LeftBracket;
    result[0x00C] = KeyCode::Minus;
    result[0x034] = KeyCode::Period;
    result[0x01B] = KeyCode::RightBracket;
    result[0x027] = KeyCode::Semicolon;
    result[0x035] = KeyCode::Slash;
    result[0x056] = KeyCode::World2;
    result[0x00E] = KeyCode::Backspace;
    result[0x153] = KeyCode::Del;
    result[0x14F] = KeyCode::End;
    result[0x01C] = KeyCode::Enter;
    result[0x001] = KeyCode::Escape;
    result[0x147] = KeyCode::Home;
    result[0x152] = KeyCode::Insert;
    result[0x15D] = KeyCode::Menu;
    result[0x151] = KeyCode::PageDown;
    result[0x149] = KeyCode::PageUp;
    result[0x045] = KeyCode::Pause;
    result[0x039] = KeyCode::Space;
    result[0x00F] = KeyCode::Tab;
    result[0x03A] = KeyCode::CapsLock;
    result[0x145] = KeyCode::NumLock;
    result[0x046] = KeyCode::ScrollLock;
    result[0x03B] = KeyCode::F1;
    result[0x03C] = KeyCode::F2;
    result[0x03D] = KeyCode::F3;
    result[0x03E] = KeyCode::F4;
    result[0x03F] = KeyCode::F5;
    result[0x040] = KeyCode::F6;
    result[0x041] = KeyCode::F7;
    result[0x042] = KeyCode::F8;
    result[0x043] = KeyCode::F9;
    result[0x044] = KeyCode::F10;
    result[0x057] = KeyCode::F11;
    result[0x058] = KeyCode::F12;
    result[0x064] = KeyCode::F13;
    result[0x065] = KeyCode::F14;
    result[0x066] = KeyCode::F15;
    result[0x067] = KeyCode::F16;
    result[0x068] = KeyCode::F17;
    result[0x069] = KeyCode::F18;
    result[0x06A] = KeyCode::F19;
    result[0x06B] = KeyCode::F20;
    result[0x06C] = KeyCode::F21;
    result[0x06D] = KeyCode::F22;
    result[0x06E] = KeyCode::F23;
    result[0x076] = KeyCode::F24;
    result[0x038] = KeyCode::LeftAlt;
    result[0x01D] = KeyCode::LeftControl;
    result[0x02A] = KeyCode::LeftShift;
    result[0x15B] = KeyCode::LeftSuper;
    result[0x137] = KeyCode::PrintScreen;
    result[0x138] = KeyCode::RightAlt;
    result[0x11D] = KeyCode::RightControl;
    result[0x036] = KeyCode::RightShift;
    result[0x15C] = KeyCode::RightSuper;
    result[0x150] = KeyCode::Down;
    result[0x14B] = KeyCode::Left;
    result[0x14D] = KeyCode::Right;
    result[0x148] = KeyCode::Up;
    result[0x052] = KeyCode::KP0;
    result[0x04F] = KeyCode::KP1;
    result[0x050] = KeyCode::KP2;
    result[0x051] = KeyCode::KP3;
    result[0x04B] = KeyCode::KP4;
    result[0x04C] = KeyCode::KP5;
    result[0x04D] = KeyCode::KP6;
    result[0x047] = KeyCode::KP7;
    result[0x048] = KeyCode::KP8;
    result[0x049] = KeyCode::KP9;
    result[0x04E] = KeyCode::KPAdd;
    result[0x053] = KeyCode::KPDecimal;
    result[0x135] = KeyCode::KPDivide;
    result[0x11C] = KeyCode::KPEnter;
    result[0x059] = KeyCode::KPEqual;
    result[0x037] = KeyCode::KPMultiply;
    result[0x04A] = KeyCode::KPSubtract;
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
    if (keyCode < KeyCode(0) || // NOLINT(clang-analyzer-optin.core.EnumCastOutOfRange)
        keyCode > KeyCode::Last)
        return -1;
    return keyCodeToScanCodeTable[+keyCode];
}

} // namespace Brisk
