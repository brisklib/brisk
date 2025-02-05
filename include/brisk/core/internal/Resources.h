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

#include "incbin.h"

#ifdef __cplusplus

#include <cstdint>
#include <string_view>

namespace Brisk {

namespace Internal {

// Must match CompressionMethod enum
enum class ResourceCompression : uint32_t {
    None = 0, ///< No compression.
    GZip = 1, ///< GZip compression.
    ZLib = 2, ///< ZLib compression.
    LZ4  = 3, ///< LZ4 compression.
#ifdef BRISK_HAVE_BROTLI
    Brotli = 4, ///< Brotli compression (enabled by BRISK_BROTLI cmake option).
#endif
};

struct ResourceEntry {
    const char* name;
    const uint8_t* data;
    const unsigned int* size;
    ResourceCompression compression;

    friend bool operator<(std::string_view name, const ResourceEntry& entry) noexcept {
        return name < std::string_view(entry.name);
    }

    friend bool operator<(const ResourceEntry& entry, std::string_view name) noexcept {
        return std::string_view(entry.name) < name;
    }
};

} // namespace Internal
} // namespace Brisk

#else

#include <stdint.h>

enum ResourceCompression {
    ResourceCompression_None = 0, ///< No compression.
    ResourceCompression_GZip = 1, ///< GZip compression.
    ResourceCompression_ZLib = 2, ///< ZLib compression.
    ResourceCompression_LZ4  = 3, ///< LZ4 compression.
#ifdef BRISK_HAVE_BROTLI
    ResourceCompression_Brotli = 4, ///< Brotli compression (enabled by BRISK_BROTLI cmake option).
#endif
};

struct ResourceEntry {
    const char* name;
    const uint8_t* data;
    const unsigned int* size;
    unsigned int compression;
};

#endif
