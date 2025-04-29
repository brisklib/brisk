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

#include <brisk/core/internal/Function.hpp>

struct uv_loop_s;

namespace Brisk {

struct Loop {
    using function_t = function<void()>;

    Loop(Loop&&) noexcept;
    Loop(const Loop&) = delete;
    Loop& operator=(Loop&&) noexcept;
    Loop& operator=(const Loop&) = delete;
    ~Loop();

    /**
     * @brief Dispatches the function to be called in the loop
     *
     * @param fn Function to be called
     */
    void once(function_t fn);

    /**
     * @brief Dispatches the function to be called in the thread pool
     *
     * @remark Thread-safe
     * @param fn Function to be called
     */
    void async(function_t fn);
    // processes pending events, doesn't wait
    void process();

    static Loop main() noexcept;
    static Loop createNew() noexcept;

    void swap(Loop& other) noexcept;

private:
    uv_loop_s* loop;
    Loop(uv_loop_s* loop) noexcept;
};

} // namespace Brisk
