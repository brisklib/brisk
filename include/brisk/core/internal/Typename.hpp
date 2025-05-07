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
 * If you do not wish to be bound by the GPL-2.0+ license, you must purchase a commercial
 * license. For commercial licensing options, please visit: https://brisklib.com
 *
 * You should have received a copy of the GNU General Public License along with this program.
 * If not, see <http://www.gnu.org/licenses/>.
 *
 * For commercial licensing, please visit: https://brisklib.com/
 */
#pragma once

#include <cstdlib>
#include "FixedString.hpp"
#include "../internal/Constants.hpp"

#include "../Brisk.h"

namespace Brisk {

namespace Internal {

#ifdef BRISK_MSVC
#define BRISK__CALL_CONV_SPEC __cdecl
#else
#define BRISK__CALL_CONV_SPEC
#endif

#ifdef BRISK_MSVC
#define BRISK__FUNC_SIGNATURE __FUNCSIG__
#else
#define BRISK__FUNC_SIGNATURE __PRETTY_FUNCTION__
#endif

#if defined BRISK_CLANG
constexpr size_t typename_prefix   = sizeof("auto Brisk::Internal::typeNameFixed() [T = ") - 1;
constexpr size_t typename_postfix  = sizeof("]") - 1;
constexpr size_t valuename_prefix  = sizeof("auto Brisk::Internal::valueNameFixed() [V = ") - 1;
constexpr size_t valuename_postfix = sizeof("]") - 1;
#elif defined BRISK_MSVC
constexpr size_t typename_prefix   = sizeof("auto __cdecl Brisk::Internal::typeNameFixed<") - 1;
constexpr size_t typename_postfix  = sizeof(">(void) noexcept") - 1;
constexpr size_t valuename_prefix  = sizeof("auto __cdecl Brisk::Internal::valueNameFixed<") - 1;
constexpr size_t valuename_postfix = sizeof(">(void) noexcept") - 1;
#else // GCC
constexpr size_t typename_prefix  = sizeof("constexpr auto Brisk::Internal::typeNameFixed() [with T = ") - 1;
constexpr size_t typename_postfix = sizeof("]") - 1;
constexpr size_t valuename_prefix =
    sizeof("constexpr auto Brisk::Internal::valueNameFixed() [with auto V = ") - 1;
constexpr size_t valuename_postfix = sizeof("]") - 1;
#endif

template <typename T>
constexpr auto BRISK__CALL_CONV_SPEC typeNameFixed() noexcept {
    constexpr size_t length = sizeof(BRISK__FUNC_SIGNATURE) - 1 > typename_prefix + typename_postfix
                                  ? sizeof(BRISK__FUNC_SIGNATURE) - 1 - typename_prefix - typename_postfix
                                  : 0;
    return FixedString<length>{ BRISK__FUNC_SIGNATURE + (length > 0 ? typename_prefix : 0), 0 };
}

template <auto V>
constexpr auto BRISK__CALL_CONV_SPEC valueNameFixed() noexcept {
    constexpr size_t length = sizeof(BRISK__FUNC_SIGNATURE) - 1 > valuename_prefix + valuename_postfix
                                  ? sizeof(BRISK__FUNC_SIGNATURE) - 1 - valuename_prefix - valuename_postfix
                                  : 0;
    return FixedString<length>{ BRISK__FUNC_SIGNATURE + (length > 0 ? valuename_prefix : 0), 0 };
}

template <typename T>
struct TypenameHelper {
    constexpr static auto name = typeNameFixed<T>();
};

template <auto V>
struct ValuenameHelper {
    constexpr static auto name = valueNameFixed<V>();
};

template <typename T>
constexpr std::string_view BRISK__CALL_CONV_SPEC typeName() noexcept {
    return TypenameHelper<T>::name.string();
}

template <auto V>
constexpr std::string_view BRISK__CALL_CONV_SPEC valueName() noexcept {
    return ValuenameHelper<V>::name.string();
}

} // namespace Internal
} // namespace Brisk
