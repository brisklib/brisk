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
#include <functional>

#include <catch2/catch_all.hpp>

#include <brisk/core/Threading.hpp>


namespace Brisk {

namespace {

std::atomic<int> wakeUpCalls{ 0 };

void countWakeUpCall() {
    ++wakeUpCalls;
}

} // namespace

TEST_CASE("Thread reports an unstarted thread") {
    class TestThread : public Thread {
    public:
        using Thread::Thread;
    };

    STATIC_REQUIRE(std::has_virtual_destructor_v<Thread>);
    TestThread thread;
    CHECK(thread.get_id() == std::thread::id{});
}

TEST_CASE("TaskQueue dispatch modes") {
    TaskQueue queue;
    int calls      = 0;

    auto immediate = queue.dispatch([&] {
        ++calls;
    });
    CHECK(immediate.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready);
    immediate.get();
    CHECK(calls == 1);

    auto deferred = queue.dispatch(
        [&] {
            ++calls;
        },
        ExecuteImmediately::IfProcessing);
    CHECK(deferred.wait_for(std::chrono::milliseconds(0)) == std::future_status::timeout);
    CHECK(calls == 1);
    queue.process();
    deferred.get();
    CHECK(calls == 2);

    auto never = queue.dispatch(
        [&] {
            ++calls;
        },
        ExecuteImmediately::Never);
    CHECK(never.wait_for(std::chrono::milliseconds(0)) == std::future_status::timeout);
    CHECK(calls == 2);
    queue.process();
    never.get();
    CHECK(calls == 3);
}

TEST_CASE("TaskQueue dispatches callable results and exceptions") {
    TaskQueue queue;

    Scheduler& scheduler                = queue;
    std::function<int()> resultFunction = [] {
        return 42;
    };
    auto result = scheduler.dispatch(std::move(resultFunction), ExecuteImmediately::Never);
    CHECK(result.wait_for(std::chrono::milliseconds(0)) == std::future_status::timeout);
    queue.process();
    CHECK(result.get() == 42);

    std::function<int()> failureFunction = []() -> int {
        throw std::runtime_error("dispatch failure");
    };
    auto failure = scheduler.dispatch(std::move(failureFunction), ExecuteImmediately::Never);
    queue.process();
    CHECK_THROWS_AS(failure.get(), std::runtime_error);
}

TEST_CASE("TaskQueue executes IfProcessing work inline while processing") {
    TaskQueue queue;
    bool nestedCalled = false;

    auto outer        = queue.dispatch(
        [&] {
            CHECK(queue.isProcessing());
            auto nested = queue.dispatch(
                [&] {
                    nestedCalled = true;
                },
                ExecuteImmediately::IfProcessing);
            CHECK(nested.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready);
            nested.get();
        },
        ExecuteImmediately::Never);

    queue.process();
    outer.get();
    CHECK(nestedCalled);
    CHECK_FALSE(queue.isProcessing());
}

TEST_CASE("TaskQueue ownership can be changed before processing") {
    TaskQueue queue;
    const std::thread::id owner = std::this_thread::get_id();

    CHECK(queue.getThreadId() == owner);
    CHECK(queue.isOnThread());

    queue.setThreadId(std::thread::id{});
    CHECK(queue.getThreadId() == std::thread::id{});
    CHECK_FALSE(queue.isOnThread());

    int calls   = 0;
    auto future = queue.dispatch(
        [&] {
            ++calls;
        },
        ExecuteImmediately::IfOnThread);
    CHECK(future.wait_for(std::chrono::milliseconds(0)) == std::future_status::timeout);

    queue.setThreadId(owner);
    queue.process();
    future.get();
    CHECK(calls == 1);
}

TEST_CASE("Scheduler completionFuture completes at a queue marker") {
    TaskQueue queue;
    int calls  = 0;

    auto first = queue.dispatch(
        [&] {
            ++calls;
        },
        ExecuteImmediately::Never);
    auto completion = queue.completionFuture();
    CHECK(first.wait_for(std::chrono::milliseconds(0)) == std::future_status::timeout);
    CHECK(completion.wait_for(std::chrono::milliseconds(0)) == std::future_status::timeout);

    queue.process();
    first.get();
    completion.get();
    CHECK(calls == 1);
}

TEST_CASE("waitFuture can pump a manually processed queue") {
    TaskQueue queue;
    Scheduler& scheduler                = queue;
    std::function<int()> resultFunction = [] {
        return 7;
    };
    auto future      = scheduler.dispatch(std::move(resultFunction), ExecuteImmediately::Never);

    const int result = waitFuture(
        [&] {
            queue.process();
        },
        std::move(future), 0);

    CHECK(result == 7);
}

TEST_CASE("main-thread and wake-up helpers") {
    CHECK(isMainThread());

    TaskQueue queue;
    wakeUpCalls = 0;
    Internal::setWakeUpMainThread(&countWakeUpCall);

    auto future = queue.dispatch([] {}, ExecuteImmediately::Never);
    CHECK(wakeUpCalls == 1);
    queue.process();
    future.get();

    Internal::setWakeUpMainThread({});
}

TEST_CASE("Timers execute expired callbacks and support recursive processing") {
    REQUIRE(mainScheduler);
    Internal::clearTimers();

    int calls = 0;
    setTimeout(-1.0, [&] {
        ++calls;
        processTimers();
    });

    CHECK(Internal::nextTimerDelay() == 0.0);
    processTimers();
    CHECK(calls == 1);
    CHECK(Internal::nextTimerDelay() < 0.0);

    Internal::clearTimers();
}

TEST_CASE("Timers can be cancelled by clearing the timer store") {
    REQUIRE(mainScheduler);
    Internal::clearTimers();

    bool called = false;
    setTimeout(60.0, [&] {
        called = true;
    });
    CHECK(Internal::nextTimerDelay() > 0.0);

    Internal::clearTimers();
    processTimers();
    CHECK_FALSE(called);
    CHECK(Internal::nextTimerDelay() < 0.0);
}

TEST_CASE("AsyncOperation supports synchronous success and multiple values") {
    AsyncOperation<int> operation;
    AsyncValue<int> first  = operation.value();
    AsyncValue<int> second = first;

    operation.ready(42);

    CHECK(first.getSync() == 42);
    CHECK(second.getSync() == 42);
}

TEST_CASE("AsyncValue getSync pumps the main scheduler") {
    REQUIRE(mainScheduler);
    AsyncOperation<int> operation;
    AsyncValue<int> value = operation.value();

    auto queued           = mainScheduler->dispatch(
        [&] {
            operation.ready(17);
        },
        ExecuteImmediately::Never);
    CHECK(value.getSync() == 17);
    queued.get();
}

TEST_CASE("AsyncOperation propagates exceptions and executes callables") {
    AsyncOperation<int> successful;
    successful.execute([] {
        return 9;
    });
    CHECK(successful.value().getSync() == 9);

    AsyncOperation<int> failed;
    failed.exception(std::make_exception_ptr(std::runtime_error("operation failure")));
    CHECK_THROWS_AS(failed.value().getSync(), std::runtime_error);
}

TEST_CASE("AsyncValue invokes success and error callbacks through a scheduler") {
    Rc<Scheduler> scheduler = std::make_shared<TaskQueue>();

    AsyncOperation<int> successful;
    bool callbackCalled = false;
    successful.value().getInCallback(scheduler, [&](int result) {
        callbackCalled = result == 23;
    });
    successful.ready(23);
    CHECK(callbackCalled);

    AsyncOperation<int> failed;
    bool errorCalled = false;
    failed.value().getInCallback(
        scheduler,
        [&](int) {
            FAIL("success callback called for a failed operation");
        },
        [&](std::exception_ptr error) {
            errorCalled = error != nullptr;
        });
    failed.exception(std::make_exception_ptr(std::runtime_error("callback failure")));
    CHECK(errorCalled);
}

} // namespace Brisk
