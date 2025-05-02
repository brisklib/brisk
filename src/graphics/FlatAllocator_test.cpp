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
#include <catch2/catch_all.hpp>
#include "Catch2Utils.hpp"
#include "FlatAllocator.hpp"
#include <random>

namespace Catch {
template <>
struct StringMaker<Brisk::FlatAllocatorStat> {
    static std::string convert(const Brisk::FlatAllocatorStat& stat) {
        return fmt::format("({}, {}, {})", stat.totalFreeSpace, stat.largestFreeBlock, stat.numFreeBlocks);
    }
};
} // namespace Catch

namespace Brisk {

TEST_CASE("FlatAllocator") {
    using Allocator = FlatAllocator<uint32_t, 1>;
    Allocator alloc(4096);
    CHECK(alloc.stat() == FlatAllocatorStat{ 4096, 4096, 4096, 1 });
    Allocator::offset_type p = alloc.allocate(77);
    CHECK(p == 0);
    CHECK(alloc.stat() == FlatAllocatorStat{ 4096, 4096 - 77, 4096 - 77, 1 });
    alloc.free(p, 77);
    CHECK(alloc.stat() == FlatAllocatorStat{ 4096, 4096, 4096, 1 });

    Allocator::offset_type p1 = alloc.allocate(12);
    CHECK(alloc.stat() == FlatAllocatorStat{ 4096, 4096 - 12, 4096 - 12, 1 });
    Allocator::offset_type p2 = alloc.allocate(123);
    CHECK(alloc.stat() == FlatAllocatorStat{ 4096, 4096 - 12 - 123, 4096 - 12 - 123, 1 });
    Allocator::offset_type p3 = alloc.allocate(1234);
    CHECK(alloc.stat() == FlatAllocatorStat{ 4096, 4096 - 12 - 123 - 1234, 4096 - 12 - 123 - 1234, 1 });
    alloc.free(p1, 12);
    CHECK(alloc.stat() == FlatAllocatorStat{ 4096, 4096 - 123 - 1234, 4096 - 12 - 123 - 1234, 2 });
    alloc.free(p2, 123);
    CHECK(alloc.stat() == FlatAllocatorStat{ 4096, 4096 - 1234, 4096 - 12 - 123 - 1234, 2 });
    alloc.free(p3, 1234);
    CHECK(alloc.stat() == FlatAllocatorStat{ 4096, 4096, 4096, 1 });
}

TEST_CASE("FlatAllocator(16)") {
    constexpr uint32_t alignment = 16;
#define A(x) alignUp(x, alignment)
    using Allocator = FlatAllocator<uint32_t, alignment>;
    Allocator alloc(4096);
    CHECK(alloc.stat() == FlatAllocatorStat{ 4096, 4096, 4096, 1 });
    Allocator::offset_type p = alloc.allocate(77);
    CHECK(p == 0);
    CHECK(alloc.stat() == FlatAllocatorStat{ 4096, 4096 - A(77), 4096 - A(77), 1 });
    alloc.free(p, 77);
    CHECK(alloc.stat() == FlatAllocatorStat{ 4096, 4096, 4096, 1 });

    Allocator::offset_type p1 = alloc.allocate(12);
    CHECK(alloc.stat() == FlatAllocatorStat{ 4096, 4096 - A(12), 4096 - A(12), 1 });
    Allocator::offset_type p2 = alloc.allocate(123);
    CHECK(alloc.stat() == FlatAllocatorStat{ 4096, 4096 - A(12) - A(123), 4096 - A(12) - A(123), 1 });
    Allocator::offset_type p3 = alloc.allocate(1234);
    CHECK(alloc.stat() ==
          FlatAllocatorStat{ 4096, 4096 - A(12) - A(123) - A(1234), 4096 - A(12) - A(123) - A(1234), 1 });
    alloc.free(p1, 12);
    CHECK(alloc.stat() ==
          FlatAllocatorStat{ 4096, 4096 - A(123) - A(1234), 4096 - A(12) - A(123) - A(1234), 2 });
    alloc.free(p2, 123);
    CHECK(alloc.stat() == FlatAllocatorStat{ 4096, 4096 - A(1234), 4096 - A(12) - A(123) - A(1234), 2 });
    alloc.free(p3, 1234);
    CHECK(alloc.stat() == FlatAllocatorStat{ 4096, 4096, 4096, 1 });
#undef A
}

TEST_CASE("FlatAllocator(stress-test)") {
    using Allocator = FlatAllocator<>;
    Allocator alloc(1048576);

    struct Block {
        Allocator::offset_type ptr;
        Allocator::size_type size;
    };

    std::vector<Block> blocks;
    std::mt19937 rnd(12345);
    auto stat = alloc.stat();
    while (stat.totalFreeSpace > 0) {
        Allocator::size_type sz  = stat.totalFreeSpace > 1 ? (rnd() % (stat.totalFreeSpace - 1) + 1) : 1;
        Allocator::offset_type p = alloc.allocate(sz);
        if (p != alloc.null()) {
            blocks.push_back(Block{ p, sz });
        }
        stat = alloc.stat();
    }
    CHECK(alloc.stat() == FlatAllocatorStat{ 1048576, 0, 0, 0 });

    std::shuffle(blocks.begin(), blocks.end(), rnd);
    for (auto b : blocks) {
        alloc.free(b.ptr, b.size);
    }
    CHECK(alloc.stat() == FlatAllocatorStat{ alloc.totalSize(), alloc.totalSize(), alloc.totalSize(), 1 });
}

} // namespace Brisk
