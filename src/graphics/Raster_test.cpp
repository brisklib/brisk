#include "brisk/graphics/Path.hpp"
#include "Catch2Utils.hpp"

template <>
struct fmt::formatter<Brisk::Internal::Patch> : fmt::formatter<std::string> {
    template <typename FormatContext>
    auto format(const Brisk::Internal::Patch& value, FormatContext& ctx) const {
        return fmt::formatter<std::string>::format(
            fmt::format("{{ {}, {}, {} }}", value.x, value.y, value.offset), ctx);
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
    using PreparedPath::patchBounds;
    using PreparedPath::patchData;
    using PreparedPath::patches;
    using PreparedPath::PreparedPath;

    PreparedPath2(const PreparedPath& path) : PreparedPath(path) {}
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
    CHECK(prepared.patchBounds() == Rectangle{ 0, 0, 4, 4 });
    CHECK(prepared.patches().size() == 1);
    CHECK(prepared.patches()[0] == Patch{ 0, 0, 0 });
    CHECK(prepared.patchData().size() == 1);
    CHECK(prepared.patchData()[0] == PatchData::fromBits(0b1111'1111'1111'1111));
}

TEST_CASE("Rasterizer: Rasterize path with rectangle 2") {
    Path path;
    path.addRect({ 2, 2, 6, 6 });
    PreparedPath2 prepared(path);
    CHECK(!prepared.empty());
    CHECK(prepared.patchBounds() == Rectangle{ 0, 0, 8, 8 });
    REQUIRE(prepared.patches().size() == 4);
    CHECK(prepared.patches()[0] == Patch{ 0, 0, 0 });
    CHECK(prepared.patches()[1] == Patch{ 4, 0, 1 });
    CHECK(prepared.patches()[2] == Patch{ 0, 4, 2 });
    CHECK(prepared.patches()[3] == Patch{ 4, 4, 3 });
    REQUIRE(prepared.patchData().size() == 4);
    CHECK(prepared.patchData()[0] == PatchData::fromBits(0b0000'0000'0011'0011));
    CHECK(prepared.patchData()[1] == PatchData::fromBits(0b0000'0000'1100'1100));
    CHECK(prepared.patchData()[2] == PatchData::fromBits(0b0011'0011'0000'0000));
    CHECK(prepared.patchData()[3] == PatchData::fromBits(0b1100'1100'0000'0000));
}

TEST_CASE("Rasterizer: Rle binary operations") {
    Path path;
    path.addRect({ 0, 0, 2, 2 });
    PreparedPath2 prepared1(path);
    path.reset();
    path.addRect({ 1, 1, 3, 3 });
    PreparedPath2 prepared2(path);

    SECTION("And") {
        PreparedPath2 result = PreparedPath::booleanOp(MaskOp::And, prepared1, prepared2);
        CHECK(!result.empty());
        CHECK(result.patchBounds() == Rectangle(0, 0, 4, 4));
        REQUIRE(result.patches().size() == 1);
        CHECK(result.patches()[0] == Patch{ 0, 0, 0 });
        REQUIRE(result.patchData().size() == 1);
        CHECK(result.patchData()[0] == PatchData::fromBits(0b0000'0100'0000'0000));
    }

    SECTION("And Not") {
        PreparedPath2 result = PreparedPath::booleanOp(MaskOp::AndNot, prepared1, prepared2);
        CHECK(!result.empty());
        CHECK(result.patchBounds() == Rectangle(0, 0, 4, 4));
        REQUIRE(result.patches().size() == 1);
        CHECK(result.patches()[0] == Patch{ 0, 0, 0 });
        REQUIRE(result.patchData().size() == 1);
        CHECK(result.patchData()[0] == PatchData::fromBits(0b1100'1000'0000'0000));
    }

    SECTION("Or") {
        PreparedPath2 result = PreparedPath::booleanOp(MaskOp::Or, prepared1, prepared2);
        CHECK(!result.empty());
        CHECK(result.patchBounds() == Rectangle(0, 0, 4, 4));
        REQUIRE(result.patches().size() == 1);
        CHECK(result.patches()[0] == Patch{ 0, 0, 0 });
        REQUIRE(result.patchData().size() == 1);
        CHECK(result.patchData()[0] == PatchData::fromBits(0b1100'1110'0110'0000));
    }

    SECTION("Xor") {
        PreparedPath2 result = PreparedPath::booleanOp(MaskOp::Xor, prepared1, prepared2);
        CHECK(!result.empty());
        CHECK(result.patchBounds() == Rectangle(0, 0, 4, 4));
        REQUIRE(result.patches().size() == 1);
        CHECK(result.patches()[0] == Patch{ 0, 0, 0 });
        REQUIRE(result.patchData().size() == 1);
        CHECK(result.patchData()[0] == PatchData::fromBits(0b1100'1010'0110'0000));
    }
}

} // namespace Brisk
