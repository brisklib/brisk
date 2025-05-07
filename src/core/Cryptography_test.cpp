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
#include <brisk/core/Cryptography.hpp>
#include <brisk/core/internal/Span.hpp>
#include "Catch2Utils.hpp"

namespace Brisk {

static std::string_view pangram = "The quick brown fox jumps over the lazy dog";

TEST_CASE("Hashing algorithm validations") {
    CHECK(md5("") == MD5Hash("D41D8CD98F00B204E9800998ECF8427E"));
    CHECK(sha1("") == SHA1Hash("DA39A3EE5E6B4B0D3255BFEF95601890AFD80709"));
    CHECK(sha256("") == SHA256Hash("E3B0C44298FC1C149AFBF4C8996FB92427AE41E4649B934CA495991B7852B855"));
    CHECK(sha512("") == SHA512Hash("CF83E1357EEFB8BDF1542850D66D8007D620E4050B5715DC83F4A921D36CE9CE47D0D13C5"
                                   "D85F2B0FF8318D2877EEC2F63B931BD47417A81A538327AF927DA3E"));
    CHECK(sha3_256("") == SHA3_256Hash("A7FFC6F8BF1ED76651C14756A061D662F580FF4DE43B49FA82D80A4B80F8434A"));
    CHECK(sha3_512("") == SHA3_512Hash("A69F73CCA23A9AC5C8B567DC185A756E97C982164FE25859E0D1DCC1475C80A615B21"
                                       "23AF1F5F94C11E3E9402C3AC558F500199D95B6D3E301758586281DCD26"));

    CHECK(md5(pangram) == MD5Hash("9E107D9D372BB6826BD81D3542A419D6"));
    CHECK(sha1(pangram) == SHA1Hash("2FD4E1C67A2D28FCED849EE1BB76E7391B93EB12"));
    CHECK(sha256(pangram) == SHA256Hash("D7A8FBB307D7809469CA9ABCB0082E4F8D5651E46D3CDB762D02D0BF37C9E592"));
    CHECK(sha512(pangram) == SHA512Hash("07E547D9586F6A73F73FBAC0435ED76951218FB7D0C8D788A309D785436BBB642E93"
                                        "A252A954F23912547D1E8A3B5ED6E1BFD7097821233FA0538F3DB854FEE6"));
    CHECK(sha3_256(pangram) ==
          SHA3_256Hash("69070DDA01975C8C120C3AADA1B282394E7F032FA9CF32F4CB2259A0897DFC04"));
    CHECK(sha3_512(pangram) ==
          SHA3_512Hash("01DEDD5DE4EF14642445BA5F5B97C15E47B9AD931326E4B0727CD94CEFC44FFF23F07BF543139939B4912"
                       "8CAF436DC1BDEE54FCB24023A08D9403F9B4BF0D450"));
    CHECK(sha256("password") ==
          SHA256Hash("5E884898DA28047151D0E56F8DC6292773603D0D6AABBDD62A11EF721D1542D8"));

    SHA256Hash h;
    { std::ignore = sha256HashStream(h)->write(pangram); }
    CHECK(h == SHA256Hash("D7A8FBB307D7809469CA9ABCB0082E4F8D5651E46D3CDB762D02D0BF37C9E592"));
}

TEST_CASE("Random number generation") {
    fmt::print("{}\n", toHex(cryptoRandom(32)));
}
} // namespace Brisk
