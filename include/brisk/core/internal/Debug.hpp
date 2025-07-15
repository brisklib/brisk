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

#include "../Brisk.h"

#include <string_view>
#include <fmt/format.h>
#include "../internal/Throw.hpp"

#if defined BRISK_MSVC
#include <intrin.h>
#define BRISK_BREAKPOINT __debugbreak()
#else
#if defined BRISK_X86
#define BRISK_BREAKPOINT __asm__ __volatile__("int $0x03")
#else
#define BRISK_BREAKPOINT __builtin_trap()
#endif
#endif

namespace Brisk {

namespace Internal {

template <typename Fn, typename L, typename R>
struct comparison {
    const L& left;
    const R& right;

    comparison(const L& left, const R& right) : left(left), right(right) {}

    constexpr bool operator()() const {
        return Fn{}(left, right);
    }
};

struct cmp_eq {
    static constexpr std::string_view op = "==";

    template <typename L, typename R>
    constexpr bool operator()(const L& left, const R& right) const noexcept(noexcept(left == right)) {
        return left == right;
    }
};

struct cmp_ne {
    static constexpr std::string_view op = "!=";

    template <typename L, typename R>
    constexpr bool operator()(const L& left, const R& right) const noexcept(noexcept(left != right)) {
        return left != right;
    }
};

struct cmp_lt {
    static constexpr std::string_view op = "<";

    template <typename L, typename R>
    constexpr bool operator()(const L& left, const R& right) const noexcept(noexcept(left < right)) {
        return left < right;
    }
};

struct cmp_gt {
    static constexpr std::string_view op = ">";

    template <typename L, typename R>
    constexpr bool operator()(const L& left, const R& right) const noexcept(noexcept(left > right)) {
        return left > right;
    }
};

struct cmp_le {
    static constexpr std::string_view op = "<=";

    template <typename L, typename R>
    constexpr bool operator()(const L& left, const R& right) const noexcept(noexcept(left <= right)) {
        return left <= right;
    }
};

struct cmp_ge {
    static constexpr std::string_view op = ">=";

    template <typename L, typename R>
    constexpr bool operator()(const L& left, const R& right) const noexcept(noexcept(left >= right)) {
        return left >= right;
    }
};

template <typename L>
struct half_comparison {
    half_comparison(const L& left) : left(left) {}

    template <typename R>
    comparison<cmp_eq, L, R> operator==(const R& right) {
        return comparison<cmp_eq, L, R>(left, right);
    }

    template <typename R>
    comparison<cmp_ne, L, R> operator!=(const R& right) {
        return comparison<cmp_ne, L, R>(left, right);
    }

    template <typename R>
    comparison<cmp_lt, L, R> operator<(const R& right) {
        return comparison<cmp_lt, L, R>(left, right);
    }

    template <typename R>
    comparison<cmp_gt, L, R> operator>(const R& right) {
        return comparison<cmp_gt, L, R>(left, right);
    }

    template <typename R>
    comparison<cmp_le, L, R> operator<=(const R& right) {
        return comparison<cmp_le, L, R>(left, right);
    }

    template <typename R>
    comparison<cmp_ge, L, R> operator>=(const R& right) {
        return comparison<cmp_ge, L, R>(left, right);
    }

    const L& left;
};

struct make_comparison {
    template <typename L>
    half_comparison<L> operator<=(const L& left) {
        return half_comparison<L>(left);
    }
};

class EAssert : public std::logic_error {
public:
    using std::logic_error::logic_error;
};

inline void assertion_failed(std::string_view details, std::string_view file, int line) {
#ifdef BRISK_ASSERT_THROWS
    throwException(EAssert(fmt::format("assertion failed at {}:{}:\n{}", file, line, details)));
#else
    fmt::println(stderr, "assertion failed at {}:{}:\n{}", file, line, details);
    std::fflush(stderr);
#endif
}

template <typename T>
inline std::string to_string_safe(const T& value) {
    if constexpr (fmt::is_formattable<T>::value) {
        return fmt::to_string(value);
    } else {
        return "(value)";
    }
}

template <typename Op, typename L, typename R>
inline bool assertion(const comparison<Op, L, R>& comparison, std::string_view expr, std::string_view file,
                      int line) {
    const bool result = comparison();
    if (!result) {
        assertion_failed(fmt::format("{} | {} {} {}", expr, to_string_safe(comparison.left), Op::op,
                                     to_string_safe(comparison.right)),
                         file, line);
    }
    return result;
}

template <typename L>
inline bool assertion(const half_comparison<L>& comparison, std::string_view expr, std::string_view file,
                      int line) {
    const bool result = static_cast<bool>(comparison.left);
    if (!result) {
        assertion_failed(fmt::format("{} | {}", expr, to_string_safe(comparison.left)), file, line);
    }
    return result;
}
} // namespace Internal
} // namespace Brisk

#define BRISK_ASSERT(...)                                                                                    \
    do {                                                                                                     \
        if (!::Brisk::Internal::assertion(::Brisk::Internal::make_comparison() <= __VA_ARGS__, #__VA_ARGS__, \
                                          __FILE__, __LINE__)) [[unlikely]] {                                \
            BRISK_BREAKPOINT;                                                                                \
        }                                                                                                    \
    } while (0)

#define BRISK_SOFT_ASSERT(...)                                                                               \
    do {                                                                                                     \
        if (!::Brisk::Internal::assertion(::Brisk::Internal::make_comparison() <= __VA_ARGS__, #__VA_ARGS__, \
                                          __FILE__, __LINE__)) [[unlikely]] {                                \
            BRISK_BREAKPOINT;                                                                                \
        }                                                                                                    \
    } while (0)

#define BRISK_ASSERT_MSG(msg, ...)                                                                           \
    do {                                                                                                     \
        if (!::Brisk::Internal::assertion(::Brisk::Internal::make_comparison() <= __VA_ARGS__, msg,          \
                                          __FILE__, __LINE__)) [[unlikely]] {                                \
            BRISK_BREAKPOINT;                                                                                \
        }                                                                                                    \
    } while (0)

#define BRISK_SOFT_ASSERT_MSG(msg, ...)                                                                      \
    do {                                                                                                     \
        if (!::Brisk::Internal::assertion(::Brisk::Internal::make_comparison() <= __VA_ARGS__, msg,          \
                                          __FILE__, __LINE__)) [[unlikely]] {                                \
            BRISK_BREAKPOINT;                                                                                \
        }                                                                                                    \
    } while (0)
