
#include "LineBlockAllocator.h"


namespace Blaze {

LineBlockAllocator::~LineBlockAllocator() {
    Arena *p = mAllArenas;

    while (p != nullptr) {
        Arena *next = p->Links.NextAll;

        free(p);

        p = next;
    }
}


void LineBlockAllocator::Clear() {
    Arena *l = nullptr;

    Arena *p = mAllArenas;

    while (p != nullptr) {
        Arena *next = p->Links.NextAll;

        p->Links.NextFree = l;

        l = p;

        p = next;
    }

    mCurrent = nullptr;
    mEnd = nullptr;
    mFreeArenas = l;
}


void LineBlockAllocator::NewArena() {
    Arena *p = mFreeArenas;

    if (p != nullptr) {
        mFreeArenas = p->Links.NextFree;
    } else {
        p = static_cast<Arena *>(malloc(sizeof(Arena)));

        p->Links.NextAll = mAllArenas;

        mAllArenas = p;
    }

    p->Links.NextFree = nullptr;

    mCurrent = p->Memory + sizeof(Arena::Links);
    mEnd = p->Memory + Arena::Size -
           Max3(sizeof(LineArrayX32Y16Block), sizeof(LineArrayX16Y16Block),
               sizeof(LineArrayTiledBlock));
}

} // namespace Blaze