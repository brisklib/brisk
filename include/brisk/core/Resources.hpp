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
#include <brisk/core/Exceptions.hpp>

namespace Brisk {

namespace Internal {
extern "C" const struct ResourceEntry resourceEntries[];
extern "C" const uint32_t resourceEntriesSize;

inline std::span<const ResourceEntry> allResources() {
    return {
        resourceEntries,
        resourceEntries + resourceEntriesSize,
    };
}

inline const ResourceEntry* lookupResource(std::string_view name) {
    auto all = allResources();
    auto it  = std::lower_bound(all.begin(), all.end(), name);
    if (it != all.end() && it->name == name)
        return &*it;
    return nullptr;
}

} // namespace Internal

class EResources : public Exception<std::runtime_error> {
public:
    using Exception<std::runtime_error>::Exception;
};

/**
 * @class Resources
 * @brief Provides utility functions for managing resource files.
 *
 * This class allows checking for resource existence, enumerating available resources,
 * and loading resources as raw bytes or text, with optional caching.
 */
class Resources {
public:
    /**
     * @brief Checks if a resource exists.
     * @param name The name of the resource to check.
     * @return True if the resource exists, false otherwise.
     */
    static bool exists(std::string_view name) {
        const Internal::ResourceEntry* rsrc = Internal::lookupResource(name);
        return rsrc && *rsrc->size > 0;
    }

    /**
     * @brief Retrieves a list of all available resource names.
     * @return A vector of resource names.
     */
    static std::vector<std::string_view> enumerate() {
        std::vector<std::string_view> list;
        list.reserve(Internal::allResources().size());
        for (const auto& r : Internal::allResources()) {
            list.push_back(r.name);
        }
        return list;
    }

    /**
     * @brief Loads a resource as raw bytes.
     * @param name The name of the resource to load.
     * @param emptyOk If true, allows returning an empty byte vector if the resource is missing.
     * @return A byte vector containing the resource data.
     *
     * @throws EResources if the resource does not exist and emptyOk is false.
     */
    static Bytes load(std::string_view name, bool emptyOk = false) {
        const Internal::ResourceEntry* rsrc = Internal::lookupResource(name);
        if ((!rsrc || *rsrc->size == 0) && emptyOk) {
            return {};
        }
        if (!rsrc)
            throwException(EResources("Resource '{}' not found", name));
        if (rsrc->compression == Internal::ResourceCompression::None) {
            return Bytes((const std::byte*)rsrc->data, (const std::byte*)rsrc->data + *rsrc->size);
        }
        return compressionDecode(
            static_cast<CompressionMethod>(rsrc->compression),
            BytesView((const std::byte*)rsrc->data, (const std::byte*)rsrc->data + *rsrc->size));
    }

    /**
     * @brief Loads a resource as raw bytes with caching.
     * @param name The name of the resource to load.
     * @param emptyOk If true, allows returning an empty byte vector if the resource is missing.
     * @return A reference to a cached byte vector containing the resource data.
     *
     * @throws EResources if the resource does not exist and emptyOk is false.
     */
    static const Bytes& loadCached(std::string name, bool emptyOk = false) {
        static std::map<std::string, Bytes> cache;
        auto data = load(name, emptyOk);
        auto it   = cache.insert_or_assign(std::move(name), std::move(data));
        return it.first->second;
    }

    /**
     * @brief Loads a resource as a text string.
     * @param name The name of the resource to load.
     * @param emptyOk If true, allows returning an empty string if the resource is missing.
     * @return A string containing the resource data.
     *
     * @throws EResources if the resource does not exist and emptyOk is false.
     */
    static std::string loadText(std::string_view name, bool emptyOk = false) {
        const Internal::ResourceEntry* rsrc = Internal::lookupResource(name);
        if ((!rsrc || *rsrc->size == 0) && emptyOk) {
            return {};
        }
        if (!rsrc)
            throwException(EResources("Resource '{}' not found", name));
        if (rsrc->compression == Internal::ResourceCompression::None) {
            return std::string((const char*)rsrc->data, (const char*)rsrc->data + *rsrc->size);
        }
        auto bytes = compressionDecode(
            static_cast<CompressionMethod>(rsrc->compression),
            BytesView((const std::byte*)rsrc->data, (const std::byte*)rsrc->data + *rsrc->size));
        return std::string((const char*)bytes.data(), (const char*)bytes.data() + bytes.size());
    }

    /**
     * @brief Loads a resource as a text string with caching.
     * @param name The name of the resource to load.
     * @param emptyOk If true, allows returning an empty string if the resource is missing.
     * @return A reference to a cached string containing the resource data.
     *
     * @throws EResources if the resource does not exist and emptyOk is false.
     */
    static const std::string& loadTextCached(std::string name, bool emptyOk = false) {
        static std::map<std::string, std::string> cache;
        auto data = loadText(name, emptyOk);
        auto it   = cache.insert_or_assign(std::move(name), std::move(data));
        return it.first->second;
    }
};

} // namespace Brisk
