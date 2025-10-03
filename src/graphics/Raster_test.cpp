#include "brisk/graphics/Path.hpp"
#include "Catch2Utils.hpp"
#include <random>

#include "Mask.hpp"

template <>
struct fmt::formatter<Brisk::Internal::Patch> : fmt::formatter<std::string> {
    template <typename FormatContext>
    auto format(const Brisk::Internal::Patch& value, FormatContext& ctx) const {
        return fmt::formatter<std::string>::format(
            fmt::format("{{ x={}, y={}, len={}, {} }}", value.x(), value.y(), value.len(), value.offset),
            ctx);
    }
};

template <>
struct fmt::formatter<Brisk::Internal::PatchData> : fmt::formatter<std::string> {
    template <typename FormatContext>
    auto format(const Brisk::Internal::PatchData& value, FormatContext& ctx) const {
        return fmt::formatter<std::string>::format(
            fmt::format("{:02X}{:02X}{:02X}{:02X}\n{:02X}{:02X}{:02X}{:02X}\n{:02X}{:02X}{:02X}{:02X}\n{:02X}"
                        "{:02X}{:02X}{:02X}", //
                        value.data_u8[0], value.data_u8[1], value.data_u8[2], value.data_u8[3],
                        value.data_u8[4], value.data_u8[5], value.data_u8[6], value.data_u8[7],
                        value.data_u8[8], value.data_u8[9], value.data_u8[10], value.data_u8[11],
                        value.data_u8[12], value.data_u8[13], value.data_u8[14], value.data_u8[15]),
            ctx);
    }
};

namespace Brisk {

struct PreparedPath2 : public PreparedPath {
    using PreparedPath::isRectangle;
    using PreparedPath::isSparse;
    using PreparedPath::patchBounds;
    using PreparedPath::patchData;
    using PreparedPath::patches;
    using PreparedPath::PreparedPath;
    using PreparedPath::rectangle;
    using PreparedPath::toSparse;

    PreparedPath2(const PreparedPath& path) : PreparedPath(path) {}

    PreparedPath2& operator=(const PreparedPath& path) {
        PreparedPath::operator=(path);
        return *this;
    }
};

TEST_CASE("Rasterizer: Rasterize empty path") {
    Path path;
    PreparedPath2 prepared(path);
    CHECK(prepared.empty());
    CHECK(prepared.patchBounds() == Rectangle{});
}

TEST_CASE("Rasterizer: Rasterize empty path 2") {
    Path path;
    path.addRect({ 10, 10, 10, 50 });
    PreparedPath2 prepared(path);
    CHECK(prepared.empty());
    CHECK(prepared.patchBounds() == Rectangle{});
}

using Internal::Patch;
using Internal::PatchData;

TEST_CASE("Rasterizer: Rasterize path with rectangle") {
    Path path;
    path.addRect({ 0, 0, 4, 4 });
    PreparedPath2 prepared(path);
    CHECK(!prepared.empty());
    CHECK(prepared.isRectangle());
}

TEST_CASE("Rasterizer: Rasterize path with rectangle 2") {
    Path path;
    path.addRect({ 2, 2, 6, 6 });
    PreparedPath2 prepared(path);
    CHECK(!prepared.empty());
    CHECK(prepared.isRectangle());

    prepared = prepared.toSparse();

    CHECK(!prepared.empty());
    CHECK(prepared.patchBounds() == Rectangle{ 0, 0, 8, 8 });
    REQUIRE(prepared.patches().size() == 4);
    CHECK(prepared.patches()[0] == Patch{ 0, 0, 1, 0 });
    CHECK(prepared.patches()[1] == Patch{ 1, 0, 1, 1 });
    CHECK(prepared.patches()[2] == Patch{ 0, 1, 1, 2 });
    CHECK(prepared.patches()[3] == Patch{ 1, 1, 1, 3 });
    REQUIRE(prepared.patchData().size() == 4);
    CHECK(prepared.patchData()[0] == PatchData::fromBits(0b0000'0000'0011'0011));
    CHECK(prepared.patchData()[1] == PatchData::fromBits(0b0000'0000'1100'1100));
    CHECK(prepared.patchData()[2] == PatchData::fromBits(0b0011'0011'0000'0000));
    CHECK(prepared.patchData()[3] == PatchData::fromBits(0b1100'1100'0000'0000));
}

TEST_CASE("Rasterizer: Rle binary operations") {
    PreparedPath2 prepared1({ 0, 0, 2, 2 });
    PreparedPath2 prepared2({ 1, 1, 3, 3 });

    SECTION("And") {
        PreparedPath2 result = PreparedPath::pathOp(MaskOp::And, prepared1, prepared2);
        REQUIRE(result.isRectangle());
        CHECK(result.rectangle() == RectangleF{ 1, 1, 2, 2 });

        result = result.toSparse();

        REQUIRE(!result.empty());
        CHECK(result.patchBounds() == Rectangle(0, 0, 4, 4));
        REQUIRE(result.patches().size() == 1);
        CHECK(result.patches()[0] == Patch{ 0, 0, 1, 0 });
        REQUIRE(result.patchData().size() == 1);
        CHECK(result.patchData()[0] == PatchData::fromBits(0b0000'0100'0000'0000));
    }

    SECTION("And Not") {
        PreparedPath2 result = PreparedPath::pathOp(MaskOp::AndNot, prepared1, prepared2);
        REQUIRE(!result.empty());
        CHECK(result.patchBounds() == Rectangle(0, 0, 4, 4));
        REQUIRE(result.patches().size() == 1);
        CHECK(result.patches()[0] == Patch{ 0, 0, 1, 0 });
        REQUIRE(result.patchData().size() == 1);
        CHECK(result.patchData()[0] == PatchData::fromBits(0b1100'1000'0000'0000));
    }

    SECTION("Or") {
        PreparedPath2 result = PreparedPath::pathOp(MaskOp::Or, prepared1, prepared2);
        REQUIRE(!result.empty());
        CHECK(result.patchBounds() == Rectangle(0, 0, 4, 4));
        REQUIRE(result.patches().size() == 1);
        CHECK(result.patches()[0] == Patch{ 0, 0, 1, 0 });
        REQUIRE(result.patchData().size() == 1);
        CHECK(result.patchData()[0] == PatchData::fromBits(0b1100'1110'0110'0000));
    }

    SECTION("Xor") {
        PreparedPath2 result = PreparedPath::pathOp(MaskOp::Xor, prepared1, prepared2);
        REQUIRE(!result.empty());
        CHECK(result.patchBounds() == Rectangle(0, 0, 4, 4));
        REQUIRE(result.patches().size() == 1);
        CHECK(result.patches()[0] == Patch{ 0, 0, 1, 0 });
        REQUIRE(result.patchData().size() == 1);
        CHECK(result.patchData()[0] == PatchData::fromBits(0b1100'1010'0110'0000));
    }
}

TEST_CASE("Rasterizer: Rle binary operations 2") {
    PreparedPath2 rect1(RectangleF{ 0, 0, 20, 20 }, false);
    PreparedPath2 rect2(RectangleF{ 10, 10, 30, 30 }, false);

    SECTION("And") {
        PreparedPath2 result = PreparedPath::pathOp(MaskOp::And, rect1, rect2);

        REQUIRE(!result.empty());
        CHECK(result.patchBounds() == Rectangle(8, 8, 20, 20));
        REQUIRE(result.patches().size() == 6);
        CHECK(result.patches()[0] == Patch{ 2, 2, 1, 0 });
        CHECK(result.patches()[1] == Patch{ 3, 2, 2, 1 });
        CHECK(result.patches()[2] == Patch{ 2, 3, 1, 2 });
        CHECK(result.patches()[3] == Patch{ 3, 3, 2, 3 });
        CHECK(result.patches()[4] == Patch{ 2, 4, 1, 4 });
        CHECK(result.patches()[5] == Patch{ 3, 4, 2, 5 });
        REQUIRE(result.patchData().size() == 6);
        CHECK(result.patchData()[0] == PatchData::fromBits(0b0000'0000'0011'0011));
        CHECK(result.patchData()[1] == PatchData::fromBits(0b0000'0000'1111'1111));
        CHECK(result.patchData()[2] == PatchData::fromBits(0b0011'0011'0011'0011));
        CHECK(result.patchData()[3] == PatchData::fromBits(0b1111'1111'1111'1111));
        CHECK(result.patchData()[4] == PatchData::fromBits(0b0011'0011'0011'0011));
        CHECK(result.patchData()[5] == PatchData::fromBits(0b1111'1111'1111'1111));
    }

    SECTION("And Not") {
        PreparedPath2 result = PreparedPath::pathOp(MaskOp::AndNot, rect1, rect2);

        REQUIRE(result.patches().size() == 9);
        CHECK(result.patches()[0] == Patch{ 0, 0, 5, 0 });
        CHECK(result.patches()[1] == Patch{ 0, 1, 5, 1 });
        CHECK(result.patches()[2] == Patch{ 0, 2, 2, 2 });
        CHECK(result.patches()[3] == Patch{ 2, 2, 1, 3 });
        CHECK(result.patches()[4] == Patch{ 3, 2, 2, 4 });
        CHECK(result.patches()[5] == Patch{ 0, 3, 2, 5 });
        CHECK(result.patches()[6] == Patch{ 2, 3, 1, 6 });
        CHECK(result.patches()[7] == Patch{ 0, 4, 2, 7 });
        CHECK(result.patches()[8] == Patch{ 2, 4, 1, 8 });
        REQUIRE(result.patchData().size() == 9);
        CHECK(result.patchData()[0] == PatchData::fromBits(0b1111'1111'1111'1111));
        CHECK(result.patchData()[1] == PatchData::fromBits(0b1111'1111'1111'1111));
        CHECK(result.patchData()[2] == PatchData::fromBits(0b1111'1111'1111'1111));
        CHECK(result.patchData()[3] == PatchData::fromBits(0b1111'1111'1100'1100));
        CHECK(result.patchData()[4] == PatchData::fromBits(0b1111'1111'0000'0000));
        CHECK(result.patchData()[5] == PatchData::fromBits(0b1111'1111'1111'1111));
        CHECK(result.patchData()[6] == PatchData::fromBits(0b1100'1100'1100'1100));
        CHECK(result.patchData()[7] == PatchData::fromBits(0b1111'1111'1111'1111));
        CHECK(result.patchData()[8] == PatchData::fromBits(0b1100'1100'1100'1100));
    }

    SECTION("Or") {
        PreparedPath2 result = PreparedPath::pathOp(MaskOp::Or, rect1, rect2);

        REQUIRE(result.patches().size() == 18);
        CHECK(result.patches()[0] == Patch{ 0, 0, 5, 0 });
        CHECK(result.patches()[1] == Patch{ 0, 1, 5, 1 });
        CHECK(result.patches()[2] == Patch{ 0, 2, 5, 2 });
        CHECK(result.patches()[3] == Patch{ 5, 2, 2, 3 });
        CHECK(result.patches()[4] == Patch{ 7, 2, 1, 4 });
        CHECK(result.patches()[5] == Patch{ 0, 3, 7, 5 });
        CHECK(result.patches()[6] == Patch{ 7, 3, 1, 6 });
        CHECK(result.patches()[7] == Patch{ 0, 4, 7, 7 });
        CHECK(result.patches()[8] == Patch{ 7, 4, 1, 8 });
        CHECK(result.patches()[9] == Patch{ 2, 5, 1, 9 });
        CHECK(result.patches()[10] == Patch{ 3, 5, 4, 10 });
        CHECK(result.patches()[11] == Patch{ 7, 5, 1, 11 });
        CHECK(result.patches()[12] == Patch{ 2, 6, 1, 12 });
        CHECK(result.patches()[13] == Patch{ 3, 6, 4, 13 });
        CHECK(result.patches()[14] == Patch{ 7, 6, 1, 14 });
        CHECK(result.patches()[15] == Patch{ 2, 7, 1, 15 });
        CHECK(result.patches()[16] == Patch{ 3, 7, 4, 16 });
        CHECK(result.patches()[17] == Patch{ 7, 7, 1, 17 });
        REQUIRE(result.patchData().size() == 18);
        CHECK(result.patchData()[0] == PatchData::fromBits(0b1111'1111'1111'1111));
        CHECK(result.patchData()[1] == PatchData::fromBits(0b1111'1111'1111'1111));
        CHECK(result.patchData()[2] == PatchData::fromBits(0b1111'1111'1111'1111));
        CHECK(result.patchData()[3] == PatchData::fromBits(0b0000'0000'1111'1111));
        CHECK(result.patchData()[4] == PatchData::fromBits(0b0000'0000'1100'1100));
        CHECK(result.patchData()[5] == PatchData::fromBits(0b1111'1111'1111'1111));
        CHECK(result.patchData()[6] == PatchData::fromBits(0b1100'1100'1100'1100));
        CHECK(result.patchData()[7] == PatchData::fromBits(0b1111'1111'1111'1111));
        CHECK(result.patchData()[8] == PatchData::fromBits(0b1100'1100'1100'1100));
        CHECK(result.patchData()[9] == PatchData::fromBits(0b0011'0011'0011'0011));
        CHECK(result.patchData()[10] == PatchData::fromBits(0b1111'1111'1111'1111));
        CHECK(result.patchData()[11] == PatchData::fromBits(0b1100'1100'1100'1100));
        CHECK(result.patchData()[12] == PatchData::fromBits(0b0011'0011'0011'0011));
        CHECK(result.patchData()[13] == PatchData::fromBits(0b1111'1111'1111'1111));
        CHECK(result.patchData()[14] == PatchData::fromBits(0b1100'1100'1100'1100));
        CHECK(result.patchData()[15] == PatchData::fromBits(0b0011'0011'0000'0000));
        CHECK(result.patchData()[16] == PatchData::fromBits(0b1111'1111'0000'0000));
        CHECK(result.patchData()[17] == PatchData::fromBits(0b1100'1100'0000'0000));
    }
    SECTION("Xor") {
        PreparedPath2 result = PreparedPath::pathOp(MaskOp::Xor, rect1, rect2);

        REQUIRE(result.patches().size() == 24);
        CHECK(result.patches()[0] == Patch{ 0, 0, 5, 0 });
        CHECK(result.patches()[1] == Patch{ 0, 1, 5, 1 });
        CHECK(result.patches()[2] == Patch{ 0, 2, 2, 2 });
        CHECK(result.patches()[3] == Patch{ 2, 2, 1, 3 });
        CHECK(result.patches()[4] == Patch{ 3, 2, 2, 4 });
        CHECK(result.patches()[5] == Patch{ 5, 2, 2, 5 });
        CHECK(result.patches()[6] == Patch{ 7, 2, 1, 6 });
        CHECK(result.patches()[7] == Patch{ 0, 3, 2, 7 });
        CHECK(result.patches()[8] == Patch{ 2, 3, 1, 8 });
        CHECK(result.patches()[9] == Patch{ 5, 3, 2, 9 });
        CHECK(result.patches()[10] == Patch{ 7, 3, 1, 10 });
        CHECK(result.patches()[11] == Patch{ 0, 4, 2, 11 });
        CHECK(result.patches()[12] == Patch{ 2, 4, 1, 12 });
        CHECK(result.patches()[13] == Patch{ 5, 4, 2, 13 });
        CHECK(result.patches()[14] == Patch{ 7, 4, 1, 14 });
        CHECK(result.patches()[15] == Patch{ 2, 5, 1, 15 });
        CHECK(result.patches()[16] == Patch{ 3, 5, 4, 16 });
        CHECK(result.patches()[17] == Patch{ 7, 5, 1, 17 });
        CHECK(result.patches()[18] == Patch{ 2, 6, 1, 18 });
        CHECK(result.patches()[19] == Patch{ 3, 6, 4, 19 });
        CHECK(result.patches()[20] == Patch{ 7, 6, 1, 20 });
        CHECK(result.patches()[21] == Patch{ 2, 7, 1, 21 });
        CHECK(result.patches()[22] == Patch{ 3, 7, 4, 22 });
        CHECK(result.patches()[23] == Patch{ 7, 7, 1, 23 });

        REQUIRE(result.patchData().size() == 24);
        CHECK(result.patchData()[0] == PatchData::fromBits(0b1111'1111'1111'1111));
        CHECK(result.patchData()[1] == PatchData::fromBits(0b1111'1111'1111'1111));
        CHECK(result.patchData()[2] == PatchData::fromBits(0b1111'1111'1111'1111));
        CHECK(result.patchData()[3] == PatchData::fromBits(0b1111'1111'1100'1100));
        CHECK(result.patchData()[4] == PatchData::fromBits(0b1111'1111'0000'0000));
        CHECK(result.patchData()[5] == PatchData::fromBits(0b0000'0000'1111'1111));
        CHECK(result.patchData()[6] == PatchData::fromBits(0b0000'0000'1100'1100));
        CHECK(result.patchData()[7] == PatchData::fromBits(0b1111'1111'1111'1111));
        CHECK(result.patchData()[8] == PatchData::fromBits(0b1100'1100'1100'1100));
        CHECK(result.patchData()[9] == PatchData::fromBits(0b1111'1111'1111'1111));
        CHECK(result.patchData()[10] == PatchData::fromBits(0b1100'1100'1100'1100));
        CHECK(result.patchData()[11] == PatchData::fromBits(0b1111'1111'1111'1111));
        CHECK(result.patchData()[12] == PatchData::fromBits(0b1100'1100'1100'1100));
        CHECK(result.patchData()[13] == PatchData::fromBits(0b1111'1111'1111'1111));
        CHECK(result.patchData()[14] == PatchData::fromBits(0b1100'1100'1100'1100));
        CHECK(result.patchData()[15] == PatchData::fromBits(0b0011'0011'0011'0011));
        CHECK(result.patchData()[16] == PatchData::fromBits(0b1111'1111'1111'1111));
        CHECK(result.patchData()[17] == PatchData::fromBits(0b1100'1100'1100'1100));
        CHECK(result.patchData()[18] == PatchData::fromBits(0b0011'0011'0011'0011));
        CHECK(result.patchData()[19] == PatchData::fromBits(0b1111'1111'1111'1111));
        CHECK(result.patchData()[20] == PatchData::fromBits(0b1100'1100'1100'1100));
        CHECK(result.patchData()[21] == PatchData::fromBits(0b0011'0011'0000'0000));
        CHECK(result.patchData()[22] == PatchData::fromBits(0b1111'1111'0000'0000));
        CHECK(result.patchData()[23] == PatchData::fromBits(0b1100'1100'0000'0000));
    }
}

} // namespace Brisk
