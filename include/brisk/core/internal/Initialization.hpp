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

#include "../BasicTypes.hpp"

namespace Brisk {

enum class InitializationFlags {
    Threading = 1 << 0,
    Settings  = 1 << 1,

    Default   = Threading | Settings,
};

template <>
constexpr inline bool isBitFlags<InitializationFlags> = true;

void initializeCommon(InitializationFlags flags = InitializationFlags::Default);
void finalizeCommon();

struct CommonInitializer {
    CommonInitializer(InitializationFlags flags = InitializationFlags::Default) {
        initializeCommon(flags);
    }

    ~CommonInitializer() {
        finalizeCommon();
    }
};

#ifdef BRISK_WINDOWS
/**
 * @brief Initializes the application on Windows with the specified module handle and command line.
 * @param moduleHandle Handle to the application module (HINSTANCE).
 * @param cmdLine Command line arguments as a wide-character string.
 */
void startup(void* moduleHandle, wchar_t* cmdLine);
#endif

/**
 * @brief Initializes the application with command line arguments.
 * @param argc Number of command line arguments.
 * @param argv Array of command line argument strings.
 */
void startup(int argc, char** argv);

/**
 * @brief Finalizes the application, releasing resources.
 */
void shutdown();
} // namespace Brisk
