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

#include <map>
#include <brisk/core/BasicTypes.hpp>
#include <brisk/core/Compression.hpp>
#include <brisk/core/internal/Resources.h>
#include <brisk/core/internal/Debug.hpp>

namespace Brisk {

namespace Internal {
extern "C" const struct ResourceEntry resourceEntries[];
extern "C" const uint32_t resourceEntriesSize;

inline const ResourceEntry* lookupResource(std::string_view name) {
    auto it = std::lower_bound(resourceEntries, resourceEntries + resourceEntriesSize, name);
    if (it != resourceEntries + resourceEntriesSize && it->name == name)
        return &*it;
    return nullptr;
}

} // namespace Internal

inline bool resourceExists(std::string_view name) {
    const Internal::ResourceEntry* rsrc = Internal::lookupResource(name);
    return rsrc && *rsrc->size > 0;
}

inline std::vector<uint8_t> loadResource(std::string_view name, bool emptyOk = false) {
    const Internal::ResourceEntry* rsrc = Internal::lookupResource(name);
    if ((!rsrc || *rsrc->size == 0) && emptyOk) {
        return {};
    }
    BRISK_ASSERT(rsrc);
    if (rsrc->compression == Internal::ResourceCompression::None) {
        return std::vector<uint8_t>(rsrc->data, rsrc->data + *rsrc->size);
    }
    return compressionDecode(static_cast<CompressionMethod>(rsrc->compression),
                             std::span<const uint8_t>(rsrc->data, rsrc->data + *rsrc->size));
}

inline const std::vector<uint8_t>& loadResourceCached(std::string name, bool emptyOk = false) {
    static std::map<std::string, std::vector<uint8_t>> cache;
    auto data = loadResource(name, emptyOk);
    auto it   = cache.insert_or_assign(std::move(name), std::move(data));
    return it.first->second;
}

inline std::string loadResourceText(std::string_view name, bool emptyOk = false) {
    const Internal::ResourceEntry* rsrc = Internal::lookupResource(name);
    if ((!rsrc || *rsrc->size == 0) && emptyOk) {
        return {};
    }
    BRISK_ASSERT(rsrc);
    if (rsrc->compression == Internal::ResourceCompression::None) {
        return std::string(rsrc->data, rsrc->data + *rsrc->size);
    }
    auto bytes = compressionDecode(static_cast<CompressionMethod>(rsrc->compression),
                                   std::span<const uint8_t>(rsrc->data, rsrc->data + *rsrc->size));
    return std::string(bytes.data(), bytes.data() + bytes.size());
}

inline const std::string& loadResourceTextCached(std::string name, bool emptyOk = false) {
    static std::map<std::string, std::string> cache;
    auto data = loadResourceText(name, emptyOk);
    auto it   = cache.insert_or_assign(std::move(name), std::move(data));
    return it.first->second;
}

} // namespace Brisk
