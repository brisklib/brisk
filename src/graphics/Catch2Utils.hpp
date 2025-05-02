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
#include "../core/Catch2Utils.hpp"
#include <brisk/graphics/Matrix.hpp>
#include <brisk/core/Reflection.hpp>

template <typename T>
struct fmt::formatter<Brisk::MatrixOf<T>> : fmt::formatter<std::string> {
    template <typename FormatContext>
    auto format(const Brisk::MatrixOf<T>& value, FormatContext& ctx) const {
        return fmt::formatter<std::string>::format(fmt::format("{{ {}, {},   {}, {},   {}, {} }}", //
                                                               value.v[0][0],
                                                               value.v[0][1], //
                                                               value.v[1][0], //
                                                               value.v[1][1], //
                                                               value.v[2][0], //
                                                               value.v[2][1]),
                                                   ctx);
    }
};
