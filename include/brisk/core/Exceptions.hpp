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

#include "internal/Throw.hpp"
#include <stdexcept>
#include <type_traits>
#include <fmt/format.h>

namespace Brisk {

/**
 * @class Exception
 * @brief A generic templated exception class that extends standard exceptions.
 *
 * This class allows for creating exceptions with formatted error messages. It supports any base class derived
 * from `std::exception`.
 *
 * @tparam Base The base exception type, which must derive from `std::exception`.
 */
template <typename Base>
class Exception : public Base {
public:
    static_assert(std::is_base_of_v<std::exception, Base>);

    /**
     * @brief Constructs an exception with a message string.
     *
     * @param str The error message string.
     */
    Exception(std::string str) : Base(std::move(str)) {}

    /**
     * @brief Constructs an exception with a message as a C-string.
     *
     * @param str The error message as a C-string.
     */
    Exception(const char* str) : Base(str) {}

    /**
     * @brief Constructs an exception with a formatted error message.
     *
     * @param fmt The format string.
     * @param args Arguments to be formatted.
     */
    template <typename... T>
    Exception(fmt::format_string<T...> fmt, T&&... args)
        : Base(fmt::format(std::move(fmt), std::forward<T>(args)...)) {}
};

/**
 * @class ENotImplemented
 * @brief Exception for indicating unimplemented functionality.
 *
 * This class represents a logic error and extends `std::logic_error`.
 */
class ENotImplemented : public Exception<std::logic_error> {
public:
    using Exception<std::logic_error>::Exception;
};

/**
 * @class ERuntime
 * @brief Exception for runtime errors.
 *
 * This class represents a runtime error and extends `std::runtime_error`.
 */
class ERuntime : public Exception<std::runtime_error> {
public:
    using Exception<std::runtime_error>::Exception;
};

/**
 * @class ERange
 * @brief Exception for range errors.
 *
 * This class represents an out-of-range error and extends `std::range_error`.
 */
class ERange : public Exception<std::range_error> {
public:
    using Exception<std::range_error>::Exception;
};

/**
 * @class ELogic
 * @brief Exception for logical errors.
 *
 * This class represents a logic error and extends `std::logic_error`.
 */
class ELogic : public Exception<std::logic_error> {
public:
    using Exception<std::logic_error>::Exception;
};

/**
 * @class EArgument
 * @brief Exception for invalid arguments.
 *
 * This class represents an invalid argument error and extends `std::invalid_argument`.
 */
class EArgument : public Exception<std::invalid_argument> {
public:
    using Exception<std::invalid_argument>::Exception;
};
} // namespace Brisk
