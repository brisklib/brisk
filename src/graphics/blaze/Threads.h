#pragma once

#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <vector>
#include "ThreadMemory.h"
#include "Utils.h"

namespace Blaze {

/**
 * Manages a pool of threads used for parallelization of rasterization tasks.
 */
class Threads final {
public:
    Threads();
    ~Threads();

public:
    static int GetHardwareThreadCount();

public:
    template <typename F>
    void ParallelFor(const int count, const F loopBody);

    void *MallocMain(const int size);

    ThreadMemory &MainMemory() {
        return mMainMemory;
    }

    template <typename T>
    T *MallocMain();

    template <typename T, typename... Args>
    T *NewMain(Args &&...args);

    void ResetFrameMemory();
    void RunThreads();

private:
    struct Function {
        virtual ~Function() {}

        virtual void Execute(const int index, ThreadMemory &memory) = 0;
    };

    template <typename T>
    struct Fun : public Function {
        explicit Fun(const T &lambda) : Lambda(lambda) {}

        void Execute(const int index, ThreadMemory &memory) override {
            Lambda(index, memory);
        }

        T Lambda;
    };

    struct TaskList final {
        std::atomic<int> Cursor{ 0 };
        int Count = 0;
        Function *Fn = nullptr;

        std::mutex Mutex;
        std::condition_variable CV;
        int RequiredWorkerCount = 0;

        std::mutex FinalizationMutex;
        std::condition_variable FinalizationCV;
        int FinalizedWorkers = 0;
    };

    struct ThreadData final {
        ThreadData(TaskList *tasks) : Tasks(tasks) {}

        ThreadMemory Memory;
        TaskList *Tasks = nullptr;
        std::thread Thread;
    };

    TaskList *mTaskData = nullptr;
    std::vector<ThreadData *> mThreadData;
    int mThreadCount = 0;
    ThreadMemory mMainMemory;

private:
    void Run(const int count, Function *loopBody);
    static void Worker(ThreadData *d);

private:
    BLAZE_DISABLE_COPY_AND_ASSIGN(Threads);
};

template <typename F>
inline void Threads::ParallelFor(const int count, const F loopBody) {
#ifdef MULTITHREAD
    RunThreads();

    const int run = Max(Min(64, count / (mThreadCount * 32)), 1);

    if (run == 1) {
        Fun p([&loopBody](const int index, ThreadMemory &memory) {
            loopBody(index, memory);
            memory.ResetTaskMemory();
        });

        Run(count, &p);
    } else {
        const int iterationCount = (count / run) + Min(count % run, 1);

        Fun p([run, count, &loopBody](const int index, ThreadMemory &memory) {
            const int idx = run * index;
            const int maxidx = Min(count, idx + run);

            for (int i = idx; i < maxidx; i++) {
                loopBody(i, memory);
                memory.ResetTaskMemory();
            }
        });

        Run(iterationCount, &p);
    }
#else
    for (int i = 0; i < count; i++) {
        loopBody(i, mMainMemory);
        mMainMemory.ResetTaskMemory();
    }
#endif
}

inline void *Threads::MallocMain(const int size) {
    return mMainMemory.FrameMalloc(size);
}

template <typename T>
inline T *Threads::MallocMain() {
    return mMainMemory.FrameMalloc<T>();
}

template <typename T, typename... Args>
inline T *Threads::NewMain(Args &&...args) {
    return new (MallocMain<T>()) T(std::forward<Args>(args)...);
}

} // namespace Blaze