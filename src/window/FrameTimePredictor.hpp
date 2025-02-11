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
#include <brisk/core/BasicTypes.hpp>
#include <brisk/core/Time.hpp>

namespace Brisk {

namespace Internal {

struct FrameTimePredictor {
    using MeasurementArray = std::array<double, 64>;
    MeasurementArray frameDeltas{};
    int64_t frameIndex{ 0 };
    std::optional<Clock::time_point> lastFrameTime;

    void markFrameTime() {
        const Clock::time_point thisFrameTime = now();
        if (lastFrameTime) {
            frameDeltas[frameIndex % frameDeltas.size()] = toSeconds(thisFrameTime - *lastFrameTime);
        }
        lastFrameTime = thisFrameTime;

        ++frameIndex;
    }

    Clock::time_point predictNextFrameTime() const {
        if (!lastFrameTime)
            return now();
        MeasurementArray d = frameDeltas;
        std::sort(d.begin(), d.end());
        size_t i = std::upper_bound(d.begin(), d.end(), 0.0) - d.begin();
        if (i == d.size())
            return now();
        return *lastFrameTime +
               std::chrono::duration_cast<Clock::duration>(FractionalSeconds(d[i + (d.size() - i) / 2]));
    }
};
} // namespace Internal

} // namespace Brisk
