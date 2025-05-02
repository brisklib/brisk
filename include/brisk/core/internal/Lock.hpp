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

#include <mutex>
#include <atomic>

namespace Brisk {

class spin_lock {
private:
    std::atomic_flag flag = ATOMIC_FLAG_INIT;

public:
    constexpr spin_lock() noexcept {}

    void lock() noexcept {
        while (flag.test_and_set(std::memory_order::acquire)) {}
    }

    bool try_lock() noexcept {
        return !flag.test_and_set(std::memory_order::acquire);
    }

    void unlock() noexcept {
        flag.clear();
    }
};

struct lock_quard_cond {
    lock_quard_cond()                                  = delete;
    lock_quard_cond(const lock_quard_cond&)            = delete;
    lock_quard_cond(lock_quard_cond&&)                 = delete;
    lock_quard_cond& operator=(const lock_quard_cond&) = delete;
    lock_quard_cond& operator=(lock_quard_cond&&)      = delete;

    lock_quard_cond(std::recursive_mutex* mutex_or_nullptr) : mutex(mutex_or_nullptr) {
        if (mutex)
            mutex->lock();
    }

    ~lock_quard_cond() {
        if (mutex)
            mutex->unlock();
    }

    std::recursive_mutex* mutex;
};

template <typename Mtx>
class unlock_guard {
public:
    unlock_guard(const unlock_guard&)            = delete;
    unlock_guard& operator=(const unlock_guard&) = delete;

    unlock_guard(Mtx& mutex) : m_mutex(mutex) {
        m_mutex.unlock();
    }

    ~unlock_guard() {
        m_mutex.lock();
    }

private:
    Mtx& m_mutex;
};
} // namespace Brisk
