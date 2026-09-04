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

#include <compare>
#include <utility>

namespace Brisk {

namespace Internal {

#if defined _LIBCPP_VERSION && _LIBCPP_VERSION < 170000
template <typename It1, typename It2, typename Cmp>
constexpr auto lexicographical_compare_three_way(It1 f1, It1 l1, It2 f2, It2 l2, Cmp cmp)
    -> std::common_comparison_category_t<decltype(cmp(*f1, *f2)), std::strong_ordering> {
    using result_type = std::common_comparison_category_t<decltype(cmp(*f1, *f2)), std::strong_ordering>;

    while (f1 != l1 && f2 != l2) {
        if (auto result = cmp(*f1, *f2); result != 0)
            return result;
        ++f1;
        ++f2;
    }

    if (f1 == l1 && f2 == l2)
        return result_type::equivalent;
    return f1 == l1 ? result_type::less : result_type::greater;
}

#define BRISK_LEXICOGRAPHICAL_COMPARE_THREE_WAY(...)                                                         \
    ::Brisk::Internal::lexicographical_compare_three_way(__VA_ARGS__)
#else
#define BRISK_LEXICOGRAPHICAL_COMPARE_THREE_WAY(...) std::lexicographical_compare_three_way(__VA_ARGS__)
#endif
} // namespace Internal

} // namespace Brisk
