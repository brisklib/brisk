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
#include <span>

namespace std {

template <typename T1, size_t E1, typename T2, size_t E2>
constexpr bool operator==(span<T1, E1> l, span<T2, E2> r) {
    return (l.size() == r.size() && equal(l.begin(), l.end(), r.begin()));
}

template <typename T1, size_t E1, typename T2, size_t E2>
constexpr bool operator!=(span<T1, E1> l, span<T2, E2> r) {
    return !operator==(l, r);
}

} // namespace std
