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
#include <brisk/core/internal/InlineVector.hpp>
#include <catch2/catch_all.hpp>
#include "Catch2Utils.hpp"

namespace Brisk {

TEST_CASE("inline_vector") {
    inline_vector<int, 4> a;
    CHECK(a.size() == 0);
    CHECK(a.empty());
    CHECK(a.begin() == a.end());

    SECTION("ctor") {
        inline_vector<int, 4> a2{ 4, 3, 2, 1 };
        CHECK(a2.size() == 4);
        a = a2;
        CHECK(a.size() == 4);
        CHECK(std::vector<int>(a.begin(), a.end()) == std::vector<int>{ 4, 3, 2, 1 });

        CHECK_THROWS(inline_vector<int, 3>{ 4, 3, 2, 1 });
    }

    SECTION("push_back") {
        a.push_back(1);
        CHECK(a.size() == 1);
        CHECK(!a.empty());
        CHECK(a.begin() + 1 == a.end());
        CHECK(a[0] == 1);
        CHECK_THROWS(a.at(1));
        CHECK(std::vector<int>(a.begin(), a.end()) == std::vector<int>{ 1 });

        a.push_back(2);
        a.push_back(3);
        a.push_back(4);
        CHECK(a.size() == 4);
        CHECK(a[1] == 2);
        CHECK(a[2] == 3);
        CHECK(a[3] == 4);
        CHECK(std::vector<int>(a.begin(), a.end()) == std::vector<int>{ 1, 2, 3, 4 });

        CHECK(std::vector<int>(a.data(), a.data() + a.size()) == std::vector<int>{ 1, 2, 3, 4 });

        CHECK_THROWS(a.push_back(5));
        CHECK(a.size() == 4);
    }
}

TEST_CASE("inline_vector_comparison") {
    inline_vector<uint8_t, 3> a{};
    inline_vector<uint8_t, 3> b{ 1, 2, 3 };

    CHECK(a != b);
    a.push_back(1);
    CHECK(a != b);
    a.push_back(2);
    CHECK(a != b);
    a.push_back(3);
    CHECK(a == b);
}
} // namespace Brisk
