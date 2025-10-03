#include "Threads.h"

namespace Blaze {

Threads::Threads() {}

Threads::~Threads() {
    // Note: Workers loop forever in current design.
    // If you want clean shutdown, you’d need a stop flag and join().
}

int Threads::GetHardwareThreadCount() {
#ifdef MULTITHREAD
    return Max(static_cast<int>(std::thread::hardware_concurrency()), 1);
#else
    return 1;
#endif
}

void Threads::Run(const int count, Function *loopBody) {
#ifdef MULTITHREAD
    BLAZE_ASSERT(loopBody != nullptr);

    if (count < 1)
        return;

    if (count == 1) {
        loopBody->Execute(0, mMainMemory);
        return;
    }

    mTaskData->Cursor = 0;
    mTaskData->Count = count;
    mTaskData->Fn = loopBody;

    const int threadCount = Min(mThreadCount, count);

    mTaskData->RequiredWorkerCount = threadCount;
    mTaskData->FinalizedWorkers = 0;

    {
        std::unique_lock<std::mutex> lock(mTaskData->FinalizationMutex);

        // Wake all threads waiting on this condition variable.
        mTaskData->CV.notify_all();

        while (mTaskData->FinalizedWorkers < threadCount) {
            mTaskData->FinalizationCV.wait(lock);
        }
    }

    // Cleanup.
    mTaskData->Cursor = 0;
    mTaskData->Count = 0;
    mTaskData->Fn = nullptr;
    mTaskData->RequiredWorkerCount = 0;
    mTaskData->FinalizedWorkers = 0;

#endif
}

void Threads::ResetFrameMemory() {
    for (auto td : mThreadData) {
        td->Memory.ResetFrameMemory();
    }
    mMainMemory.ResetFrameMemory();
}

void Threads::RunThreads() {
#ifdef MULTITHREAD
    if (mTaskData != nullptr) {
        return;
    }

    mTaskData = new TaskList();
    mThreadCount = Min(GetHardwareThreadCount(), 128);
    mThreadData.resize(mThreadCount);

    for (int i = 0; i < mThreadCount; i++) {
        mThreadData[i] = new ThreadData(mTaskData);
    }

    for (int i = 0; i < mThreadCount; i++) {
        ThreadData *d = mThreadData[i];
        d->Thread = std::thread(&Threads::Worker, d);
        d->Thread.detach(); // same as pthread_create + no join
    }
#endif
}

void Threads::Worker(ThreadData *d) {
#ifdef MULTITHREAD
    BLAZE_ASSERT(d != nullptr);

    TaskList *items = d->Tasks;

    for (;;) {
        {
            std::unique_lock<std::mutex> lock(items->Mutex);
            while (items->RequiredWorkerCount < 1) {
                items->CV.wait(lock);
            }
            items->RequiredWorkerCount--;
        }

        const int count = items->Count;

        for (;;) {
            int index = items->Cursor++;
            if (index >= count)
                break;
            items->Fn->Execute(index, d->Memory);
        }

        {
            std::lock_guard<std::mutex> lock(items->FinalizationMutex);
            items->FinalizedWorkers++;
        }

        items->FinalizationCV.notify_one();
    }
#endif
}

} // namespace Blaze