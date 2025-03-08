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
#include <brisk/core/Bytes.hpp>
#include <catch2/catch_all.hpp>
#include "Catch2Utils.hpp"
#include <brisk/core/Hash.hpp>
#include <brisk/core/internal/cityhash.hpp>

namespace Brisk {

using namespace std::string_literals;
using namespace std::string_view_literals;

TEST_CASE("Convert empty Bytes to hex representation") {
    CHECK(toHex(Bytes{}) == "");
}

TEST_CASE("Convert empty hex string to Bytes") {
    CHECK(fromHex("") == Bytes{});
}

TEST_CASE("Handle invalid hex input: '0'") {
    CHECK(fromHex("0") == std::nullopt);
}

TEST_CASE("Handle invalid hex input: 'X'") {
    CHECK(fromHex("X") == std::nullopt);
}

TEST_CASE("Convert uint32 vector to hex") {
    CHECK(toHex(toBytesView(std::vector<uint32_t>{ 0x01234567, 0x89ABCDEF })) == "67452301EFCDAB89");
}

TEST_CASE("Convert Bytes to uppercase hex representation") {
    CHECK(toHex(Bytes{ 0x01_b, 0x23_b, 0x45_b, 0x67_b, 0x89_b, 0xAB_b, 0xCD_b, 0xEF_b }) ==
          "0123456789ABCDEF");
}

TEST_CASE("Convert Bytes to lowercase hex representation") {
    CHECK(toHex(Bytes{ 0x01_b, 0x23_b, 0x45_b, 0x67_b, 0x89_b, 0xAB_b, 0xCD_b, 0xEF_b }, false) ==
          "0123456789abcdef");
}

TEST_CASE("Convert uppercase hex string to Bytes") {
    CHECK(fromHex("0123456789ABCDEF") ==
          Bytes{ 0x01_b, 0x23_b, 0x45_b, 0x67_b, 0x89_b, 0xAB_b, 0xCD_b, 0xEF_b });
}

TEST_CASE("Convert lowercase hex string to Bytes") {
    CHECK(fromHex("0123456789abcdef") ==
          Bytes{ 0x01_b, 0x23_b, 0x45_b, 0x67_b, 0x89_b, 0xAB_b, 0xCD_b, 0xEF_b });
}

TEST_CASE("Convert empty Bytes to Base64 representation") {
    CHECK(toBase64(Bytes{}) == "");
}

TEST_CASE("Convert empty Base64 string to Bytes") {
    CHECK(fromBase64("", false, true) == Bytes{});
}

TEST_CASE("Handle invalid Base64 input: '0'") {
    CHECK(fromBase64("0", false, true) == std::nullopt);
}

TEST_CASE("Handle invalid Base64 input: '@'") {
    CHECK(fromBase64("@", false, true) == std::nullopt);
}

TEST_CASE("Convert Base64 string with newlines to Bytes") {
    CHECK(fromBase64("AAA\r\nAAA", false, false) == Bytes{ 0_b, 0_b, 0_b, 0_b });
}

TEST_CASE("Invalid Base64 string with newlines returns std::nullopt") {
    CHECK(fromBase64("AAA\r\nAAA", false, true) == std::nullopt);
}

TEST_CASE("Convert uint32 vector to Base64") {
    CHECK(toBase64(toBytesView(std::vector<uint32_t>{ 0x01234567, 0x89ABCDEF })) == "Z0UjAe/Nq4k=");
}

TEST_CASE("Convert uint32 vector to Base64 with URL safe flag") {
    CHECK(toBase64(toBytesView(std::vector<uint32_t>{ 0x01234567, 0x89ABCDEF }), true, false) ==
          "Z0UjAe_Nq4k");
}

TEST_CASE("Convert Bytes to Base64") {
    CHECK(toBase64(Bytes{ 0x01_b, 0x23_b, 0x45_b, 0x67_b, 0x89_b, 0xAB_b, 0xCD_b, 0xEF_b }) ==
          "ASNFZ4mrze8=");
}

TEST_CASE("Convert Bytes to Base64 without padding") {
    CHECK(toBase64(Bytes{ 0x01_b, 0x23_b, 0x45_b, 0x67_b, 0x89_b, 0xAB_b, 0xCD_b, 0xEF_b }, false) ==
          "ASNFZ4mrze8=");
}

TEST_CASE("Convert valid Base64 string to Bytes") {
    CHECK(fromBase64("ASNFZ4mrze8=", false, true) ==
          Bytes{ 0x01_b, 0x23_b, 0x45_b, 0x67_b, 0x89_b, 0xAB_b, 0xCD_b, 0xEF_b });
}

TEST_CASE("Duplicate check for valid Base64 string conversion") {
    CHECK(fromBase64("ASNFZ4mrze8=", false, true) ==
          Bytes{ 0x01_b, 0x23_b, 0x45_b, 0x67_b, 0x89_b, 0xAB_b, 0xCD_b, 0xEF_b });
}

TEST_CASE("Convert string of length 3 to CC") {
    CHECK(CC<3>("abc").toString() == "abc"s);
}

TEST_CASE("Convert string of length 3 to string_view") {
    CHECK(CC<3>("def").toStringView() == "def"sv);
}

TEST_CASE("Convert single character string to CC") {
    CHECK(CC<1>("x").toString() == "x"s);
}

TEST_CASE("Convert single character string to string_view") {
    CHECK(CC<1>("x").toStringView() == "x"sv);
}

TEST_CASE("Convert FixedBytes of length 3 to hex") {
    CHECK(FixedBytes<3>("abcdef").toHex() == "ABCDEF"s);
}

TEST_CASE("Convert FixedBytes of length 4 to hex") {
    CHECK(FixedBytes<4>("abcdef01").toHex() == "ABCDEF01"s);
}

TEST_CASE("Convert FixedBytes of length 4 to lowercase hex") {
    CHECK(FixedBytes<4>("abcdef01").toHex(false) == "abcdef01"s);
}

TEST_CASE("Convert FixedBytes of length 4 to Base64 (URL safe)") {
    CHECK(FixedBytes<4>("abcdef01").toBase64(true, false) == "q83vAQ"s);
}

TEST_CASE("Convert FixedBytes of length 4 to Base64 (with padding)") {
    CHECK(FixedBytes<4>("abcdef01").toBase64(true, true) == "q83vAQ=="s);
}

TEST_CASE("Format FixedBytes of length 3 to string") {
    CHECK(fmt::to_string(FixedBytes<3>("abcdef")) == "ABCDEF"s);
}

TEST_CASE("hash") {
    CHECK(fastHash("") == 0);
    CHECK(fastHash("123") == 0xb569baf6b7c11f1a);
    CHECK(fastHash("12345") == 0x98fb61a2e1ad4c5);
    CHECK(fastHash("1234567890") == 0x4fabad57d84b98c1);
    CHECK(fastHash("12345678901234567890") == 0x92f1b6f853ec12d3);
    CHECK(fastHash("1234567890123456789012345678901234567890") == 0xa9e75df98640032c);

    CHECK(CityHash::CityHash64WithSeed("", 0) == 0);
    CHECK(CityHash::CityHash64WithSeed("123", 0) == 0xb569baf6b7c11f1a);
    CHECK(CityHash::CityHash64WithSeed("12345", 0) == 0x98fb61a2e1ad4c5);
    CHECK(CityHash::CityHash64WithSeed("1234567890", 0) == 0x4fabad57d84b98c1);
    CHECK(CityHash::CityHash64WithSeed("12345678901234567890", 0) == 0x92f1b6f853ec12d3);
    CHECK(CityHash::CityHash64WithSeed("1234567890123456789012345678901234567890", 0) == 0xa9e75df98640032c);

    static_assert(CityHash::CityHash64WithSeed("", 0) == 0);
    static_assert(CityHash::CityHash64WithSeed("123", 0) == 0xb569baf6b7c11f1a);
    static_assert(CityHash::CityHash64WithSeed("12345", 0) == 0x98fb61a2e1ad4c5);
    static_assert(CityHash::CityHash64WithSeed("1234567890", 0) == 0x4fabad57d84b98c1);
    static_assert(CityHash::CityHash64WithSeed("12345678901234567890", 0) == 0x92f1b6f853ec12d3);
    static_assert(CityHash::CityHash64WithSeed("1234567890123456789012345678901234567890", 0) ==
                  0xa9e75df98640032c);
}

} // namespace Brisk
