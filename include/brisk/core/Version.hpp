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

#include "Brisk.h"

#define BRISK_VERSION_MAJOR 0
#define BRISK_VERSION_MINOR 9
#define BRISK_VERSION_PATCH 3
#define BRISK_VERSION_METADATA "-beta"

#define BRISK_VERSION                                                                                        \
    BRISK_STRINGIFY(BRISK_VERSION_MAJOR)                                                                     \
    "." BRISK_STRINGIFY(BRISK_VERSION_MINOR) "." BRISK_STRINGIFY(BRISK_VERSION_PATCH) BRISK_VERSION_METADATA

#include <string_view>

namespace Brisk {

/**
 * @brief A constant representing the version of the Brisk library.
 *
 * This constant holds the version of the Brisk library as a string view in the format
 * "major.minor.patch-metadata". For example, "0.9.1-alpha".
 */
extern const std::string_view version;

/**
 * @brief A constant containing detailed build information of the Brisk library.
 *
 * This string includes various pieces of build metadata, such as the operating system,
 * compiler version, standard library version, and specific flags used during the build.
 *
 * The buildInfo string is composed dynamically, depending on the system and compiler
 * used for building. It may contain details like:
 * - OS name
 * - Compiler version (Clang, MSVC, GCC)
 * - C++ standard library version
 * - Platform-specific versions (Windows, Android)
 * - Debugging and exception flags
 * - Date of build
 *
 * Example output:
 * ```
 * OS=Linux; Clang=10.0.0; GNUC=9.3.0; GLIBC=2.31; FLAGS=[D]; BUILT_ON=Sep 23 2024
 * ```
 */
extern const std::string_view buildInfo;

} // namespace Brisk
