/*
 * Brisk
 *
 * Cross-platform application framework
 * --------------------------------------------------------------
 *
 * Copyright (C) 2024 Brisk Developers
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

#if defined(_M_IX86) || defined(__i386__) || defined(_M_X64) || defined(__x86_64__) || defined(__wasm)
#define BRISK_X86 1
#elif defined(__arm__) || defined(__arm64__) || defined(_M_ARM) || defined(__aarch64__)
#define BRISK_ARM 1
#endif

#if defined(_M_X64) || defined(__x86_64__) || defined(__wasm64) || defined(__aarch64__)
#define BRISK_X64 1
#else
#define BRISK_X32 1
#endif

#define BRISK_STRINGIFY2(x) #x
#define BRISK_STRINGIFY(x) BRISK_STRINGIFY2(x)

#define BRISK_CONCAT2(a, b) a##b
#define BRISK_CONCAT(a, b) BRISK_CONCAT2(a, b)

#ifdef _MSC_VER
#define BRISK_FUNCTION_NAME __FUNCSIG__
#else
#define BRISK_FUNCTION_NAME __PRETTY_FUNCTION__
#endif

#if defined(_WIN32)
#define BRISK_WINDOWS 1
#endif

#if defined(__APPLE__)
#include "TargetConditionals.h"
#if defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE
#define BRISK_IOS 1
#elif defined(TARGET_IPHONE_SIMULATOR) && TARGET_IPHONE_SIMULATOR
#define BRISK_IOS 1
#define BRISK_IOS_SIMULATOR 1
#elif defined(TARGET_OS_MAC) && TARGET_OS_MAC
#define BRISK_MACOS 1
#endif
#define BRISK_APPLE 1
#define BRISK_POSIX 1
#endif

#if defined(__ANDROID__)
#define BRISK_ANDROID 1
#define BRISK_POSIX 1
#elif defined(__linux__)
#define BRISK_LINUX 1
#define BRISK_POSIX 1
#endif

#if defined BRISK_IOS
#define BRISK_OS_NAME "iOS"
#elif defined BRISK_MACOS
#define BRISK_OS_NAME "macOS"
#elif defined BRISK_ANDROID
#define BRISK_OS_NAME "Android"
#elif defined BRISK_LINUX
#define BRISK_OS_NAME "Linux"
#elif defined BRISK_WINDOWS
#define BRISK_OS_NAME "Windows"
#else
#error unknown system
#endif

#if defined(__clang__)
#define BRISK_CLANG 1
#define BRISK_GNU_ATTR 1
#endif

#if defined(_MSC_VER)
#define BRISK_MSVC_ATTR 1
#if defined(__clang__)
#define BRISK_CLANG_CL 1
#else
#define BRISK_MSVC 1
#endif
#endif

#if defined(__GNUC__)
#define BRISK_GNU 1
#if !defined(__clang__)
#define BRISK_GCC 1
#define BRISK_GNU_ATTR 1
#endif
#endif

#ifdef BRISK_MSVC
#define BRISK_IF_MSVC(...) __VA_ARGS__
#define BRISK_MSVC_PRAGMA(...) _Pragma(#__VA_ARGS__)
#else
#define BRISK_IF_MSVC(...)
#define BRISK_MSVC_PRAGMA(...)
#endif

#ifdef BRISK_GNU
#define BRISK_IF_GNU(...) __VA_ARGS__
#define BRISK_GNU_PRAGMA(...) _Pragma(#__VA_ARGS__)
#else
#define BRISK_IF_GNU(...)
#define BRISK_GNU_PRAGMA(...)
#endif

#ifdef BRISK_GNU_ATTR
#define BRISK_IF_GNU_ATTR(...) __VA_ARGS__
#define BRISK_GNU_ATTR_PRAGMA(...) _Pragma(#__VA_ARGS__)
#else
#define BRISK_IF_GNU_ATTR(...)
#define BRISK_GNU_ATTR_PRAGMA(...)
#endif

#ifdef BRISK_GCC
#define BRISK_IF_GCC(...) __VA_ARGS__
#define BRISK_GCC_PRAGMA(...) _Pragma(#__VA_ARGS__)
#else
#define BRISK_IF_GCC(...)
#define BRISK_GCC_PRAGMA(...)
#endif

#ifdef BRISK_CLANG
#define BRISK_IF_CLANG(...) __VA_ARGS__
#define BRISK_CLANG_PRAGMA(...) _Pragma(#__VA_ARGS__)
#else
#define BRISK_IF_CLANG(...)
#define BRISK_CLANG_PRAGMA(...)
#endif

#ifdef BRISK_GNU_ATTR
#define BRISK_ALWAYS_INLINE __attribute__((__always_inline__))
#else
#define BRISK_ALWAYS_INLINE __forceinline
#endif

#ifdef NDEBUG
#define BRISK_INLINE BRISK_ALWAYS_INLINE
#else
#define BRISK_INLINE
#endif

#ifdef __cplusplus
#ifdef BRISK_GNU_ATTR
#define BRISK_INLINE_LAMBDA BRISK_INLINE
#else
#define BRISK_INLINE_LAMBDA [[msvc::forceinline]]
#endif
#else
#endif

#ifdef BRISK_X86
#if defined __SSE2__ || defined(_M_X64) || defined(__x86_64__) || defined(__wasm64)
#define BRISK_SSE2
#else
// #error SSE2 is required on x86
#endif
#endif

#ifdef BRISK_ARM
#if defined __ARM_NEON__ || defined __ARM_NEON
#define BRISK_NEON
#else
#error NEON is required on ARM
#endif
#endif

#ifdef __cplusplus

#if __cpp_concepts < 201907L
#error concepts feature is required
#endif

#if __cpp_designated_initializers < 201707L
#error designated initializers feature is required
#endif

#if __cpp_using_enum < 201907L
#error using enum feature is required
#endif

#if __cpp_impl_coroutine < 201902L
#error coroutine feature is required
#endif

#if defined(__GNUC__) && defined(__EXCEPTIONS)
#define BRISK_EXCEPTIONS 1
#elif defined(_MSC_VER) && defined(_CPPUNWIND)
#define BRISK_EXCEPTIONS 1
#elif __has_feature(cxx_exceptions)
#define BRISK_EXCEPTIONS 1
#endif

#if defined(__GNUC__) && defined(__GXX_RTTI)
#define BRISK_RTTI 1
#elif defined(_MSC_VER) && defined(_CPPRTTI)
#define BRISK_RTTI 1
#elif __has_feature(cxx_rtti)
#define BRISK_RTTI 1
#endif

#if defined(DEBUG) || defined(_DEBUG) || defined(BRISK_CFG_Debug)
#define BRISK_DEBUG 1
#endif

#endif // ifdef __cplusplus

#ifdef __GNUC__
[[noreturn]] inline __attribute__((always_inline)) void unreachable_path() {
    __builtin_unreachable();
}
#elif defined(_MSC_VER)
[[noreturn]] __forceinline void unreachable_path() {
    __assume(false);
}
#else
#error unsupported compiler
#endif
#define BRISK_UNREACHABLE() unreachable_path()

#ifdef BRISK_CLANG
#ifdef __cplusplus
#define BRISK_ASSUME(...) __builtin_assume(static_cast<bool>(__VA_ARGS__))
#else
#define BRISK_ASSUME(...) __builtin_assume(__VA_ARGS__)
#endif
#else
#define BRISK_ASSUME(...)                                                                                    \
    do {                                                                                                     \
    } while (0)
#endif
