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

#include "Time.hpp"
#include "BasicTypes.hpp"
#include "brisk/core/Utilities.hpp"
#include "internal/Function.hpp"
#include <functional>
#include <future>
#include <brisk/core/internal/Debug.hpp>
#include <brisk/core/Log.hpp>

namespace Brisk {

/**
 * @enum ExecuteImmediately
 * @brief Defines when and how a scheduled function is dispatched in a task queue.
 *
 * This enum specifies the conditions under which a scheduled function should be executed
 * in relation to the task queue and its default thread.
 */
enum class ExecuteImmediately {
    /**
     * @brief Executes the scheduled function immediately if the current thread is the queue's default thread.
     *
     * In this mode, the function bypasses the task queue and is invoked directly if the
     * current thread matches the queue's default thread. Otherwise, it will be added to the queue.
     */
    IfOnThread,

    /**
     * @brief Executes the scheduled function immediately only if the task queue is currently processing
     * tasks.
     *
     * In this mode, the function is executed directly on the queue's default thread only
     * when the queue is actively processing tasks. Otherwise, the function will be added
     * to the queue for later execution.
     */
    IfProcessing,

    /**
     * @brief The scheduled function is never executed immediately and is always added to the queue.
     *
     * In this mode, the function is guaranteed to be dispatched to the task queue, ensuring
     * it is invoked only when the queue processes its tasks.
     */
    Never,
};

using VoidFunc = function<void()>;

namespace Internal {

/**
 * @brief Forces the main thread to wake up and process its task queue.
 *
 * This function is particularly useful when the main thread is sleeping in the event loop.
 * The function should interrupts the event loop by posting an empty message, ensuring that the main thread
 * resumes execution and processes its pending tasks.
 *
 * Set in window/WindowApplication.cpp
 */
extern VoidFunc wakeUpMainThread;
} // namespace Internal

bool isMainThread();
void mustBeMainThread();

void setTimeout(double time_s, VoidFunc fn);

namespace Internal {

#ifdef BRISK_EXCEPTIONS
#define BRISK_SUPPRESS_EXCEPTIONS(...)                                                                       \
    try {                                                                                                    \
        __VA_ARGS__;                                                                                         \
    } catch (const std::exception& e) {                                                                      \
        LOG_WARN(core, "Exception suppressed: {}", e.what());                                                \
    } catch (...) {                                                                                          \
        LOG_WARN(core, "Unknown exception suppressed");                                                      \
    }
#else
#define BRISK_SUPPRESS_EXCEPTIONS(...)                                                                       \
    do {                                                                                                     \
        __VA_ARGS__;                                                                                         \
    } while (0)
#endif

template <typename Fn, typename... Args>
void suppressExceptions(Fn&& fn, Args&&... args) {
#ifdef BRISK_EXCEPTIONS
    try {
#endif
        std::forward<Fn>(fn)(std::forward<Args>(args)...);
#ifdef BRISK_EXCEPTIONS
    } catch (const std::exception& e) {
        LOG_WARN(core, "Exception suppressed: {}", e.what());
    } catch (...) {
        LOG_WARN(core, "Unknown exception suppressed");
    }
#endif
}
} // namespace Internal

/**
 * @brief Schedules the function for execution in the thread pool
 *
 * @param fn function to call in the thread pool
 */
void async(function<void()> fn);

void processTimers();

template <typename T>
T waitFuture(VoidFunc waitFunc, std::future<T> future, int intervalMS = 0);

template <typename T>
T waitFuture(std::future<T> future, int intervalMS = 0);

/**
 * @brief Abstract base class for scheduling tasks.
 *
 * This class defines the interface for a scheduler that can dispatch functions
 * asynchronously. Derived classes must implement the `dispatch` method.
 */
class Scheduler {
public:
    /// Virtual destructor.
    virtual ~Scheduler() {}

    /**
     * @brief Dispatches a function for execution.
     *
     * This method schedules the specified function for execution. The function
     * may be executed immediately or added to the task queue depending on the
     * dispatch mode and the current thread context.
     *
     * @param func The function to be executed.
     * @param mode The mode that determines when the function will be executed.
     * @return A future that will be satisfied once the function has completed.
     */
    virtual std::future<void> dispatch(VoidFunc func,
                                       ExecuteImmediately mode = ExecuteImmediately::IfOnThread) noexcept = 0;

    void dispatchAndWait(VoidFunc func, ExecuteImmediately mode = ExecuteImmediately::IfOnThread) noexcept {
        return waitFuture(dispatch(std::move(func), mode));
    }

    /**
     * @brief Dispatches a callable and returns a future for the result.
     *
     * This method allows the dispatch of any callable (function, lambda, etc.)
     * that returns a result, providing a future for the result.
     *
     * @tparam Callable The type of the callable (e.g., function, lambda) to dispatch.
     * @tparam Args The types of the arguments to pass to the callable.
     * @param func The callable to be dispatched.
     * @param mode The mode that determines when the callable will be executed.
     * @param args The arguments to pass to the callable.
     * @return A future that will contain the result of the callable when it completes.
     * @threadsafe This method is thread-safe and can be called from any thread.
     */
    template <typename Callable, typename ReturnType = std::invoke_result_t<Callable>>
    std::future<ReturnType> dispatch(Callable&& func,
                                     ExecuteImmediately mode = ExecuteImmediately::IfOnThread)
        requires(!std::is_same_v<ReturnType, void>)
    {
        std::promise<ReturnType> promise;
        std::future<ReturnType> result = promise.get_future();

        dispatch(
            [promise = std::move(promise), func = std::forward<Callable>(func)]() mutable {
                try {
                    promise.set_value(func());
                } catch (...) {
                    promise.set_exception(std::current_exception());
                }
            },
            mode);
        return result;
    }

    template <typename Callable, typename ReturnType = std::invoke_result_t<Callable>>
    ReturnType dispatchAndWait(Callable&& func, ExecuteImmediately mode = ExecuteImmediately::IfOnThread)
        requires(!std::is_same_v<ReturnType, void>)
    {
        return waitFuture(dispatch(std::move(func), mode));
    }

    /**
     * @brief Waits until all tasks in the queue are processed.
     *
     * This method ensures that the task queue is completely processed before
     * returning.
     *
     * @threadsafe This method is thread-safe and can be called from any thread.
     */
    void waitForCompletion();

    /**
     * @brief Creates a future that will be satisfied when all tasks in the queue
     * are processed.
     *
     * This method returns a `std::future<void>` that can be used to wait for the completion of
     * all queued tasks.
     *
     * @return A future that will be satisfied once all tasks in the queue are processed.
     * @threadsafe This method is thread-safe and can be called from any thread.
     */
    std::future<void> completionFuture();
};

/**
 * @brief Class that manages a queue of tasks to be executed.
 *
 * The TaskQueue class allows functions to be dispatched for immediate execution
 * or queued for later execution, depending on the dispatch mode and the current thread context.
 */
class TaskQueue : public Scheduler {
public:
    /**
     * @brief Constructs a TaskQueue object.
     *
     * @note The thread that executes this constructor becomes the queue's default thread.
     */
    TaskQueue();

    ~TaskQueue();

    /**
     * @brief Dispatches a function for execution.
     *
     * This method schedules the specified function for execution. If the current thread
     * is the same as the queue's default thread and the dispatch mode permits immediate
     * execution, the function is executed immediately. Otherwise, it is added to the queue.
     *
     * @param func The function to be executed.
     * @param mode The mode that determines when the function will be executed.
     * @return A future that will be satisfied once the function has completed.
     * @threadsafe This method is thread-safe and can be called from any thread.
     */
    std::future<void> dispatch(VoidFunc func,
                               ExecuteImmediately mode = ExecuteImmediately::IfOnThread) noexcept override;

    /**
     * @brief Checks if the current thread is the queue's default thread.
     *
     * @return true if the current thread is the queue's default thread, false otherwise.
     * @threadsafe This method is thread-safe and can be called from any thread.
     */
    bool isOnThread() const noexcept;

    /**
     * @brief Asserts that the current thread is the queue's default thread.
     *
     * This method checks if the calling thread is the same as the queue's
     * default thread. If the check fails, an assertion is triggered.
     * This ensures certain operations are performed on the correct thread.
     */
    void ensureOnThread() const noexcept {
        BRISK_ASSERT(isOnThread());
    }

    /**
     * @brief Checks if tasks are currently being processed.
     *
     * @return true if tasks are being processed, false otherwise.
     * @threadsafe This method is thread-safe and can be called from any thread.
     */
    bool isProcessing() const noexcept;

    /**
     * @brief Processes tasks in the queue.
     *
     * This method dequeues and executes tasks on the current thread. It ensures
     * that no other processing occurs during this operation.
     * The method will continue executing tasks until the queue is empty.
     */
    void process() noexcept;

    /**
     * @brief Gets the thread ID of the queue's default thread.
     *
     * @return The thread ID of the queue's default thread.
     * @threadsafe This method is thread-safe and can be called from any thread.
     */
    std::thread::id getThreadId() const noexcept;

protected:
    /**
     * @brief Enqueues a function to be processed later.
     *
     * @param func The function to be enqueued.
     * @threadsafe This method is thread-safe and can be called from any thread.
     */
    void enqueue(VoidFunc func) noexcept;

    /**
     * @brief Attempts to dequeue a function for processing.
     *
     * @param func Reference to store the dequeued function.
     * @return true if a function was dequeued, false otherwise.
     */
    bool tryDequeue(VoidFunc& func) noexcept;

    struct Impl;

    std::thread::id m_threadId;
    int m_processing = 0;
    std::unique_ptr<Impl> m_impl;
};

/// @brief Represents the task queue and scheduler for the main thread.
extern RC<TaskQueue> mainScheduler;

/// @brief Pointer to the scheduler associated with the current thread. If no scheduler has been assigned,
/// this pointer may be nullptr.
extern thread_local Scheduler* threadScheduler;

template <typename T>
T waitFuture(VoidFunc waitFunc, std::future<T> future, int intervalMS) {
    if (waitFunc || isMainThread()) {
        for (;;) {
            std::future_status status = future.wait_for(std::chrono::milliseconds(intervalMS));
            if (status != std::future_status::timeout)
                return future.get();
            if (waitFunc)
                waitFunc();
            if (isMainThread()) {
                mainScheduler->process();
            }
        }
    } else {
        return future.get();
    }
}

template <typename T>
T waitFuture(std::future<T> future, int intervalMS) {
    return waitFuture(nullptr, std::move(future), intervalMS);
}

template <typename... Args>
struct DeferredCallback {
    function<void(Args...)> func;
    RC<Scheduler> scheduler;

    bool operator()(Args... args) const {
        if (func) {
            if constexpr (sizeof...(Args) == 0) {
                scheduler->dispatch(func);
            } else {
                scheduler->dispatch([=, this]() BRISK_INLINE_LAMBDA {
                    func(args...);
                });
            }
            return true;
        }
        return false;
    }
};

template <typename... Args>
struct DeferredCallbacks : public std::vector<DeferredCallback<Args...>> {
public:
    DeferredCallbacks& operator+=(DeferredCallback<Args...> cb) {
        std::vector<DeferredCallback<Args...>>::push_back(std::move(cb));
        return *this;
    }

    void operator()(Args... args) const {
        for (const DeferredCallback<Args...>& cb : *this) {
            cb(args...);
        }
    }
};

/**
 * @brief Sets the name of the current thread.
 *
 * This function sets the name of the calling thread. The name is useful for debugging
 * and profiling tools that display thread names in their interfaces. Thread names
 * are typically limited to a small number of characters, depending on the platform.
 *
 * @param name A string view representing the desired name of the thread.
 */
void setThreadName(std::string_view name);

/**
 * @brief Defines thread priority levels.
 *
 * This enumeration represents the various levels of thread priority, which can
 * be used to influence the scheduling of threads. Higher priorities may result
 * in the thread receiving more CPU time or being scheduled sooner.
 */
enum class ThreadPriority {
    Lowest,  ///< The lowest possible thread priority.
    Low,     ///< A low thread priority.
    Normal,  ///< The default thread priority.
    High,    ///< A higher-than-normal thread priority.
    Highest, ///< The highest possible thread priority.
};

/**
 * @brief Sets the priority of the current thread.
 *
 * This function sets the scheduling priority of the calling thread. The effect
 * of this may vary depending on the operating system and platform. Higher
 * priority threads may receive more CPU time than lower priority ones.
 *
 * @param priority The desired thread priority level, from the ThreadPriority enumeration.
 */
void setThreadPriority(ThreadPriority priority);

namespace Internal {

template <typename T>
struct AsyncCallback {
    std::mutex sync;
    std::variant<std::monostate, T, std::exception_ptr> result;
    function<void(T)> fn_ready;
    function<void(std::exception_ptr)> fn_exception;

    void ready(T value) {
        std::lock_guard lk(sync);
        if (fn_ready) {
            fn_ready(value);
            fn_ready = nullptr; // free memory
        } else {
            result = std::move(value);
        }
    }

    void exception(std::exception_ptr exc) {
        std::lock_guard lk(sync);
        if (fn_exception) {
            fn_exception(exc);
            fn_exception = nullptr; // free memory
        } else {
            result = std::move(exc);
        }
    }

    void onReady(function<void(T)> fn) {
        std::lock_guard lk(sync);
        if (result.index() == 1) {
            fn(std::get<1>(result));
            result = std::monostate{};
        } else {
            fn_ready = std::move(fn);
        }
    }

    void onException(function<void(std::exception_ptr)> fn) {
        std::lock_guard lk(sync);
        if (result.index() == 2) {
            fn(std::get<2>(result));
            result = std::monostate{};
        } else {
            fn_exception = std::move(fn);
        }
    }
};
} // namespace Internal

template <typename Result>
struct AsyncOperation;

template <typename Result>
struct AsyncValue {
public:
    AsyncValue(const AsyncValue&) noexcept            = default;
    AsyncValue(AsyncValue&&) noexcept                 = default;
    AsyncValue& operator=(const AsyncValue&) noexcept = default;
    AsyncValue& operator=(AsyncValue&&) noexcept      = default;

    Result getSync() {
        RC<std::promise<Result>> promise = std::make_shared<std::promise<Result>>();
        std::future<Result> future       = promise->get_future();
        cb->onReady([promise](Result result) {
            promise->set_value(std::move(result));
        });
        cb->onException([promise](std::exception_ptr exc) {
            promise->set_exception(std::move(exc));
        });
        return future.get();
    }

    void wait() {
        std::ignore = getSync();
    }

    void getInCallback(RC<Scheduler> scheduler, function<void(Result)> callback,
                       function<void(std::exception_ptr)> error = {}) {
        cb->onReady([scheduler, callback = std::move(callback)](Result result) mutable {
            scheduler->dispatch([callback = std::move(callback), result = std::move(result)]() mutable {
                callback(result);
            });
        });
        if (error) {
            cb->onException([scheduler, error = std::move(error)](std::exception_ptr exc) mutable {
                scheduler->dispatch([error = std::move(error), exc = std::move(exc)]() mutable {
                    error(exc);
                });
            });
        }
    }

private:
    friend struct AsyncOperation<Result>;

    AsyncValue(RC<Internal::AsyncCallback<Result>> cb) : cb(std::move(cb)) {}

    RC<Internal::AsyncCallback<Result>> cb;
};

template <typename Result>
struct AsyncOperation {
    AsyncOperation() : cb(std::make_shared<Internal::AsyncCallback<Result>>()) {}

    void ready(Result result) {
        cb->ready(std::move(result));
    }

    void exception(std::exception_ptr exc) {
        cb->exception(std::move(exc));
    }

    template <typename Fn>
    void execute(Fn&& fn) {
        try {
            ready(fn());
        } catch (...) {
            exception(std::current_exception());
        }
    }

    AsyncValue<Result> value() {
        return { cb };
    }

private:
    RC<Internal::AsyncCallback<Result>> cb;
};

} // namespace Brisk
