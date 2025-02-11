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
#include <fmt/format.h>
#include <sstream>
#include <iostream>
#include <brisk/core/Json.hpp>
#include <brisk/core/Reflection.hpp>
#include <brisk/core/Reflection.hpp>
#include <brisk/core/BasicTypes.hpp>
#include <brisk/core/Encoding.hpp>
#include <brisk/core/Time.hpp>
#include <fmt/ostream.h>
#include <catch2/catch_all.hpp>

namespace Brisk {

struct NameAndNumber {
    std::string name;
    double number;

    inline static const std::tuple Reflection = {
        ReflectionField{ "name", &NameAndNumber::name },
        ReflectionField{ "number", &NameAndNumber::number },
    };
};

struct SomePoint {
    int x = 0;
    int y = 0;

    friend bool toJson(Json& b, const SomePoint& v) {
        return packArray(b, v.x, v.y);
    }

    friend bool fromJson(const Json& b, SomePoint& v) {
        return unpackArray(b, v.x, v.y);
    }

    bool operator==(const SomePoint& b) const noexcept = default;
};

struct SomePoint2 {
    int x                                  = 0;
    int y                                  = 0;

    static constexpr std::tuple Reflection = {
        ReflectionField{ "x", &SomePoint2::x },
        ReflectionField{ "y", &SomePoint2::y },
    };

    bool operator==(const SomePoint2& b) const noexcept = default;
};

} // namespace Brisk

namespace Brisk {

TEST_CASE("Json") {
    Json b(3.1415926535897932384626433832795);

    b     = "Hello!";

    b     = true;

    b     = JsonArray{ true, 1.5, "ok", 19820984, nullptr };

    b     = -3;

    int y = b.access<JsonSignedInteger>();

    CHECK(y == -3);

    SomePoint pt{ 3, 4 };
    b = pt;

    CHECK(b.is<JsonArray>());
    CHECK(b.access<JsonArray>() == JsonArray{ 3, 4 });

    SomePoint pt2 = *b.to<SomePoint>();

    CHECK(pt2 == SomePoint{ 3, 4 });

    CHECK(std::tuple_size_v<decltype(SomePoint2::Reflection)> == 2);
    CHECK(std::get<0>(SomePoint2::Reflection).pointerToField == &SomePoint2::x);
    CHECK(std::get<1>(SomePoint2::Reflection).pointerToField == &SomePoint2::y);
    CHECK(std::get<0>(SomePoint2::Reflection).name == "x"sv);
    CHECK(std::get<1>(SomePoint2::Reflection).name == "y"sv);

    SomePoint2 pt3{ 123, 456 };
    Json j = SomePoint2{ 777, 10000 };
    CHECK(fmt::to_string(j.toJson()) == R"xxx({"x":777,"y":10000})xxx");
    std::stringstream ss;
    ss << pt3;
    CHECK(ss.str() == "{x:123,y:456}");

    CHECK(fmt::to_string(NameAndNumber{ "John", 42 }) == R"xxx({name:"John",number:42})xxx");
}

BRISK_GNU_ATTR_PRAGMA(GCC diagnostic ignored "-Wunused-value")

TEST_CASE("Json Json") {

    fmt::print("sizeof(Json) = {}\n", sizeof(Json));

    Json b = JsonArray{
        true,
        1.5,
        "ok",
        19820984,
        nullptr,
        JsonObject{
            { "flt", 3.1415926535897932384626433832795 },
            { "str", "\"\"" },
            { "val", UINT64_MAX },
        },
    };

    auto s =
        R"xxx([true,1.5,"ok",19820984,null,{"flt":3.141592653589793,"str":"\"\"","val":18446744073709551615}])xxx";

    CHECK(b.toJson() == s);

    std::optional<Json> bb = Json::fromJson(s);

    CHECK(bb.has_value());
    CHECK(*bb == b);
}

TEST_CASE("item with key") {
    Json b = JsonObject{ { "a", 123 }, { "b", 3.1416 } };

    CHECK(b.type() == JsonType::Object);

    CHECK(b.getItem<JsonSignedInteger>("a") == 123);
    CHECK(b.getItem<JsonFloat>("b") == 3.1416);
}

TEST_CASE("Json: ") {
    Json b = std::vector<int>{ 1, 2, 3 };
    CHECK(b.type() == JsonType::Array);
    CHECK(b.access<JsonArray>().size() == 3);

    CHECK(b.toJson() == "[1,2,3]");

    CHECK(b.to<std::vector<int>>() == std::vector<int>{ 1, 2, 3 });
}

template <typename Container>
auto toVector(Container&& c) {
    return std::vector(c.begin(), c.end());
}

TEST_CASE("iteratePath") {
    using StrList = std::vector<std::string_view>;

    CHECK(toVector(iteratePath("abc")) == StrList{ "abc" });
    CHECK(toVector(iteratePath("/abc")) == StrList{ "", "abc" });
    CHECK(toVector(iteratePath("abc/def")) == StrList{ "abc", "def" });
    CHECK(toVector(iteratePath("/0/1/2")) == StrList{ "", "0", "1", "2" });
    CHECK(toVector(iteratePath("0/1/2")) == StrList{ "0", "1", "2" });

    CHECK(toVector(iteratePath("0/1/2//")) == StrList{ "0", "1", "2", "" });
}

TEST_CASE("load") {
    std::string j = R"(
{
  "artist": "The Beatles",
  "album": "Abbey Road",
  "released": 1969,
  "genre": ["Rock", "Pop"],
  "tracks": [
    {"title": "Come Together", "length": 259},
    {"title": "Something", "length": 182},
    {"title": "Here Comes The Sun", "length": 185}
  ],
  "availableOn": {
    "vinyl": true,
    "cd": true,
    "digital": false
  },
  "rating": 9.5,
  "bandMembers": ["John", "Paul", "George", "Ringo"]
}
)";

    auto b        = Json::fromJson(j);
    REQUIRE(b.has_value());
    CHECK(
        b->toJson() ==
        R"({"album":"Abbey Road","artist":"The Beatles","availableOn":{"cd":true,"digital":false,"vinyl":true},"bandMembers":["John","Paul","George","Ringo"],"genre":["Rock","Pop"],"rating":9.5,"released":1969,"tracks":[{"length":259,"title":"Come Together"},{"length":182,"title":"Something"},{"length":185,"title":"Here Comes The Sun"}]})");
}

TEST_CASE("get long long") {
    Json b  = 12345u;
    auto bb = b.to<long long>();
    REQUIRE(bb.has_value());
    CHECK(*bb == 12345ll);
}

TEST_CASE("get chrono::duration") {
    Json b  = std::chrono::milliseconds(1234);
    auto bb = b.to<std::chrono::milliseconds>();
    REQUIRE(bb.has_value());
    CHECK(*bb == std::chrono::milliseconds(1234));
}

TEST_CASE("get bool") {
    Json b  = true;
    auto bb = b.to<bool>();
    REQUIRE(bb.has_value());
    CHECK(*bb == true);
}

TEST_CASE("toMsgPack") {
    CHECK(Json(true).toMsgPack() == Bytes{ 0xC3_b });
    CHECK(Json(false).toMsgPack() == Bytes{ 0xC2_b });
    CHECK(Json(nullptr).toMsgPack() == Bytes{ 0xC0_b });
    CHECK(Json(1).toMsgPack() == Bytes{ 0x01_b });
    CHECK(Json(32).toMsgPack() == Bytes{ 0x20_b });
    CHECK(Json(1024).toMsgPack() == Bytes{ 0xCD_b, 0x04_b, 0x00_b });
    CHECK(Json(1048768).toMsgPack() == Bytes{ 0xCE_b, 0x00_b, 0x10_b, 0x00_b, 0xC0_b });
    CHECK(Json(1073741824).toMsgPack() == Bytes{ 0xCE_b, 0x40_b, 0x00_b, 0x00_b, 0x00_b });
    CHECK(Json(JsonArray{ 1, 2, 3, 4 }).toMsgPack() == Bytes{ 0x94_b, 0x01_b, 0x02_b, 0x03_b, 0x04_b });
    CHECK(Json("abc").toMsgPack() == Bytes{ 0xA3_b, 0x61_b, 0x62_b, 0x63_b });
    CHECK(Json(JsonObject{ { "a", 1 }, { "b", 0.5f } }).toMsgPack() ==
          Bytes{ 0x82_b, 0xA1_b, 0x61_b, 0x01_b, 0xA1_b, 0x62_b, 0xCB_b, 0x3F_b, 0xE0_b, 0x00_b, 0x00_b,
                 0x00_b, 0x00_b, 0x00_b, 0x00_b });

    CHECK(BytesView(Json(JsonObject{ { "compact", true }, { "schema", 0 } }).toMsgPack()) ==
          toBytesView("\x82\xA7"
                      "compact\xC3\xA6schema\x00"));

    CHECK(Json::fromMsgPack(toBytesView("\x82\xA7"
                                        "compact\xC3\xA6schema\x00")) ==
          Json(JsonObject{ { "compact", true }, { "schema", 0 } }));
}

} // namespace Brisk
