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
#include <brisk/core/Version.hpp>

#if defined __has_include
#if __has_include(<features.h>)
#include <features.h>
#endif
#if __has_include(<android/api-level.h>)
#include <android/api-level.h>
#endif
#if __has_include(<ntverp.h>)
#include <ntverp.h>
#endif
#endif

namespace Brisk {

const std::string_view version = BRISK_VERSION;

const std::string_view buildInfo =
    "OS=" BRISK_OS_NAME
#ifdef __clang_version__
    "; Clang=" __clang_version__
#endif
#ifdef _MSC_FULL_VER
    "; MSVC=" BRISK_STRINGIFY(_MSC_FULL_VER) "-" BRISK_STRINGIFY(_MSVC_LANG)
#endif
#ifdef WINVER
        "; WINVER=" BRISK_STRINGIFY(WINVER)
#endif
#ifdef VER_PRODUCTVERSION_STR
            "; VER_PRODUCTVERSION_STR=" VER_PRODUCTVERSION_STR
#endif
#ifdef __GNUC__
            "; GNUC=" BRISK_STRINGIFY(__GNUC__) "." BRISK_STRINGIFY(__GNUC_MINOR__) "." BRISK_STRINGIFY(
                __GNUC_PATCHLEVEL__)
#endif
#ifdef __GLIBC__
                "; GLIBC=" BRISK_STRINGIFY(__GLIBC__) "." BRISK_STRINGIFY(__GLIBC_MINOR__)
#endif
#ifdef __GLIBCXX__
                    "; GLIBCXX=" BRISK_STRINGIFY(__GLIBCXX__)
#endif
#ifdef _LIBCPP_VERSION
                        "; LIBCPP_VERSION=" BRISK_STRINGIFY(_LIBCPP_VERSION)
#endif
#ifdef __ANDROID_API__
                            "; ANDROID_API=" BRISK_STRINGIFY(__ANDROID_API__)
#endif
                                "; FLAGS=["
#ifdef BRISK_EXCEPTIONS
                                "E"
#endif
#ifdef BRISK_RTTI
                                "R"
#endif
#ifdef BRISK_DEBUG
                                "D"
#endif
                                "]"
#ifdef __DATE__
                                "; BUILT_ON=" __DATE__
#endif
    ;
} // namespace Brisk
