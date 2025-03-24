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
    using MeasurementArray = std::array<double, 32>;
    MeasurementArray frameDurations{};
    int64_t frameIndex{ 0 };
    std::optional<double> lastFrameTime;
    double fps = 0;

    double markFrameTime() {
        double thisFrameTime = currentTime();
        if (lastFrameTime) {
            double dur                                         = thisFrameTime - *lastFrameTime;
            frameDurations[frameIndex % frameDurations.size()] = dur;
            lastFrameTime                                      = thisFrameTime;
            ++frameIndex;
            return dur;
        } else {
            lastFrameTime = thisFrameTime;
            ++frameIndex;
            return 0.0;
        }
    }

    double predictNextFrameTime() {
        if (!lastFrameTime)
            return currentTime();
        MeasurementArray d = frameDurations;
        std::sort(d.begin(), d.end());
        size_t i = std::upper_bound(d.begin(), d.end(), 0.0) - d.begin();
        if (i == d.size())
            return currentTime();
        double medianTime = d[i + (d.size() - i) / 2];
        fps               = 1.0 / medianTime;
        return *lastFrameTime + medianTime;
    }
};
} // namespace Internal

} // namespace Brisk
