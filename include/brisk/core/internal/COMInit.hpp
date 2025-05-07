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

#include "../Brisk.h"
#include <cstdint>

namespace Brisk {

#if defined BRISK_WINDOWS
/**
 * @brief A RAII class that initializes and uninitializes COM library.
 *
 * This class is used to ensure that the COM library is properly initialized
 * when working on Windows. The constructor initializes COM, and the destructor
 * uninitializes it.
 */
struct COMInitializer {
    /**
     * @brief Constructs a COMInitializer object and initializes the COM library.
     *
     * If initialization fails, the result will contain the error code.
     */
    COMInitializer();

    /**
     * @brief Destroys the COMInitializer object and uninitializes the COM library.
     */
    ~COMInitializer();

    /// The result of the COM initialization.
    uint32_t result;

    /**
     * @brief Converts the COMInitializer to a boolean indicating success of initialization.
     *
     * @return true if COM was successfully initialized; false otherwise.
     */
    explicit operator bool() const noexcept;
};
#else
/**
 * @brief A no-op COMInitializer for non-Windows platforms.
 *
 * This struct is defined to provide a consistent interface across platforms,
 * but does not perform any actions on non-Windows systems.
 */
struct [[maybe_unused]] COMInitializer {};
#endif

} // namespace Brisk
