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
#include <cstdint>
#include <cstdio>
#include <random>

#include <fmt/format.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_sinks.h>

#include <brisk/core/Log.hpp>
#include <brisk/core/Settings.hpp>
#include <brisk/core/System.hpp>
#include <brisk/core/Text.hpp>
#include <brisk/core/Version.hpp>
#if defined(BRISK_WINDOWS)
#include <spdlog/sinks/msvc_sink.h>
#endif

#include <brisk/core/App.hpp>

namespace Brisk {

fs::path logfolder() {
    return defaultFolder(DefaultFolder::AppUserData) / "logs";
}

static std::string logFilename() {
    // Use the raw count of steady_clock (no wall-clock dependence, sub-second
    // resolution) to reduce the chance of filename collisions between runs.
    const auto ticks =
        static_cast<std::uint64_t>(std::chrono::steady_clock::now().time_since_epoch().count());
    static std::string filename = (fmt::format("{}-{:04X}.log", ticks, std::random_device{}() & 0xFFFF));
    return filename;
}

#if defined(_WIN32) && !defined(BRISK_LOG_TO_STDERR)
using debugsink_t = spdlog::sinks::msvc_sink_mt;
#else
using debugsink_t = spdlog::sinks::stderr_sink_mt;
#endif

static std::vector<spdlog::sink_ptr>& sinks() {
    static std::vector<spdlog::sink_ptr> list = [] {
        std::vector<spdlog::sink_ptr> result;
        std::error_code ec;
        fs::create_directories(logfolder(), ec);
        if (ec) {
            std::fprintf(stderr, "*** LOG ERROR: cannot create log directory '%s': %s ***\n",
                         logfolder().string().c_str(), ec.message().c_str());
        } else {
            try {
                result.emplace_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>(
                    (logfolder() / logFilename()).string()));
            } catch (const spdlog::spdlog_ex& error) {
                std::fprintf(stderr, "*** LOG ERROR: cannot create file sink: %s ***\n", error.what());
            }
        }
#ifndef NODEBUGLOGS
        result.emplace_back(std::make_shared<debugsink_t>());
#endif
        return result;
    }();
    return list;
}

static void printSystemInformation() {
    BRISK_LOG_INFO("Brisk {} running on {}", Brisk::version, osName());
    BRISK_LOG_INFO("Brisk build info {}", replaceAll(Brisk::buildInfo, ";", "\n"));

    BRISK_LOG_INFO(
        "steady_clock granularity = {}ns",
        std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::duration(1)).count());
    BRISK_LOG_INFO(
        "high_resolution_clock granularity = {}ns",
        std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::duration(1))
            .count());
    BRISK_LOG_INFO(
        "system_clock granularity = {}ns",
        std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::duration(1)).count());

    BRISK_LOG_INFO("settingsFile = {}", Settings::path().string());
}

void initializeLogs() {
    std::error_code ec;
    fs::create_directories(logfolder(), ec);
    if (ec) {
        std::fprintf(stderr, "*** LOG ERROR: cannot create log directory '%s': %s ***\n",
                     logfolder().string().c_str(), ec.message().c_str());
    }
    Brisk::Internal::applog().set_level(spdlog::level::trace);
    printSystemInformation();
}

spdlog::logger& Internal::applog() {
    static spdlog::logger instance("", sinks().begin(), sinks().end());
    static const bool errorHandlerSet = [] {
        // Prevent logging failures (e.g. I/O errors) from throwing out of
        // logging macros; report them to stderr instead.
        instance.set_error_handler([](const std::string& msg) {
            std::fprintf(stderr, "*** LOG ERROR: %s ***\n", msg.c_str());
        });
        return true;
    }();
    (void)errorHandlerSet;
    return instance;
}

} // namespace Brisk
