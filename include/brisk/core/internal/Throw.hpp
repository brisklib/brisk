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

#include <type_traits>
#include <utility>
#include <exception>

#include <brisk/core/Brisk.h>
#include <brisk/core/internal/Typename.hpp>

namespace Brisk {

/**
 * @brief Exception-handling macros that remain syntactically valid when exceptions are disabled.
 *
 * Use these macros in the form:
 * @code
 * BRISK_TRY {
 *     operation();
 * } BRISK_CATCH(const std::exception& error) {
 *     handle(error);
 * } BRISK_CATCH_ALL {
 *     handleUnknownError();
 * }
 * @endcode
 *
 * In a build without exception support, the try body is executed and catch bodies
 * are discarded at compile time. Catch handlers should therefore not be required
 * for the no-exception code path.
 */
#ifdef BRISK_EXCEPTIONS
#define BRISK_TRY try
#define BRISK_CATCH(...) catch (__VA_ARGS__)
#define BRISK_CATCH_ALL catch (...)
#else
#define BRISK_TRY if constexpr (true)
#define BRISK_CATCH(...) else if constexpr (false)
#define BRISK_CATCH_ALL else if constexpr (false)
#endif

void logException(std::string_view className, std::string_view message);

/**
 * @brief Throws an exception of the specified type or terminates the program.
 *
 * This function logs the exception and either throws it if exceptions are enabled,
 * or calls std::terminate if exceptions are disabled.
 *
 * @tparam ExceptionType The type of the exception to be thrown, which must be derived from std::exception.
 * @param exc The exception object to throw.
 * @throw std::exception The specified exception type.
 * @note This function will not return; it either throws the exception or terminates the program.
 */
template <typename ExceptionType>
[[noreturn]] void throwException(ExceptionType&& exc) {
    static_assert(std::is_base_of_v<std::exception, ExceptionType>);
    logException(Internal::typeName<ExceptionType>(), exc.what());
#ifdef BRISK_EXCEPTIONS
    throw std::move(exc);
#else
    std::terminate();
#endif
}
} // namespace Brisk
