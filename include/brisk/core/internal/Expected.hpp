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

#include <stdexcept>

#include <tl/expected.hpp>

#include <brisk/core/Brisk.h>

namespace Brisk {
using tl::expected;
using tl::unexpected;

template <typename E>
using status = expected<void, E>;

template <typename E>
constexpr status<E> unexpected_if(bool condition, E error) {
    if (condition)
        return {};
    return unexpected<E>(error);
}

#ifdef BRISK_EXCEPTIONS

template <typename E>
class EUnexpected : public std::runtime_error {
public:
    using ErrorType = E;

    explicit EUnexpected(E value) : std::runtime_error(formatError(value)), m_value(std::move(value)) {}

    const E& error() const noexcept {
        return m_value;
    }

private:
    static std::string formatError(const E& value) {
        if constexpr (requires { toString(value); }) // ADL customization point
            return std::string(toString(value));
        else
            return "Unexpected error";
    }

    E m_value;
};

template <typename T, typename E>
T unwrapOrThrow(expected<T, E> result) {
    if (!result)
        throw EUnexpected<E>(std::move(result.error()));
    return std::move(*result);
}
#endif

} // namespace Brisk
