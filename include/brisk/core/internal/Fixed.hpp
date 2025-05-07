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

#include <cstdint>
#include <type_traits>
#include <cmath>
#include <limits>
#include <fmt/format.h>

namespace Brisk {

struct fixed_t {};

/// @brief Signed Fixed-Point number
template <int Integer, int Fractional>
struct fixed {
    constexpr static int bits = Integer + Fractional;

    // current version supports only the following bit widths:
    static_assert(bits == 8 || bits == 16 || bits == 32);

    using single_t = std::conditional_t<bits <= 8, int8_t, std::conditional_t<bits <= 16, int16_t, int32_t>>;
    using double_t = std::conditional_t<bits <= 8, int16_t, std::conditional_t<bits <= 16, int32_t, int64_t>>;

    constexpr static double_t scale = static_cast<double_t>(1) << Fractional;

    constexpr static fixed minimum() noexcept {
        return fixed{ fixed_t{}, -((static_cast<double_t>(1) << (bits - 1)) - 1) - 1 };
    }

    constexpr static fixed maximum() noexcept {
        return fixed{ fixed_t{}, (static_cast<double_t>(1) << (bits - 1)) - 1 };
    }

    constexpr static fixed epsilon() noexcept {
        return fixed{ fixed_t{}, 1 };
    }

    constexpr fixed() noexcept                        = default;
    constexpr fixed(const fixed&) noexcept            = default;
    constexpr fixed(fixed&&) noexcept                 = default;
    constexpr fixed& operator=(const fixed&) noexcept = default;
    constexpr fixed& operator=(fixed&&) noexcept      = default;

    explicit constexpr fixed(float value) noexcept : f(from_fp(value)) {}

    explicit constexpr fixed(double value) noexcept : f(from_fp(value)) {}

    explicit constexpr fixed(unsigned char value) noexcept : f(from_int(value)) {}

    explicit constexpr fixed(signed char value) noexcept : f(from_int(value)) {}

    explicit constexpr fixed(unsigned short value) noexcept : f(from_int(value)) {}

    explicit constexpr fixed(signed short value) noexcept : f(from_int(value)) {}

    explicit constexpr fixed(unsigned int value) noexcept : f(from_int(value)) {}

    explicit constexpr fixed(signed int value) noexcept : f(from_int(value)) {}

    explicit constexpr fixed(unsigned long value) noexcept : f(from_int(value)) {}

    explicit constexpr fixed(signed long value) noexcept : f(from_int(value)) {}

    explicit constexpr fixed(unsigned long long value) noexcept : f(from_int(value)) {}

    explicit constexpr fixed(signed long long value) noexcept : f(from_int(value)) {}

    constexpr explicit fixed(fixed_t, single_t f) noexcept : f(f) {}

    [[nodiscard]] explicit constexpr operator float() const noexcept {
        return to_fp<float>(f);
    }

    [[nodiscard]] explicit constexpr operator double() const noexcept {
        return to_fp<double>(f);
    }

    template <int SrcInteger, int SrcFractional>
    explicit constexpr fixed(fixed<SrcInteger, SrcFractional> value) noexcept
        : f(static_cast<single_t>(shift<SrcFractional - Fractional>(value.f))) {}

    [[nodiscard]] friend constexpr fixed operator+(fixed x, fixed y) noexcept {
        return fixed{ fixed_t{}, static_cast<single_t>(x.f + y.f) };
    }

    [[nodiscard]] friend constexpr fixed operator-(fixed x, fixed y) noexcept {
        return fixed{ fixed_t{}, static_cast<single_t>(x.f - y.f) };
    }

    [[nodiscard]] friend constexpr fixed operator-(fixed x) noexcept {
        return fixed{ fixed_t{}, static_cast<single_t>(-x.f) };
    }

    [[nodiscard]] friend constexpr fixed operator*(fixed x, fixed y) noexcept {
        const double_t tmp = double_t(x.f) * double_t(y.f);
        auto ff            = shift<Fractional>(tmp);
        return fixed{ fixed_t{}, static_cast<single_t>(ff) };
    }

    [[nodiscard]] friend constexpr fixed operator<<(fixed x, int sh) noexcept {
        return fixed{ fixed_t{}, static_cast<single_t>(x.f << sh) };
    }

    [[nodiscard]] friend constexpr fixed operator>>(fixed x, int sh) noexcept {
        return fixed{ fixed_t{}, static_cast<single_t>(x.f >> sh) };
    }

    [[nodiscard]] friend constexpr bool operator<(fixed x, fixed y) noexcept {
        return x.f < y.f;
    }

    [[nodiscard]] friend constexpr bool operator>(fixed x, fixed y) noexcept {
        return x.f > y.f;
    }

    [[nodiscard]] friend constexpr bool operator<=(fixed x, fixed y) noexcept {
        return x.f <= y.f;
    }

    [[nodiscard]] friend constexpr bool operator>=(fixed x, fixed y) noexcept {
        return x.f >= y.f;
    }

    [[nodiscard]] friend constexpr bool operator==(fixed x, fixed y) noexcept {
        return x.f == y.f;
    }

    [[nodiscard]] friend constexpr bool operator!=(fixed x, fixed y) noexcept {
        return x.f != y.f;
    }

    [[nodiscard]] friend constexpr fixed<2 * Integer, 2 * Fractional> mul_full(fixed x, fixed y) noexcept {
        const double_t tmp = double_t(x.f) * double_t(y.f);
        return fixed<2 * Integer, 2 * Fractional>{ fixed_t{}, tmp };
    }

    [[nodiscard]] friend constexpr fixed<Integer, Fractional> mad(fixed x, fixed y, fixed a) noexcept {
        const double_t tmp = double_t(x.f) * double_t(y.f);
        return fixed{ fixed_t{}, static_cast<single_t>(shift<Fractional>(tmp + shift<-Fractional>(a.f))) };
    }

    single_t f;

    std::string debug() const {
        std::string b = fmt::format("{:0{}B}", uint32_t(f) & ((1 << bits) - 1), bits);
        b.insert(Integer, ".");
        return fmt::format("{:+f} ({})", operator double(), b);
    }

private:
    template <int shift_bits, typename T>
    constexpr static T shift(T value) {
        if constexpr (shift_bits > 0) {
            return (value + (1 << (shift_bits - 1))) >> shift_bits;
        } else {
            return value << -shift_bits;
        }
    }

    template <typename T>
    constexpr static T to_fp(single_t value) {
        return static_cast<T>(value) / scale;
    }

    template <typename T>
    constexpr static single_t from_fp(T value) {
        return static_cast<single_t>(std::llround(value * scale));
    }

    template <typename T>
    constexpr static single_t from_int(T value) {
        return static_cast<single_t>(value << Fractional);
    }
};

using fixed16_16 = fixed<16, 16>;

using fixed26_6  = fixed<26, 6>;

using fixed10_6  = fixed<10, 6>;

using fixed4_4   = fixed<4, 4>;

} // namespace Brisk

namespace std {
template <int Integer, int Fractional>
class numeric_limits<Brisk::fixed<Integer, Fractional>> {
public:
    using Ty                                       = Brisk::fixed<Integer, Fractional>;

    static constexpr int radix                     = 2;
    static constexpr bool is_signed                = true;
    static constexpr bool is_specialized           = true;
    static constexpr bool is_bounded               = true;
    static constexpr float_round_style round_style = round_to_nearest;

    [[nodiscard]] static constexpr Ty(min)() noexcept {
        return Ty::minimum();
    }

    [[nodiscard]] static constexpr Ty(max)() noexcept {
        return Ty::maximum();
    }

    [[nodiscard]] static constexpr Ty lowest() noexcept {
        return Ty::minimum();
    }

    [[nodiscard]] static constexpr Ty epsilon() noexcept {
        return Ty::epsilon();
    }

    [[nodiscard]] static constexpr Ty round_error() noexcept {
        return Ty();
    }

    [[nodiscard]] static constexpr Ty denorm_min() noexcept {
        return Ty();
    }

    [[nodiscard]] static constexpr Ty infinity() noexcept {
        return Ty();
    }

    [[nodiscard]] static constexpr Ty quiet_NaN() noexcept {
        return Ty();
    }

    [[nodiscard]] static constexpr Ty signaling_NaN() noexcept {
        return Ty();
    }
};
} // namespace std
