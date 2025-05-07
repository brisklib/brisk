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
#include <fmt/ranges.h>
#include <catch2/catch_all.hpp>
#include "../core/Catch2Utils.hpp"
#include <brisk/network/Fetch.hpp>
#include <brisk/network/UserAgent.hpp>

using namespace Brisk;

TEST_CASE("Network tests") {
    using namespace std::literals;
    auto [response, bytes] = httpFetchBytes(HttpRequest{
        .url     = "https://example.com",
        .method  = HttpMethod::Head,
        .timeout = 10000ms,
    });
    fmt::println("error = {}", response.error);
    fmt::println("httpCode = {}", response.httpCode);
    fmt::println("effectiveUrl = {}", response.effectiveUrl);
    fmt::println("headers:\n{}", fmt::join(response.headers, "\n"));

    fmt::println("bytes.size() = {}", bytes.size());
    if (!bytes.empty()) {
        fmt::println("response: \n{}", toStringView(bytes));
    }
}
