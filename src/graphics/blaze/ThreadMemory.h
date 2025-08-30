
#pragma once


#include "BumpAllocator.h"
#include "LineBlockAllocator.h"


namespace Blaze {

/**
 * Maintains per-thread memory.
 *
 * Each thread gets a separate instance of thread memory. There
 * are two types of per-thread memory. They are different in
 * terms of how long the allocations stay valid.
 *
 * Frame memory.
 *
 * Frame memory is allocated any time during frame and
 * released after frame ends. Once something is allocated from
 * frame memory, this allocation stays valid until frame is
 * complete.
 *
 *
 * Task memory.
 *
 * Task memory is allocated during a single task and released
 * once task ends. Allocations made from task memory become
 * invalid once thread finishes executing a current task and
 * another task picked up by the same thread will be able to
 * make allocations from the same memory region, potentially
 * overwriting contents written by previous task. Task memory
 * is suited for temporary objects that are required during
 * execution of a task and can be discarded once it finishes.
 *
 * All methods in this class indicate which memory type they
 * operate on.
 *
 * The system automatically releases both frame and task
 * memory at appropriate times.
 */
class ThreadMemory final {
public:
    ThreadMemory() {}

public:
    void *TaskMalloc(const int size);


    /**
     * Allocates memory for one element of type T. Does not zero-fill
     * allocated memory and does not call any constructors.
     *
     * This method allocates from frame memory.
     */
    template <typename T>
    T *TaskMalloc();


    /**
     * Allocates memory for one element of type T. Does not zero-fill
     * allocated memory and does not call any constructors.
     *
     * This method allocates from frame memory.
     */
    template <typename T>
    T *FrameMalloc();


    /**
     * Allocates memory for a given amount of pointers of type T. Does not
     * zero-fill allocated memory. Note that this method does not allocate
     * any objects, only arrays of pointers.
     *
     * This method allocates from task memory.
     *
     * @param count A number of pointers to allocate. Must be at least 1.
     */
    template <typename T>
    T **TaskMallocPointers(const int count);


    /**
     * Allocates memory for a given amount of pointers of type T. Does not
     * zero-fill allocated memory. Note that this method does not allocate
     * any objects, only arrays of pointers.
     *
     * This method allocates from frame memory.
     *
     * @param count A number of pointers to allocate. Must be at least 1.
     */
    template <typename T>
    T **FrameMallocPointers(const int count);


    /**
     * Allocates memory for a given amount of pointers of type T and fills the
     * entire block of allocated memory with zeroes. Note that this method
     * does not allocate any objects, only arrays of pointers.
     *
     * This method allocates from frame memory.
     *
     * @param count A number of pointers to allocate. Must be at least 1.
     */
    template <typename T>
    T **FrameMallocPointersZeroFill(const int count);


    /**
     * Allocates memory for an array of values of type T. Does not zero-fill
     * allocated memory and does not call any constructors.
     *
     * This method allocates from frame memory.
     *
     * @param count A number of pointers to allocate. Must be at least 1.
     */
    template <typename T>
    T *FrameMallocArray(const int count);


    /**
     * Allocates memory for an array of values of type T. Fills allocated
     * memory with zeroes, but does not call any constructors.
     *
     * This method allocates from task memory.
     *
     * @param count A number of elements to allocate. Must be at least 1.
     */
    template <typename T>
    T *TaskMallocArrayZeroFill(const int count);


    /**
     * Allocates memory for an array of values of type T. Fills allocated
     * memory with zeroes, but does not call any constructors.
     *
     * This method allocates from frame memory.
     *
     * @param count A number of pointers to allocate. Must be at least 1.
     */
    template <typename T>
    T *FrameMallocArrayZeroFill(const int count);


    /**
     * Allocates memory for one element of type T and calls constructor with
     * given parameters. Returns fully constructed object of type T.
     *
     * This method allocates from task memory.
     */
    template <typename T, typename... Args>
    T *TaskNew(Args &&...args);


    /**
     * Allocates memory for one element of type T and calls constructor with
     * given parameters. Returns fully constructed object of type T.
     *
     * This method allocates from frame memory.
     */
    template <typename T, typename... Args>
    T *FrameNew(Args &&...args);


    /**
     * Allocates given amount of bytes. Does not zero-fill allocated memory.
     *
     * This method allocates from frame memory.
     *
     * @param size A number of bytes to allocate. Must be at least 1.
     */
    void *FrameMalloc(const int size);


    /**
     * Returns new tiled line array block. Returned memory is not zero-filled.
     *
     * Line blocks are always allocated from frame memory.
     */
    LineArrayTiledBlock *FrameNewTiledBlock(LineArrayTiledBlock *next);


    /**
     * Returns new narrow line array block. Returned memory is not
     * zero-filled.
     *
     * Line blocks are always allocated from frame memory.
     */
    LineArrayX16Y16Block *FrameNewX16Y16Block(LineArrayX16Y16Block *next);


    /**
     * Returns new wide line array block. Returned memory is
     * not zero-filled.
     *
     * Line blocks are always allocated from frame memory.
     */
    LineArrayX32Y16Block *FrameNewX32Y16Block(LineArrayX32Y16Block *next);


    /**
     * Resets frame memory. All allocations made during frame
     * by thread this memory belongs to will become invalid once
     * this method returns.
     */
    void ResetFrameMemory();


    /**
     * Resets task memory. All allocations made during execution
     * of a single task by a thread owning this memory will
     * become invalid once this method returns.
     *
     * This method is automatically called by Threads class
     * which manages execution of tasks, but task can call it as
     * well.
     */
    void ResetTaskMemory();

private:
    LineBlockAllocator mFrameLineBlockAllocator;
    BumpAllocator mFrameAllocator;
    BumpAllocator mTaskAllocator;

private:
    BLAZE_DISABLE_COPY_AND_ASSIGN(ThreadMemory);
};


inline void *ThreadMemory::TaskMalloc(const int size) {
    return mTaskAllocator.Malloc(size);
}


template <typename T>
inline T *ThreadMemory::TaskMalloc() {
    return mTaskAllocator.Malloc<T>();
}


template <typename T>
inline T *ThreadMemory::FrameMalloc() {
    return mFrameAllocator.Malloc<T>();
}


template <typename T>
inline T **ThreadMemory::TaskMallocPointers(const int count) {
    return mTaskAllocator.MallocPointers<T>(count);
}


template <typename T>
inline T **ThreadMemory::FrameMallocPointers(const int count) {
    return mFrameAllocator.MallocPointers<T>(count);
}


template <typename T>
inline T **ThreadMemory::FrameMallocPointersZeroFill(const int count) {
    return mFrameAllocator.MallocPointersZeroFill<T>(count);
}


template <typename T>
inline T *ThreadMemory::FrameMallocArray(const int count) {
    return mFrameAllocator.MallocArray<T>(count);
}


template <typename T>
inline T *ThreadMemory::TaskMallocArrayZeroFill(const int count) {
    return mTaskAllocator.MallocArrayZeroFill<T>(count);
}


template <typename T>
inline T *ThreadMemory::FrameMallocArrayZeroFill(const int count) {
    return mFrameAllocator.MallocArrayZeroFill<T>(count);
}


template <typename T, typename... Args>
inline T *ThreadMemory::TaskNew(Args &&...args) {
    return new (TaskMalloc<T>()) T(std::forward<Args>(args)...);
}


template <typename T, typename... Args>
inline T *ThreadMemory::FrameNew(Args &&...args) {
    return new (FrameMalloc<T>()) T(std::forward<Args>(args)...);
}


inline void *ThreadMemory::FrameMalloc(const int size) {
    return mFrameAllocator.Malloc(size);
}


inline LineArrayTiledBlock *ThreadMemory::FrameNewTiledBlock(
    LineArrayTiledBlock *next) {
    return mFrameLineBlockAllocator.NewTiledBlock(next);
}


inline LineArrayX16Y16Block *ThreadMemory::FrameNewX16Y16Block(
    LineArrayX16Y16Block *next) {
    return mFrameLineBlockAllocator.NewX16Y16Block(next);
}


inline LineArrayX32Y16Block *ThreadMemory::FrameNewX32Y16Block(
    LineArrayX32Y16Block *next) {
    return mFrameLineBlockAllocator.NewX32Y16Block(next);
}


inline void ThreadMemory::ResetFrameMemory() {
    mFrameLineBlockAllocator.Clear();
    mFrameAllocator.Free();
}


inline void ThreadMemory::ResetTaskMemory() {
    mTaskAllocator.Free();
}

} // namespace Blaze