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
#include <catch2/catch_all.hpp>

#include <brisk/core/Brisk.h>
#include <brisk/core/DynamicLibrary.hpp>

using namespace Brisk;

namespace {

Rc<DynamicLibrary> loadKnownSystemLibrary() {
#if defined(BRISK_WINDOWS)
    return DynamicLibrary::load("kernel32.dll");
#elif defined(BRISK_MACOS)
    return DynamicLibrary::load("/usr/lib/libSystem.B.dylib");
#elif defined(BRISK_ANDROID)
    return DynamicLibrary::load("libc.so");
#elif defined(BRISK_LINUX) && defined(__GLIBC__)
    return DynamicLibrary::load("libc.so.6");
#else
    return nullptr;
#endif
}

} // namespace

TEST_CASE("DynamicLibrary reports a missing symbol") {
#if !defined(BRISK_WINDOWS) && !defined(BRISK_MACOS) && !defined(BRISK_ANDROID) && \
    !(defined(BRISK_LINUX) && defined(__GLIBC__))
    SKIP("No portable system library name is available for this target");
#else
    auto lib = loadKnownSystemLibrary();
    REQUIRE(lib);
    CHECK(lib->func<void()>("") == nullptr);
#endif
}

TEST_CASE("DynamicLibrary resolves a known platform function") {
#if !defined(BRISK_WINDOWS) && !defined(BRISK_MACOS) && !defined(BRISK_ANDROID) && \
    !(defined(BRISK_LINUX) && defined(__GLIBC__))
    SKIP("No portable system library name is available for this target");
#else
    auto lib = loadKnownSystemLibrary();
    REQUIRE(lib);
#if defined(BRISK_WINDOWS)
    auto getCurrentProcessIdFunction = lib->func<unsigned long __stdcall()>("GetCurrentProcessId");
    REQUIRE(getCurrentProcessIdFunction);
    CHECK(getCurrentProcessIdFunction() != 0);
#else
    auto strlen = lib->func<std::size_t(const char*)>("strlen");
    REQUIRE(strlen);
    CHECK(strlen("brisk") == 5);
#endif
#endif
}

TEST_CASE("DynamicFunc reports missing functions") {
#if !defined(BRISK_WINDOWS) && !defined(BRISK_MACOS) && !defined(BRISK_ANDROID) && \
    !(defined(BRISK_LINUX) && defined(__GLIBC__))
    SKIP("No portable system library name is available for this target");
#else
    auto lib = loadKnownSystemLibrary();
    REQUIRE(lib);
    bool available = true;
    DynamicFunc<void()> missing(lib, "", available);
    CHECK_FALSE(available);
#endif
}

TEST_CASE("DynamicFunc wraps a resolved function") {
#if !defined(BRISK_WINDOWS) && !defined(BRISK_MACOS) && !defined(BRISK_ANDROID) && \
    !(defined(BRISK_LINUX) && defined(__GLIBC__))
    SKIP("No portable system library name is available for this target");
#else
    auto lib = loadKnownSystemLibrary();
    REQUIRE(lib);
#if defined(BRISK_WINDOWS)
    auto getCurrentProcessIdFunction = lib->func<unsigned long __stdcall()>("GetCurrentProcessId");
    REQUIRE(getCurrentProcessIdFunction);
    DynamicFunc<unsigned long __stdcall()> getCurrentProcessId(lib, "GetCurrentProcessId");
    CHECK(getCurrentProcessId() != 0);
#else
    auto strlenFunction = lib->func<std::size_t(const char*)>("strlen");
    REQUIRE(strlenFunction);
    DynamicFunc<std::size_t(const char*)> strlen(lib, "strlen");
    CHECK(strlen("brisk") == 5);
#endif
#endif
}
