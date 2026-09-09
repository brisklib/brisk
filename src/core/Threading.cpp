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
#include <concurrentqueue/concurrentqueue.h>

#include <brisk/core/Encoding.hpp>
#include <brisk/core/Log.hpp>
#include <brisk/core/Threading.hpp>
#include <brisk/core/Utilities.hpp>
#include <brisk/core/internal/Lock.hpp>

#include "Uv.hpp"

namespace Brisk {

std::thread::id mainThreadId = std::this_thread::get_id();

namespace {

std::atomic<Internal::WakeUpMainThreadFn> wakeUpMainThreadFn{ nullptr };

} // namespace

void Internal::wakeUpMainThread() {
    const WakeUpMainThreadFn fn = wakeUpMainThreadFn.load(std::memory_order_acquire);
    if (fn) {
        BRISK_SUPPRESS_EXCEPTIONS(fn());
    }
}

void Internal::setWakeUpMainThread(WakeUpMainThreadFn fn) noexcept {
    wakeUpMainThreadFn.store(fn, std::memory_order_release);
}

bool isMainThread() {
    return std::this_thread::get_id() == mainThreadId;
}

void mustBeMainThread() {
    BRISK_ASSERT(isMainThread() == true);
}

struct TimerItem {
    double targetTime;
    VoidFunc fn;
};

static std::vector<TimerItem> timers;

void setTimeout(double time_s, VoidFunc fn) {
    if (fn && mainScheduler) {
        TimerItem item{ currentTime() + std::max(0.0, time_s), std::move(fn) };
        mainScheduler->dispatch([item = std::move(item)] {
            auto position = std::upper_bound(timers.begin(), timers.end(), item,
                                             [](const TimerItem& x, const TimerItem& y) BRISK_INLINE_LAMBDA {
                                                 return x.targetTime > y.targetTime;
                                             });
            timers.insert(position, item);
        });
    }
}

void processTimers() {
    if (!isMainThread())
        return;
    const double time = currentTime();
    while (!timers.empty()) {
        if (time >= timers.back().targetTime) {
            TimerItem item = std::move(timers.back());
            timers.pop_back();
            BRISK_SUPPRESS_EXCEPTIONS(item.fn());
        } else {
            break;
        }
    }
}

double Internal::nextTimerDelay() noexcept {
    if (timers.empty())
        return -1.0;
    return std::max(0.0, timers.back().targetTime - currentTime());
}

void async(function<void()> fn) {
    Loop::main().async(std::move(fn));
}

void Internal::clearTimers() noexcept {
    BRISK_ASSERT(isMainThread());
    if (!isMainThread())
        return;
    timers.clear();
}

std::thread::id TaskQueue::getThreadId() const noexcept {
    return m_threadId.load(std::memory_order_acquire);
}

void TaskQueue::process() noexcept {
    BRISK_ASSERT(isOnThread());
    if (!isOnThread())
        return;
    ++m_processing;
    SCOPE_EXIT {
        --m_processing;
    };
    VoidFunc func;
    while (tryDequeue(func)) {
        BRISK_SUPPRESS_EXCEPTIONS(func());
    }
}

bool TaskQueue::isProcessing() const noexcept {
    return m_processing > 0;
}

bool TaskQueue::isOnThread() const noexcept {
    return std::this_thread::get_id() == m_threadId.load(std::memory_order_acquire);
}

std::future<void> TaskQueue::dispatch(VoidFunc func, ExecuteImmediately mode) {
    std::promise<void> promise;
    std::future<void> future = promise.get_future();
    if (mode != ExecuteImmediately::Never && isOnThread()) {
        if (mode == ExecuteImmediately::IfOnThread || isProcessing()) {
            BRISK_SUPPRESS_EXCEPTIONS(func());
            promise.set_value();
            return future;
        }
    }
    enqueue([func = std::move(func), promise = std::move(promise)]() mutable {
        BRISK_SUPPRESS_EXCEPTIONS(func());
        promise.set_value();
    });
    return future;
}

struct TaskQueue::Impl {
    moodycamel::ConcurrentQueue<VoidFunc> m_q{};
    moodycamel::ConsumerToken m_tok{ m_q };
};

void TaskQueue::enqueue(VoidFunc func) noexcept {
    m_impl->m_q.enqueue(std::move(func));
    if (m_threadId.load(std::memory_order_acquire) == mainThreadId) {
        Internal::wakeUpMainThread();
    }
}

bool TaskQueue::tryDequeue(VoidFunc& func) noexcept {
    return m_impl->m_q.try_dequeue(m_impl->m_tok, func);
}

TaskQueue::TaskQueue() : m_threadId(std::this_thread::get_id()), m_impl(new TaskQueue::Impl()) {}

TaskQueue::~TaskQueue() {}

std::future<void> Scheduler::completionFuture() {
    std::promise<void> promise;
    std::future<void> future = promise.get_future();
    dispatch(
        [promise = std::move(promise)]() mutable {
            promise.set_value();
        },
        ExecuteImmediately::Never);
    return future;
}

void Scheduler::waitForCompletion() {
    waitFuture(completionFuture(), 0);
}

Rc<TaskQueue> mainScheduler;
Rc<Scheduler> noScheduler; // always nullptr

void TaskQueue::setThreadId(std::thread::id threadId) noexcept {
    m_threadId.store(threadId, std::memory_order_release);
}

Thread::Thread() {}

Thread::~Thread() {
    // stop thread and wait
    m_terminate.store(true, std::memory_order_release);
    if (m_thread.joinable()) {
        m_thread.join();
    }
}

void Thread::start() {
    if (m_thread.joinable()) {
        return;
    }
    m_terminate.store(false, std::memory_order_release);
    m_thread = std::thread([this] {
        BRISK_SUPPRESS_EXCEPTIONS(threadBody());
    });
}

std::thread::id Thread::get_id() const noexcept {
    return m_thread.get_id();
}

bool Thread::isTerminated() const noexcept {
    return m_terminate.load(std::memory_order_acquire);
}

void Thread::threadBody() {}
} // namespace Brisk
