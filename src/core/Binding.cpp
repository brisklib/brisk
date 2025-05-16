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
#include <brisk/core/Binding.hpp>
#include <brisk/core/internal/Lock.hpp>
#include <brisk/core/Utilities.hpp>
#include <brisk/core/Memory.hpp>

namespace Brisk {

AutoSingleton<Bindings> bindings;

int Bindings::addHandler(const RegionList& srcRegions, uint64_t id, Handler handler,
                         BindingAddresses srcAddresses, Region* destRegion, BindingAddress destAddress,
                         BindType type, std::string_view destDesc, std::string_view srcDesc,
                         Rc<Scheduler> srcQueue) {

    BRISK_ASSERT(srcRegions.size() == srcAddresses.size());

    if (srcAddresses.size() == 1) [[likely]] {
        // Handle single address case efficiently
        Region& region         = *srcRegions.front();
        BindingAddress address = srcAddresses.front();

        region.entriesChanged  = true;
        region.entries.insert(std::pair<BindingAddress, Entry>{
            address,
            Entry{
                .id          = id,
                .handler     = std::move(handler), // Use move semantics for handler
                .destRegion  = destRegion,
                .destAddress = destAddress,
                .type        = type,
                .destDesc    = destDesc,
                .srcDesc     = srcDesc,
                .srcQueue    = std::move(srcQueue),
            },
        });
    } else [[unlikely]] {
        // Handle multiple source addresses
        for (size_t i = 0; i < srcAddresses.size(); ++i) {
            Region& region         = *srcRegions[i];
            BindingAddress address = srcAddresses[i];
            region.entriesChanged  = true;
            region.entries.insert(std::pair<BindingAddress, Entry>{
                address,
                Entry{
                    .id          = id,
                    .handler     = handler, // Copy handler (no move)
                    .destRegion  = destRegion,
                    .destAddress = destAddress,
                    .type        = type,
                    .destDesc    = destDesc,
                    .srcDesc     = srcDesc,
                    .srcQueue    = srcQueue,
                },
            });
        }
    }
    return srcAddresses.size();
}

bool Bindings::isRegisteredRegion(BindingAddress range) const {
    return m_regions.contains(range.min());
}

void Bindings::registerRegion(BindingAddress range, Rc<Scheduler> queue) {
    std::lock_guard lk(m_mutex);
    if (!m_regions.insert_or_assign(range.min(), std::make_shared<Region>(range, std::move(queue))).second) {
        BRISK_ASSERT(false); // Assert if the region cannot be registered
    }
}

void Bindings::unregisterRegion(BindingAddress range) {
    unregisterRegion(range.min()); // Delegate to unregister by address start
}

void Bindings::unregisterRegion(const uint8_t* rangeBegin) {
    std::lock_guard lk(m_mutex);
    auto it = m_regions.find(rangeBegin);
    if (it == m_regions.end()) {
        BRISK_ASSERT(false); // Assert if the region is not found
    }
    removeIndirectDependencies(it->second.get());
    const BindingAddress range = it->second->region;
    m_regions.erase(it);
}

int Bindings::notifyRange(BindingAddress range) {
    int handlersCalled = 0;
    std::lock_guard lk(m_mutex);
    ++m_counter;

    Rc<Region> region = lookupRegion(range);
    BRISK_ASSERT_MSG("notifyRange: region is not registered", region);

    do {
        region->entriesChanged = false;

        auto first             = region->entries.begin();
        auto last              = region->entries.lower_bound(BindingAddress{ range.max(), 0 });

        for (auto it = first; it != last; ++it) {
            Entry& entry = it->second;
            if (!it->first.range().intersects(range.range())) {
                continue;
            }
            if (inStack(entry.id) || entry.counter == m_counter) {
                // Skip handlers that are already processed
                continue;
            }

            {
                m_stack.push_back(entry.id);
                SCOPE_EXIT {
                    m_stack.pop_back();
                };
                unlock_guard ulk(m_mutex);
                entry.handler(); // Execute the handler
            }
            entry.counter = m_counter;
            ++handlersCalled;

            if (region->entriesChanged) {
                // If m_entries have changed during a call to handler(), we can no longer
                // iterate through it with the current iterators.
                // Exit the loop and start from the beginning of m_entries.
                // Handlers that have already been called are skipped based on the counter value.
                break;
            }
        }
    } while (region->entriesChanged);

    return handlersCalled;
}

static bool addressesContain(const BindingAddresses& a, BindingAddress r) {
    return std::find(a.begin(), a.end(), r) != a.end();
}

Rc<Bindings::Region> Bindings::lookupRegion(BindingAddress address) {
    // Find the first region whose address is greater than the desired address
    auto it = m_regions.upper_bound(address.min());

    // Check if the region exists and contains the address
    if (it == m_regions.begin())
        return nullptr;
    --it;
    if (address.max() > it->second->region.max())
        return nullptr; // Address is outside the region
    return it->second;
}

Bindings::~Bindings() {
    unregisterRegion(staticBindingAddress); // Unregister the static region
}

Bindings::Bindings() {
    registerRegion(staticBindingAddress, nullptr); // Register the static region
}

void Bindings::enqueueInto(Rc<Scheduler> queue, VoidFunc fn, ExecuteImmediately mode) {
    if (queue) {
        queue->dispatch(std::move(fn), mode);
    } else {
        BRISK_SUPPRESS_EXCEPTIONS(fn()); // Execute function immediately if no queue
    }
}

size_t Bindings::numHandlers() const noexcept {
    std::lock_guard lk(m_mutex);
    size_t result = 0;
    for (const auto& [k, region] : m_regions) {
        result += region->entries.size();
    }
    return result;
}

size_t Bindings::numRegions() const noexcept {
    std::lock_guard lk(m_mutex);
    return m_regions.size() - 1; // Exclude implicit static region
}

bool Bindings::inStack(uint64_t id) {
    return std::find(m_stack.begin(), m_stack.end(), id) != m_stack.end();
}

void Bindings::removeConnection(uint64_t id) {
    for (const auto& [k, region] : m_regions) {
        region->disconnectIf([id](const std::pair<BindingAddress, Entry>& it) BRISK_INLINE_LAMBDA {
            return it.second.id == id;
        });
    }
}

void Bindings::removeIndirectDependencies(Region* regionToRemove) {
    for (const auto& [k, region] : m_regions) {
        region->disconnectIf([regionToRemove](const std::pair<BindingAddress, Entry>& it)
                                 BRISK_INLINE_LAMBDA {
                                     return it.second.destRegion == regionToRemove;
                                 });
    }
}

void Bindings::disconnect(BindingHandle handle) {
    std::lock_guard lk(m_mutex);
    for (const auto& [k, region] : m_regions) {
        region->disconnectIf([&](const std::pair<BindingAddress, Entry>& it) BRISK_INLINE_LAMBDA {
            return it.second.id == handle.m_id;
        });
    }
}

void Bindings::Region::disconnectIf(function_ref<bool(const std::pair<BindingAddress, Entry>&)> pred) {
    for (auto it = entries.begin(); it != entries.end();) {
        if (pred(*it)) [[unlikely]] {
            entriesChanged = true;
            it             = entries.erase(it); // Erase matching entries
        } else {
            ++it;
        }
    }
}

void Bindings::internalDisconnect(const BindingAddress& destAddress, const BindingAddresses& srcAddresses) {
    for (const auto& [k, region] : m_regions) {
        region->disconnectIf([&](const std::pair<BindingAddress, Entry>& it) BRISK_INLINE_LAMBDA {
            return addressesContain(srcAddresses, it.first) && it.second.destAddress == destAddress;
        });
    }
}

void Bindings::internalDisconnect(const BindingAddresses& addresses, BindDir dir) {
    if (dir == BindDir::Dest) {
        for (const auto& [k, region] : m_regions) {
            region->disconnectIf([&](const std::pair<BindingAddress, Entry>& it) BRISK_INLINE_LAMBDA {
                return addressesContain(addresses, it.second.destAddress);
            });
        }
    } else if (dir == BindDir::Src) {
        for (const auto& [k, region] : m_regions) {
            region->disconnectIf([&](const std::pair<BindingAddress, Entry>& it) BRISK_INLINE_LAMBDA {
                return addressesContain(addresses, it.first);
            });
        }
    } else {
        // Disconnect in both source and destination directions
        for (const auto& [k, region] : m_regions) {
            region->disconnectIf([&](const std::pair<BindingAddress, Entry>& it) BRISK_INLINE_LAMBDA {
                return addressesContain(addresses, it.first) ||
                       addressesContain(addresses, it.second.destAddress);
            });
        }
    }
}

} // namespace Brisk
