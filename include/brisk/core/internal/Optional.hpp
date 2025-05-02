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
#pragma once

#include <optional>
#include <fmt/format.h>

namespace Brisk {

template <typename T>
struct optional_ref : public std::optional<T*> {
    constexpr optional_ref(std::nullopt_t) noexcept : std::optional<T*>(std::nullopt) {}

    constexpr optional_ref(T& ref) noexcept : std::optional<T*>(&ref) {}

    constexpr T& operator*() const noexcept {
        return *std::optional<T*>::operator*();
    }

    constexpr T* operator->() const noexcept {
        return std::optional<T*>::operator*();
    }

    constexpr std::remove_cv_t<T> value() const {
        return static_cast<std::remove_cv_t<T>>(*std::optional<T*>::value());
    }

    template <typename U>
    constexpr std::remove_cv_t<T> value_or(U&& right) const {
        if (this->has_value())
            return static_cast<const std::remove_cv_t<T>&>(*std::optional<T*>::operator*());
        return static_cast<std::remove_cv_t<T>>(std::forward<U>(right));
    }
};

} // namespace Brisk

template <fmt::formattable T, typename Char>
struct fmt::formatter<std::optional<T>, Char> : fmt::formatter<T, Char> {
    template <typename FormatContext>
    auto format(const std::optional<T>& val, FormatContext& ctx) const {
        if (val)
            return formatter<T, Char>::format(*val, ctx);
        return fmt::format_to(ctx.out(), "(nullopt)");
    }
};
