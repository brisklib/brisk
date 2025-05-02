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
#include <brisk/core/internal/Typename.hpp>
#include <catch2/catch_all.hpp>
#include "Catch2Utils.hpp"

struct A {
    int first, second, third;
};

namespace Brisk {

TEST_CASE("Typename") {
    CHECK(Internal::typeName<float>() == "float");
#if !defined _MSC_VER || defined __clang__
    CHECK(Internal::valueName<123>() == "123");
#endif

    CHECK(Internal::valueName<&A::first>() == "&A::first");
}
} // namespace Brisk
