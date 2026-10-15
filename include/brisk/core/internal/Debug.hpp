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
 * If you do not wish to be bound by the GPL-2.0+ license, you must purchase a commercial
 * license. For commercial licensing options, please visit: https://brisklib.com
 *
 * You should have received a copy of the GNU General Public License along with this program.
 * If not, see <http://www.gnu.org/licenses/>.
 *
 * For commercial licensing, please visit: https://brisklib.com/
 */
#pragma once

#include <exception>
#include <optional>
#include <string>
#include <string_view>

#if defined BRISK_MSVC
#include <intrin.h>
#endif

#include <fmt/format.h>

#include <brisk/core/Brisk.h>
#include <brisk/core/internal/Throw.hpp>

#ifdef BRISK_DEBUG
#if defined BRISK_MSVC
#define BRISK_BREAKPOINT __debugbreak()
#elif defined BRISK_X86
#define BRISK_BREAKPOINT __asm__ __volatile__("int $0x03")
#else
#define BRISK_BREAKPOINT __builtin_trap()
#endif
#endif

#ifndef BRISK_BREAKPOINT
#define BRISK_BREAKPOINT ((void)0)
#endif

namespace Brisk {

namespace Internal {

template <typename FunctionType, typename LeftType, typename RightType>
struct Comparison {
    static constexpr std::string_view op = FunctionType::op;

    const LeftType& left;
    const RightType& right;

    Comparison(const LeftType& left, const RightType& right) : left(left), right(right) {}

    constexpr bool operator()() const {
        return FunctionType{}(left, right);
    }
};

struct CmpEq {
    static constexpr std::string_view op = "==";

    template <typename LeftType, typename RightType>
    constexpr bool operator()(const LeftType& left, const RightType& right) const
        noexcept(noexcept(left == right)) {
        return left == right;
    }
};

struct CmpNe {
    static constexpr std::string_view op = "!=";

    template <typename LeftType, typename RightType>
    constexpr bool operator()(const LeftType& left, const RightType& right) const
        noexcept(noexcept(left != right)) {
        return left != right;
    }
};

struct CmpLt {
    static constexpr std::string_view op = "<";

    template <typename LeftType, typename RightType>
    constexpr bool operator()(const LeftType& left, const RightType& right) const
        noexcept(noexcept(left < right)) {
        return left < right;
    }
};

struct CmpGt {
    static constexpr std::string_view op = ">";

    template <typename LeftType, typename RightType>
    constexpr bool operator()(const LeftType& left, const RightType& right) const
        noexcept(noexcept(left > right)) {
        return left > right;
    }
};

struct CmpLe {
    static constexpr std::string_view op = "<=";

    template <typename LeftType, typename RightType>
    constexpr bool operator()(const LeftType& left, const RightType& right) const
        noexcept(noexcept(left <= right)) {
        return left <= right;
    }
};

struct CmpGe {
    static constexpr std::string_view op = ">=";

    template <typename LeftType, typename RightType>
    constexpr bool operator()(const LeftType& left, const RightType& right) const
        noexcept(noexcept(left >= right)) {
        return left >= right;
    }
};

template <typename LeftType>
struct HalfComparison {
    HalfComparison(const LeftType& left) : left(left) {}

    constexpr bool operator()() const {
        return static_cast<bool>(left);
    }

    template <typename RightType>
    Comparison<CmpEq, LeftType, RightType> operator==(const RightType& right) {
        return Comparison<CmpEq, LeftType, RightType>(left, right);
    }

    template <typename RightType>
    Comparison<CmpNe, LeftType, RightType> operator!=(const RightType& right) {
        return Comparison<CmpNe, LeftType, RightType>(left, right);
    }

    template <typename RightType>
    Comparison<CmpLt, LeftType, RightType> operator<(const RightType& right) {
        return Comparison<CmpLt, LeftType, RightType>(left, right);
    }

    template <typename RightType>
    Comparison<CmpGt, LeftType, RightType> operator>(const RightType& right) {
        return Comparison<CmpGt, LeftType, RightType>(left, right);
    }

    template <typename RightType>
    Comparison<CmpLe, LeftType, RightType> operator<=(const RightType& right) {
        return Comparison<CmpLe, LeftType, RightType>(left, right);
    }

    template <typename RightType>
    Comparison<CmpGe, LeftType, RightType> operator>=(const RightType& right) {
        return Comparison<CmpGe, LeftType, RightType>(left, right);
    }

    const LeftType& left;
};

struct MakeComparison {
    template <typename LeftType>
    HalfComparison<LeftType> operator<=(const LeftType& left) {
        return HalfComparison<LeftType>(left);
    }
};

inline void assertionFailed(std::string_view expression, std::string_view expansion, std::string_view message,
                            std::string_view file, int line) {
    fmt::println(stderr, "assertion failed at {}:{}:\n{} | {}\n{}", file, line, expression, expansion,
                 message);
    std::fflush(stderr);
    BRISK_BREAKPOINT;
    std::terminate();
}

template <typename TypeName>
inline std::string toStringSafe(const TypeName& value) {
    using T = std::remove_cvref_t<TypeName>;

    if constexpr (std::is_same_v<T, std::nullptr_t>) {
        return "(nullptr)";
    } else if constexpr (std::is_pointer_v<T>) {
        if (value == nullptr)
            return "(nullptr)";
        return fmt::to_string(fmt::ptr(value));
    } else if constexpr (fmt::is_formattable<T>::value) {
        return fmt::to_string(value);
    }

    return "(value)";
}

template <bool Expected, typename FunctionType, typename LeftType, typename RightType>
inline std::optional<std::string> assertionCheck(const Comparison<FunctionType, LeftType, RightType>& value) {
    const bool result = static_cast<bool>(value()) == Expected;
    if (result) {
        return std::nullopt;
    }
    return toStringSafe(value.left) + " " + toStringSafe(value.op) + " " + toStringSafe(value.right);
}

template <bool Expected, typename LeftType>
inline std::optional<std::string> assertionCheck(const HalfComparison<LeftType>& value) {
    const bool result = static_cast<bool>(value()) == Expected;
    if (result) {
        return std::nullopt;
    }
    return toStringSafe(value.left);
}
} // namespace Internal
} // namespace Brisk

#define BRISK_ASSERT(...)                                                                                    \
    do {                                                                                                     \
        if (auto assertionResult =                                                                           \
                ::Brisk::Internal::assertionCheck<true>(::Brisk::Internal::MakeComparison() <= __VA_ARGS__); \
            assertionResult) [[unlikely]] {                                                                  \
            ::Brisk::Internal::assertionFailed(#__VA_ARGS__, *assertionResult, "", __FILE__, __LINE__);      \
        }                                                                                                    \
    } while (0)

#define BRISK_ASSERT_MSG(msg, ...)                                                                           \
    do {                                                                                                     \
        if (auto assertionResult =                                                                           \
                ::Brisk::Internal::assertionCheck<true>(::Brisk::Internal::MakeComparison() <= __VA_ARGS__); \
            assertionResult) [[unlikely]] {                                                                  \
            ::Brisk::Internal::assertionFailed(#__VA_ARGS__, *assertionResult, msg, __FILE__, __LINE__);     \
        }                                                                                                    \
    } while (0)

#define BRISK_ASSERT_FALSE(...)                                                                              \
    do {                                                                                                     \
        if (auto assertionResult = ::Brisk::Internal::assertionCheck<false>(                                 \
                ::Brisk::Internal::MakeComparison() <= __VA_ARGS__);                                         \
            assertionResult) [[unlikely]] {                                                                  \
            ::Brisk::Internal::assertionFailed(#__VA_ARGS__, *assertionResult, "", __FILE__, __LINE__);      \
        }                                                                                                    \
    } while (0)

#define BRISK_ASSERT_FALSE_MSG(msg, ...)                                                                     \
    do {                                                                                                     \
        if (auto assertionResult = ::Brisk::Internal::assertionCheck<false>(                                 \
                ::Brisk::Internal::MakeComparison() <= __VA_ARGS__);                                         \
            assertionResult) [[unlikely]] {                                                                  \
            ::Brisk::Internal::assertionFailed(#__VA_ARGS__, *assertionResult, msg, __FILE__, __LINE__);     \
        }                                                                                                    \
    } while (0)
