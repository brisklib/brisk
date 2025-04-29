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
#include "Uv.hpp"

#include <uv.h>

#include <brisk/core/Threading.hpp>

namespace Brisk {

Loop Loop::createNew() noexcept {
    uv_loop_s* loop = (uv_loop_s*)malloc(uv_loop_size());
    BRISK_ASSERT(loop);
    uv_loop_init(loop);
    return Loop(loop);
}

Loop::Loop(Loop&& move) noexcept : loop(nullptr) {
    swap(move);
}

Loop& Loop::operator=(Loop&& move) noexcept {
    swap(move);
    return *this;
}

Loop::~Loop() {
    if (loop && loop != uv_default_loop()) {
        int result = uv_loop_close(loop);
        BRISK_ASSERT(result >= 0);
        free(loop);
    }
}

Loop::Loop(uv_loop_s* loop) noexcept : loop(loop) {}

void Loop::swap(Loop& other) noexcept {
    std::swap(loop, other.loop);
}

Loop Loop::main() noexcept {
    return Loop{ uv_default_loop() };
}

namespace {

struct UVTimer {
    uv_timer_t uv;
    Loop::function_t fn;
};

template <typename T, typename Arg, typename... Args>
void uvClean(Arg* handle, Args...) {
    delete reinterpret_cast<T*>(handle);
}

void uvTimer(uv_timer_t* handle) {
    BRISK_SUPPRESS_EXCEPTIONS(reinterpret_cast<UVTimer*>(handle)->fn());
    uv_timer_stop(handle);
    uv_close((uv_handle_t*)handle, &uvClean<UVTimer, uv_handle_t>);
}

struct UVWork {
    uv_work_t uv;
    Loop::function_t fn;
};

void uvWork(uv_work_t* req) {
    BRISK_SUPPRESS_EXCEPTIONS(reinterpret_cast<UVTimer*>(req)->fn());
}

} // namespace

void Loop::once(function_t fn) {
    UVTimer* timer = new UVTimer();
    timer->fn      = std::move(fn);
    uv_timer_init(loop, &timer->uv);
    uv_timer_start(&timer->uv, &uvTimer, 0, 0);
}

void Loop::async(function_t fn) {
    UVWork* work = new UVWork();
    work->fn     = std::move(fn);
    uv_queue_work(loop, &work->uv, &uvWork, &uvClean<UVWork, uv_work_t, int>);
}

void Loop::process() {
    uv_run(loop, UV_RUN_NOWAIT);
}

} // namespace Brisk
