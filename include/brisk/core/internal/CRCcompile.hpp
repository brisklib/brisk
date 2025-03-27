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
#include <cstdint>

namespace Brisk {

namespace Internal {

inline constexpr const uint32_t crcTable[256] = {
#include "crctable.inc.hpp"
};

constexpr uint32_t crc32(const char* data, uint32_t len, uint32_t crc = 0) {
    crc = crc ^ 0xFFFFFFFFU;
    for (uint32_t i = 0; i < len; i++) {
        crc = Internal::crcTable[static_cast<uint8_t>(*data) ^ (crc & 0xFFu)] ^ (crc >> 8);
        data++;
    }
    crc = crc ^ 0xFFFFFFFFU;
    return crc;
}
} // namespace Internal

constexpr uint32_t crc32(const char* data, uint32_t len, uint32_t crc = 0) {
    return Internal::crc32(data, len, crc);
}

template <size_t N>
constexpr uint32_t crc32(const char (&str)[N], uint32_t crc = 0) {
    return Internal::crc32(str, sizeof(str) - 1, crc);
}
} // namespace Brisk
