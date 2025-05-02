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

#include "../Brisk.h"
#include <brisk/core/internal/FixedString.hpp>
#include <brisk/core/internal/Debug.hpp>
#include <utility>
#include <cstdint>
#include <variant>

namespace Brisk {

enum class ArgumentOp : uint8_t {
    Assignment,
    // Addition,
    // Subtraction,
    // Multiply,
    // Division,
    // Remainder,
    // Or,
    // And,
    // Xor,
    ShiftLeft,
    ShiftRight,
};

template <typename Target>
struct ArgumentsView;

template <typename Target>
void applier(Target* target, ArgumentsView<Target> arg);

template <typename Arg, typename Target>
concept Applicable = requires(const Arg& arg, Target* target) {
    { applier(target, arg) } -> std::same_as<void>;
};

template <typename Target>
struct ArgumentsView {
public:
    constexpr ArgumentsView(const ArgumentsView&) noexcept            = default;
    constexpr ArgumentsView(ArgumentsView&&) noexcept                 = default;
    constexpr ArgumentsView& operator=(const ArgumentsView&) noexcept = default;
    constexpr ArgumentsView& operator=(ArgumentsView&&) noexcept      = default;

    constexpr ArgumentsView(std::nullptr_t) noexcept : args(nullptr), applyFn(nullptr) {}

    template <typename... Args>
    ArgumentsView(const std::tuple<Args...>& args) : args(&args) {
        static_assert((Applicable<Args, Target> && ...));

        applyFn = [](const void* args, Target* target) BRISK_INLINE_LAMBDA {
            applyTuple(*reinterpret_cast<const std::tuple<Args...>*>(args), target,
                       std::index_sequence_for<Args...>{});
        };
    }

    BRISK_INLINE void apply(Target* target) const {
        BRISK_ASSERT(target);
        if (applyFn)
            applyFn(args, target);
    }

private:
    const void* args;
    using ApplyFn = void (*)(const void*, Target*);
    ApplyFn applyFn;

    template <typename... Args, size_t... Indices>
    BRISK_INLINE static void applyTuple(const std::tuple<Args...>& args, Target* target,
                                        std::index_sequence<Indices...>) {
        (applier(target, std::get<Indices>(args)), ...);
    }
};

template <typename Target>
BRISK_INLINE void applier(Target* target, ArgumentsView<Target> arg) {
    arg.apply(target);
}

template <typename Tag, typename Type = typename Tag::Type, ArgumentOp op = ArgumentOp::Assignment>
struct ArgVal {
    using ValueType = Type;
    ValueType value;

    template <typename U>
    constexpr operator ArgVal<Tag, U, op>() const
        requires std::convertible_to<Type, U>
    {
        return { static_cast<U>(value) };
    }
};

template <typename T, typename Tag>
concept MatchesExtraTypes = requires(T val) { Tag::ExtraTypes::accept(val); };

template <typename Tag>
struct Argument : Tag {
    using ValueType = typename Tag::Type;

    constexpr ArgVal<Tag> operator=(ValueType value) const {
        return { std::move(value) };
    }

    template <MatchesExtraTypes<Tag> U>
    constexpr ArgVal<Tag, U> operator=(U value) const {
        return { std::move(value) };
    }
};

namespace Tag {
template <Internal::FixedString Name, typename Type_ = void>
struct Named {
    using Type = Type_;
};
} // namespace Tag

template <Internal::FixedString S>
struct Argument<Tag::Named<S>> {
    using ValueType = typename Tag::Named<S>::Type; // void

    template <typename U>
    constexpr ArgVal<Tag::Named<S>, U> operator=(U value) const {
        return { std::move(value) };
    }
};

} // namespace Brisk
