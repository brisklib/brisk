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
#include <utility>
#include <atomic>
#include <variant>
#include "Optional.hpp"

namespace Brisk {

struct GenerationInit {};

struct Generation {
    std::atomic<uint32_t> value{ 0 };

    Generation() noexcept = default;

    // copy constructor
    Generation(const Generation& c) noexcept : value(c.value.load(std::memory_order::relaxed)) {}

    Generation& operator=(const Generation& c) noexcept {
        value = c.value.load(std::memory_order::relaxed);
        return *this;
    }

    Generation(const GenerationInit&) noexcept : value{ UINT32_MAX } {}

    void operator++() noexcept {
        value.fetch_add(1, std::memory_order::release);
    }

    bool operator==(const Generation& g) const noexcept {
        return g.value == value;
    }

    bool operator!=(const Generation& g) const noexcept {
        return g.value != value;
    }
};

struct GenerationStored {
    uint32_t value{ UINT32_MAX };

    constexpr GenerationStored() noexcept = default;

    GenerationStored(const Generation& c) noexcept : value(c.value.load(std::memory_order::relaxed)) {}

    bool operator<<=(const Generation& g) noexcept {
        uint32_t gvalue = g.value.load(std::memory_order::relaxed);
        if (value != gvalue) {
            value = gvalue;
            return true;
        }
        return false;
    }
};

template <typename T, typename U>
inline bool assign(T& target, U&& newValue) {
    bool result = newValue != target;
    if (result)
        target = std::forward<U>(newValue);
    return result;
}

template <typename... Ts, typename U>
inline bool assign(std::variant<Ts...>& target, U&& newValue) {
    auto* val   = std::get_if<std::remove_cvref_t<U>>(&target);
    bool result = val == nullptr || newValue != *val;
    if (result)
        target = std::forward<U>(newValue);
    return result;
}

template <typename T, typename U>
inline bool assign(std::optional<T>& target, U&& newValue) {
    bool result = !target.has_value() || newValue != *target;
    if (result)
        target = std::forward<U>(newValue);
    return result;
}

template <typename T>
inline bool assign(std::optional<T>& target, std::nullopt_t) {
    bool result = target.has_value();
    if (result)
        target = std::nullopt;
    return result;
}

template <typename T, typename U>
inline bool assignAndIncrement(T& target, U&& newValue, Generation& generation) {
    if (target != newValue) {
        target = std::forward<U>(newValue);
        ++generation;
        return true;
    }
    return false;
}
} // namespace Brisk
