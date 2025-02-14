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
#include <brisk/core/Hash.hpp>
#include "cityhash/city.h"

namespace Brisk {

static const uint32_t crctable[256] = {
#include <brisk/core/internal/crctable.inc.hpp>
};

uint32_t crc32(BytesView data, uint32_t crc) {
    crc = crc ^ 0xFFFFFFFFU;
    for (size_t i = 0; i < data.size(); i++) {
        crc = crctable[static_cast<uint8_t>(data[i]) ^ (crc & 0xFFu)] ^ (crc >> 8);
    }
    crc = crc ^ 0xFFFFFFFFU;
    return crc;
}

uint64_t fastHash(BytesView data, uint64_t seed) {
    return CityHash64WithSeed(reinterpret_cast<const char*>(data.data()), data.size(), seed);
}

} // namespace Brisk
