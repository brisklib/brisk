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

#include <brisk/core/Time.hpp>

namespace Brisk {

TEST_CASE("Time duration conversions") {
    CHECK(toSeconds(std::chrono::milliseconds(1500)) == Catch::Approx(1.5));
    CHECK(toSeconds(std::chrono::microseconds(2500)) == Catch::Approx(0.0025));
    CHECK(toHerzs(std::chrono::milliseconds(500)) == Catch::Approx(2.0));
}

TEST_CASE("Time current clock values are monotonic") {
    const auto before = now();
    const auto after  = now();

    CHECK(after >= before);
    CHECK(timeSinceStart() >= Clock::duration::zero());
    CHECK(currentTime() >= 0.0);
    CHECK(perfNow() >= PerformanceDuration::zero());
}

TEST_CASE("PeriodicTimer starts, elapses, and stops") {
    PeriodicTimer timer(false);
    CHECK_FALSE(timer.active());
    CHECK_FALSE(timer.elapsed(0.0));

    timer.start();
    CHECK(timer.active());
    CHECK(timer.elapsed(0.0));

    timer.stop();
    CHECK_FALSE(timer.active());
    CHECK_FALSE(timer.elapsed(0.0));
}

TEST_CASE("PeriodicTimer does not elapse before a positive period") {
    PeriodicTimer timer;
    CHECK(timer.active());
    CHECK_FALSE(timer.elapsed(60.0));
}

TEST_CASE("Stopwatch accumulates elapsed time") {
    PerformanceDuration elapsed = PerformanceDuration::zero();
    {
        Stopwatch stopwatch(elapsed);
        CHECK(elapsed == PerformanceDuration::zero());
    }
    CHECK(elapsed >= PerformanceDuration::zero());
}

TEST_CASE("Chrono durations serialize to JSON") {
    const std::chrono::milliseconds duration{ 1234 };
    Json encoded = duration;
    CHECK(encoded.to<std::chrono::milliseconds>() == duration);

    const auto time = Clock::time_point{ std::chrono::milliseconds{ 5678 } };
    Json encodedTime = time;
    CHECK(encodedTime.to<Clock::time_point>() == time);
}

} // namespace Brisk
