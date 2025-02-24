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
#include <array>
#include <cstdint>

namespace Brisk {

/**
 * @struct AppVersion
 * @brief Represents the version of an application.
 *
 * This struct holds information about an application's version, including major, minor, release, and patch
 * numbers, as well as an optional suffix. It provides a method to convert the version information into a
 * human-readable string.
 */
struct AppVersion {
    unsigned int major   = 0; ///< Major version number
    unsigned int minor   = 0; ///< Minor version number
    unsigned int release = 0; ///< Release number
    unsigned int patch   = 0; ///< Patch number
    std::string suffix;       ///< Optional suffix for the version

    /**
     * @brief Compares this version to another for equality.
     *
     * @param other The version to compare against.
     * @return `true` if the versions are equal, `false` otherwise.
     */
    constexpr bool operator==(const AppVersion&) const noexcept = default;

    /**
     * @brief Converts the version to a string representation.
     *
     * @return A string representing the version in the format "major.minor.release.patch[suffix]".
     */
    std::string string() const {
        return std::to_string(major) + "." + std::to_string(minor) + "." + std::to_string(release) + "." +
               std::to_string(patch) + suffix;
    }

    uint64_t linear(std::array<uint32_t, 3> factors = { 0x1'0000, 0x1'0000, 0x1'0000 }) const {
        uint64_t result = 0;
        result += major;
        result *= factors[0];
        result += minor;
        result *= factors[1];
        result += release;
        result *= factors[2];
        result += patch;
        return result;
    }

    static AppVersion fromLinear(uint64_t linear,
                                 std::array<uint32_t, 3> factors = { 0x1'0000, 0x1'0000, 0x1'0000 }) {
        AppVersion result;
        result.patch = linear % factors[2];
        linear /= factors[2];
        result.release = linear % factors[1];
        linear /= factors[1];
        result.minor = linear % factors[0];
        linear /= factors[0];
        result.major = linear;
        return result;
    }
};

/**
 * @struct AppMetadata
 * @brief Holds metadata for an application.
 *
 * This struct contains information about the application, such as its vendor, name, description, homepage
 * URL, copyright information, and version.
 */
struct AppMetadata {
    /**
     * @brief Vendor (manufacturer) name.
     */
    std::string vendor;

    /**
     * @brief Application name.
     */
    std::string name;

    /**
     * @brief Application description.
     */
    std::string description;

    /**
     * @brief URL of the application's homepage.
     */
    std::string homepage;

    /**
     * @brief Copyright information with the © symbol.
     */
    std::string copyright;

    /**
     * @brief Version information of the application.
     */
    AppVersion version;
};

/**
 * @brief Global variable holding application metadata.
 *
 * This variable is used to store metadata about the application, including version and other relevant
 * details.
 */
extern AppMetadata appMetadata;

} // namespace Brisk
