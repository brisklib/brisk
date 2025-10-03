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

#include <spdlog/spdlog.h>

namespace Brisk {

namespace Internal {
/**
 * @brief Retrieves the application logger instance.
 *
 * This function provides access to the application's logger, which is configured
 * to handle logging throughout the Brisk namespace.
 *
 * @return A reference to the application logger (an instance of spdlog::logger).
 */
spdlog::logger& applog();
} // namespace Internal

/**
 * @def BRISK_LOG_LOG(LEVEL, ...)
 * @brief Logs a message with the specified log level.
 *
 * This macro is used to log a message with a specified log level. It is a wrapper around
 * the logger instance obtained from `applog()`. Supported log levels include trace, debug,
 * info, warn, error, and critical.
 *
 * @param LEVEL The log level (e.g., `trace`, `debug`, `info`, etc.).
 * @param ... The format string followed by its arguments for logging.
 */
#define BRISK_LOG_LOG(LEVEL, ...)                                                                            \
    do {                                                                                                     \
        ::Brisk::Internal::applog().LEVEL(__VA_ARGS__);                                                      \
    } while (0)

/**
 * @def BRISK_LOG_LOG_CHECK(LEVEL, COND, ...)
 * @brief Logs a message with the specified log level if the condition is false.
 *
 * This macro checks a condition, and if the condition evaluates to false, it logs the specified
 * message at the given log level. Useful for debugging or handling unexpected states.
 *
 * @param LEVEL The log level (e.g., `trace`, `debug`, `info`, etc.).
 * @param COND The condition to check. The log message is printed if this evaluates to `false`.
 * @param ... The format string followed by its arguments for logging if the condition fails.
 */
#define BRISK_LOG_LOG_CHECK(LEVEL, COND, ...)                                                                \
    do {                                                                                                     \
        const bool cond = (COND);                                                                            \
        if (!cond) {                                                                                         \
            ::Brisk::Internal::applog().LEVEL(__VA_ARGS__);                                                  \
        }                                                                                                    \
    } while (0)

/**
 * @def BRISK_LOG_NOP(...)
 * @brief A no-operation macro for disabling logging.
 *
 * This macro does nothing and is used to replace logging statements in builds where
 * logging should be disabled (e.g., release builds).
 *
 * @param ... Ignored parameters.
 */
#define BRISK_LOG_NOP(...)                                                                                   \
    do {                                                                                                     \
    } while (0)

#if !defined NDEBUG || defined BRISK_TRACING
/**
 * @def BRISK_LOG_TRACE(fmtstr, ...)
 * @brief Logs a trace-level message.
 *
 * Logs a trace-level message if the `NDEBUG` flag is not defined or `BRISK_TRACING` is enabled.
 * Trace messages are typically used for very fine-grained logging.
 *
 * @param fmtstr The format string for the message.
 * @param ... The arguments for the format string.
 */
#define BRISK_LOG_TRACE(fmtstr, ...) BRISK_LOG_LOG(trace, fmtstr, ##__VA_ARGS__)

/**
 * @def BRISK_LOG_DEBUG(fmtstr, ...)
 * @brief Logs a debug-level message.
 *
 * Logs a debug-level message if the `NDEBUG` flag is not defined or `BRISK_TRACING` is enabled.
 * Debug messages are typically used for development and debugging.
 *
 * @param fmtstr The format string for the message.
 * @param ... The arguments for the format string.
 */
#define BRISK_LOG_DEBUG(fmtstr, ...) BRISK_LOG_LOG(debug, fmtstr, ##__VA_ARGS__)
#else
#define BRISK_LOG_TRACE(fmtstr, ...) BRISK_LOG_NOP()
#define BRISK_LOG_DEBUG(fmtstr, ...) BRISK_LOG_NOP()
#endif

/**
 * @def BRISK_LOG_INFO(fmtstr, ...)
 * @brief Logs an info-level message.
 *
 * Logs an info-level message, providing general runtime information.
 *
 * @param fmtstr The format string for the message.
 * @param ... The arguments for the format string.
 */
#define BRISK_LOG_INFO(fmtstr, ...) BRISK_LOG_LOG(info, fmtstr, ##__VA_ARGS__)

/**
 * @def BRISK_LOG_WARN(fmtstr, ...)
 * @brief Logs a warning-level message.
 *
 * Logs a warning-level message, indicating a potential issue that should be reviewed.
 *
 * @param fmtstr The format string for the message.
 * @param ... The arguments for the format string.
 */
#define BRISK_LOG_WARN(fmtstr, ...) BRISK_LOG_LOG(warn, fmtstr, ##__VA_ARGS__)

/**
 * @def BRISK_LOG_ERROR(fmtstr, ...)
 * @brief Logs an error-level message.
 *
 * Logs an error-level message, indicating an issue that caused or might cause failure.
 *
 * @param fmtstr The format string for the message.
 * @param ... The arguments for the format string.
 */
#define BRISK_LOG_ERROR(fmtstr, ...) BRISK_LOG_LOG(error, fmtstr, ##__VA_ARGS__)

/**
 * @def BRISK_LOG_CRITICAL(fmtstr, ...)
 * @brief Logs a critical-level message.
 *
 * Logs a critical-level message, indicating a severe issue that requires immediate attention.
 *
 * @param fmtstr The format string for the message.
 * @param ... The arguments for the format string.
 */
#define BRISK_LOG_CRITICAL(fmtstr, ...) BRISK_LOG_LOG(critical, fmtstr, ##__VA_ARGS__)

#if !defined NDEBUG || defined BRISK_TRACING
/**
 * @def BRISK_LOG_TRACE_CHECK(COND, fmtstr, ...)
 * @brief Logs a trace-level message if the condition fails.
 *
 * Logs a trace-level message if the provided condition evaluates to `false`. Only active
 * if the `NDEBUG` flag is not defined or `BRISK_TRACING` is enabled.
 *
 * @param COND The condition to check.
 * @param fmtstr The format string for the message.
 * @param ... The arguments for the format string.
 */
#define BRISK_LOG_TRACE_CHECK(COND, fmtstr, ...)                                                             \
    BRISK_LOG_LOG_CHECK(trace, COND, "FAILED: (" #COND ") " fmtstr, ##__VA_ARGS__)

/**
 * @def BRISK_LOG_DEBUG_CHECK(COND, fmtstr, ...)
 * @brief Logs a debug-level message if the condition fails.
 *
 * Logs a debug-level message if the provided condition evaluates to `false`. Only active
 * if the `NDEBUG` flag is not defined or `BRISK_TRACING` is enabled.
 *
 * @param COND The condition to check.
 * @param fmtstr The format string for the message.
 * @param ... The arguments for the format string.
 */
#define BRISK_LOG_DEBUG_CHECK(COND, fmtstr, ...)                                                             \
    BRISK_LOG_LOG_CHECK(debug, COND, "FAILED: (" #COND ") " fmtstr, ##__VA_ARGS__)
#else
#define BRISK_LOG_TRACE_CHECK(COND, fmtstr, ...) BRISK_LOG_NOP()
#define BRISK_LOG_DEBUG_CHECK(COND, fmtstr, ...) BRISK_LOG_NOP()
#endif

/**
 * @def BRISK_LOG_INFO_CHECK(COND, fmtstr, ...)
 * @brief Logs an info-level message if the condition fails.
 *
 * Logs an info-level message if the provided condition evaluates to `false`.
 *
 * @param COND The condition to check.
 * @param fmtstr The format string for the message.
 * @param ... The arguments for the format string.
 */
#define BRISK_LOG_INFO_CHECK(COND, fmtstr, ...)                                                              \
    BRISK_LOG_LOG_CHECK(info, COND, "FAILED: (" #COND ") " fmtstr, ##__VA_ARGS__)

/**
 * @def BRISK_LOG_WARN_CHECK(COND, fmtstr, ...)
 * @brief Logs a warning-level message if the condition fails.
 *
 * Logs a warning-level message if the provided condition evaluates to `false`.
 *
 * @param COND The condition to check.
 * @param fmtstr The format string for the message.
 * @param ... The arguments for the format string.
 */
#define BRISK_LOG_WARN_CHECK(COND, fmtstr, ...)                                                              \
    BRISK_LOG_LOG_CHECK(warn, COND, "FAILED: (" #COND ") " fmtstr, ##__VA_ARGS__)

/**
 * @def BRISK_LOG_ERROR_CHECK(COND, fmtstr, ...)
 * @brief Logs an error-level message if the condition fails.
 *
 * Logs an error-level message if the provided condition evaluates to `false`.
 *
 * @param COND The condition to check.
 * @param fmtstr The format string for the message.
 * @param ... The arguments for the format string.
 */
#define BRISK_LOG_ERROR_CHECK(COND, fmtstr, ...)                                                             \
    BRISK_LOG_LOG_CHECK(error, COND, "FAILED: (" #COND ") " fmtstr, ##__VA_ARGS__)

/**
 * @def BRISK_LOG_CRITICAL_CHECK(COND, fmtstr, ...)
 * @brief Logs a critical-level message if the condition fails.
 *
 * Logs a critical-level message if the provided condition evaluates to `false`.
 *
 * @param COND The condition to check.
 * @param fmtstr The format string for the message.
 * @param ... The arguments for the format string.
 */
#define BRISK_LOG_CRITICAL_CHECK(COND, fmtstr, ...)                                                          \
    BRISK_LOG_LOG_CHECK(critical, COND, "FAILED: (" #COND ") " fmtstr, ##__VA_ARGS__)

/**
 * @brief Flushes the logger.
 *
 * Forces the logger to flush its internal buffer, ensuring that all log messages are written
 * to the output.
 */
inline void logFlush() {
    Internal::applog().flush();
}

/**
 * @brief Initializes the logging system.
 *
 * This function sets up the logging system and prints Brisk version to the log.
 */
void initializeLogs();

} // namespace Brisk
