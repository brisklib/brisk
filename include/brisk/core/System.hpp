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

#include <string>
#include <cstdint>
#include <brisk/core/Brisk.h>
#include <brisk/core/Reflection.hpp>

namespace Brisk {

/**
 * @brief Structure representing the operating system version.
 */
struct OSVersion {
    uint16_t major = 0; ///< Major version of the operating system.
    uint16_t minor = 0; ///< Minor version of the operating system.
    uint32_t build = 0; ///< Build number of the operating system.

    /**
     * @brief Default comparison operator.
     */
    constexpr auto operator<=>(const OSVersion&) const noexcept = default;

    /**
     * @brief Reflection data for the fields of OSVersion.
     */
    constexpr static std::tuple reflection{
        ReflectionField{ "major", &OSVersion::major },
        ReflectionField{ "minor", &OSVersion::minor },
        ReflectionField{ "build", &OSVersion::build },
    };
};

/**
 * @brief Structure representing OS uname information.
 */
struct OSUname {
    std::string sysname; ///< System name.
    std::string release; ///< Release name.
    std::string version; ///< Version of the system.
    std::string machine; ///< Machine hardware name.

    /**
     * @brief Default comparison operator.
     */
    constexpr bool operator==(const OSUname&) const noexcept = default;

    /**
     * @brief Reflection data for the fields of OSUname.
     */
    constexpr static std::tuple reflection{
        ReflectionField{ "sysname", &OSUname::sysname },
        ReflectionField{ "release", &OSUname::release },
        ReflectionField{ "version", &OSUname::version },
        ReflectionField{ "machine", &OSUname::machine },
    };
};

/**
 * @brief Retrieves the operating system version.
 * @return The OSVersion structure containing the operating system version.
 */
OSVersion osVersion();

/**
 * @brief Retrieves the operating system name.
 * @return The name of the operating system as a string.
 */
std::string osName();

/**
 * @brief Retrieves the system uname information.
 * @return The OSUname structure containing the uname information.
 */
OSUname osUname();

/**
 * @brief Structure representing CPU information.
 */
struct CpuInfo {
    std::string model; ///< CPU model name.
    int speed;         ///< CPU speed in MHz.

    /**
     * @brief Reflection data for the fields of CpuInfo.
     */
    constexpr static std::tuple reflection{
        ReflectionField{ "model", &CpuInfo::model },
        ReflectionField{ "speed", &CpuInfo::speed },
    };
};

/**
 * @brief Retrieves the CPU information.
 * @return The CpuInfo structure containing CPU model and speed.
 */
CpuInfo cpuInfo();

struct MemoryInfo {
    uint64_t maxrss;  /* maximum resident set size, kilobytes */
    uint64_t majflt;  /* page faults (hard page faults) */
    uint64_t inblock; /* block input operations */
    uint64_t oublock; /* block output operations */
};

MemoryInfo memoryInfo();

#ifdef BRISK_WINDOWS

/**
 * @brief Enum class representing various Windows 10 versions.
 */
enum class Windows10Version : uint32_t {
    _1507              = 10240, ///< Threshold release.
    _1511              = 10586, ///< Threshold 2 - November Update.
    _1607              = 14393, ///< Redstone - Anniversary Update.
    _1703              = 15063, ///< Redstone 2 - Creators Update.
    _1709              = 16299, ///< Redstone 3 - Fall Creators Update.
    _1803              = 17134, ///< Redstone 4 - April 2018 Update.
    _1809              = 17763, ///< Redstone 5 - October 2018 Update.
    _1903              = 18362, ///< 19H1 - May 2019 Update.
    _1909              = 18363, ///< 19H2 - November 2019 Update.
    _2004              = 19041, ///< 20H1 - May 2020 Update.
    _20H2              = 19042, ///< 20H2 - October 2020 Update.
    _21H1              = 19043, ///< 21H1 - May 2021 Update.
    _21H2              = 19044, ///< 21H2 - November 2021 Update.
    _22H2              = 19045, ///< 22H2 - 2022 Update.
    _19H1              = _1903,
    _19H2              = _1909,
    _20H1              = _2004,

    NovemberUpdate     = _1511, ///< Alias for the November Update.
    AnniversaryUpdate  = _1607, ///< Alias for the Anniversary Update.
    CreatorsUpdate     = _1703, ///< Alias for the Creators Update.
    FallCreatorsUpdate = _1709, ///< Alias for the Fall Creators Update.
};

/**
 * @brief Checks if the current OS is Windows.
 * @return True if the OS is Windows, false otherwise.
 */
constexpr bool isOSWindows() {
    return true;
}

/**
 * @brief Checks if the current OS version is Windows and matches the specified version.
 * @param major Major version number.
 * @param minor Minor version number.
 * @param build Build number.
 * @return True if the current OS version matches, false otherwise.
 */
inline bool isOSWindows(uint16_t major, uint16_t minor = 0, uint32_t build = 0) {
    return osVersion() >= OSVersion{ major, minor, build };
}

/**
 * @brief Checks if the current OS is Windows 10 and at least the specified version.
 * @param minVersion The minimum Windows 10 version to check against.
 * @return True if the current OS version is at least the specified Windows 10 version.
 */
inline bool isOSWindows10(Windows10Version minVersion = Windows10Version::_1507) {
    return osVersion() >= OSVersion{ 10, 0, static_cast<uint32_t>(minVersion) };
}

#else

/**
 * @brief Checks if the current OS is Windows.
 * @return Always false for non-Windows systems.
 */
constexpr bool isOSWindows() {
    return false;
}

/**
 * @brief Dummy function for non-Windows systems.
 * @param major Major version number (unused).
 * @param minor Minor version number (unused).
 * @param build Build number (unused).
 * @return Always false for non-Windows systems.
 */
constexpr bool isOSWindows(unsigned int major, unsigned int minor, unsigned int build) {
    return false;
}
#endif

} // namespace Brisk
