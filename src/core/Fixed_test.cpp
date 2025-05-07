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
#include <brisk/core/internal/Fixed.hpp>
#include <catch2/catch_all.hpp>
#include "Catch2Utils.hpp"

template <int I, int F>
struct fmt::formatter<Brisk::fixed<I, F>> : fmt::formatter<std::string> {
    template <typename FormatContext>
    auto format(const Brisk::fixed<I, F>& val, FormatContext& ctx) const {
        std::string str = val.debug();
        return fmt::formatter<std::string>::format(str, ctx);
    }
};

namespace Brisk {

template <int I, int F>
static void test_fixed() {
    INFO("I=" << I);
    INFO("F=" << F);
    using fix           = fixed<I, F>;
    using fix2          = fixed<2 * I, 2 * F>;
    int errors_add      = 0;
    int errors_sub      = 0;
    int errors_mul      = 0;
    int errors_mul_full = 0;
    constexpr int bits  = I + F;
    constexpr int max   = (1 << (bits - 1)) - 1;

    auto in_range       = [](double x) -> bool {
        return static_cast<double>(static_cast<fix>(x)) == x;
    };
    auto in_range2 = [](double x) -> bool {
        return static_cast<double>(static_cast<fix2>(x)) == x;
    };

    for (int x = -max - 1; x <= max; ++x) {
        for (int y = -max - 1; y <= max; ++y) {
            fix xf(fixed_t{}, x);
            fix yf(fixed_t{}, y);
            double xd = x / static_cast<double>(1 << F);
            double yd = y / static_cast<double>(1 << F);
            if (in_range(xd + yd) && static_cast<double>(xf + yf) != xd + yd) {
                ++errors_add;
            }
            if (in_range(xd - yd) && static_cast<double>(xf - yf) != xd - yd) {
                ++errors_sub;
            }
            if (in_range(xd * yd) && static_cast<double>(xf * yf) != xd * yd) {
                ++errors_mul;
            }
            if (in_range2(xd * yd) && static_cast<double>(mul_full(xf, yf)) != xd * yd) {
                ++errors_mul_full;
            }
        }
    }
    CHECK(errors_add == 0);
    CHECK(errors_sub == 0);
    CHECK(errors_mul == 0);
    CHECK(errors_mul_full == 0);
}

TEST_CASE("fixed") {
    CHECK(fixed10_6{ 1.0 }.f == 0x40);
    CHECK(fixed10_6{ 1.015625 }.f == 0x41);
    CHECK(fixed10_6{ 1.25 }.f == 0x50);
    CHECK(fixed10_6{ 1.5 }.f == 0x60);
    CHECK(fixed10_6{ 2.0 }.f == 0x80);
    CHECK(fixed10_6{ -1.015625 }.f == -0x41);
    CHECK(fixed10_6{ 511 }.f == 0x7FC0);
    CHECK(fixed10_6{ 511.984375 }.f == 0x7FFF);
    CHECK(fixed10_6{ -511.984375 }.f == -0x7FFF);
    CHECK(fixed10_6{ -512 }.f == int16_t(0x8000));

    CHECK(static_cast<double>(fixed10_6::minimum()) == -512);
    CHECK(static_cast<double>(fixed10_6::maximum()) == 511.984375);

    CHECK(static_cast<double>(fixed10_6{ fixed_t{}, 0x40 }) == 1.0);
    CHECK(static_cast<double>(fixed10_6{ fixed_t{}, 0x41 }) == 1.015625);
    CHECK(static_cast<double>(fixed10_6{ fixed_t{}, 0x50 }) == 1.25);
    CHECK(static_cast<double>(fixed10_6{ fixed_t{}, 0x60 }) == 1.5);
    CHECK(static_cast<double>(fixed10_6{ fixed_t{}, 0x80 }) == 2.0);
    CHECK(static_cast<double>(fixed10_6{ fixed_t{}, -0x41 }) == -1.015625);
    CHECK(static_cast<double>(fixed10_6{ fixed_t{}, 0x7FC0 }) == 511);
    CHECK(static_cast<double>(fixed10_6{ fixed_t{}, 0x7FFF }) == 511.984375);
    CHECK(static_cast<double>(fixed10_6{ fixed_t{}, -0x7FFF }) == -511.984375);

    CHECK(fixed26_6{ 1.0 }.f == 0x40);
    CHECK(fixed26_6{ 1.015625 }.f == 0x41);
    CHECK(fixed26_6{ 1.25 }.f == 0x50);
    CHECK(fixed26_6{ 1.5 }.f == 0x60);
    CHECK(fixed26_6{ 2.0 }.f == 0x80);
    CHECK(fixed26_6{ -1.015625 }.f == -0x41);
    CHECK(fixed26_6{ 511 }.f == 0x7FC0);
    CHECK(fixed26_6{ 511.984375 }.f == 0x7FFF);
    CHECK(fixed26_6{ -512 }.f == 0xffff8000);
    CHECK(fixed26_6{ 33554431 }.f == 0x7fffffc0);
    CHECK(fixed26_6{ 33554431.984375 }.f == 0x7fffffff);
    CHECK(fixed26_6{ -33554431.984375 }.f == -0x7fffffff);
    CHECK(fixed26_6{ -33554432 }.f == int32_t(0x80000000));

    CHECK(fixed<1, 7>(fixed_t{}, -127) + fixed<1, 7>(0) == fixed<1, 7>(fixed_t{}, -127));

    CHECK(fixed<8, 0>{ 1.0 }.f == int8_t(0x01));
    CHECK(fixed<8, 0>{ 1.1 }.f == int8_t(0x01));
    CHECK(fixed<8, 0>{ 1.5 }.f == int8_t(0x02));
    CHECK(fixed<8, 0>{ 1.9 }.f == int8_t(0x02));

    test_fixed<8, 0>();
    test_fixed<7, 1>();
    test_fixed<6, 2>();
    test_fixed<5, 3>();
    test_fixed<4, 4>();
    test_fixed<3, 5>();
    test_fixed<2, 6>();
    test_fixed<1, 7>();
    test_fixed<0, 8>();
}
} // namespace Brisk
