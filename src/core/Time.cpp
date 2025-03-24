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
#include <brisk/core/Time.hpp>
#include <brisk/core/Threading.hpp>

namespace Brisk {

Clock::time_point appStartTime = Clock::now();

SingleTimerThread::SingleTimerThread() : m_thread(&SingleTimerThread::run, this) {}

SingleTimerThread::~SingleTimerThread() {
    terminate();
    m_thread.join();
}

void SingleTimerThread::terminate() {
    m_terminated = true;
}

void SingleTimerThread::run() {
    setThreadPriority(ThreadPriority::Highest);
    setThreadName("SingleTimerThread");
    auto time = now();
    while (!m_terminated) {
        tick(time);
        std::this_thread::sleep_until(time);
    }
}

PeriodicTimer::PeriodicTimer(bool startNow) : time(NAN) {
    if (startNow)
        start();
}

void PeriodicTimer::stop() {
    time = NAN;
}

void PeriodicTimer::start() {
    time = currentTime();
}

bool PeriodicTimer::active() const {
    return !std::isnan(time);
}

bool PeriodicTimer::elapsed(double period) {
    if (!active())
        return false;

    double now = currentTime();
    if (now - time >= period) {
        time = now;
        return true;
    }
    return false;
}

PerformanceDuration perfNow() {
    return std::chrono::duration_cast<PerformanceDuration>(timeSinceStart());
}

Stopwatch::Stopwatch(PerformanceDuration& target) : target(target), startTime(perfNow()) {}

Stopwatch::~Stopwatch() {
    const PerformanceDuration currentTime = perfNow();
    target += currentTime - startTime;
}
} // namespace Brisk
