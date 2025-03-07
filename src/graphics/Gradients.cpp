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
#include <brisk/graphics/Gradients.hpp>
#include <brisk/core/internal/Lock.hpp>

namespace Brisk {

GradientData::GradientData(const Gradient& gradient) {
    ColorStopArray colorStops = gradient.colorStops();
    if (colorStops.empty()) {
        std::fill(data.begin(), data.end(), ColorF(0.f, 0.f));
        return;
    }
    if (colorStops.size() == 1) {
        std::fill(data.begin(), data.end(), ColorF(colorStops.front().color).premultiply());
        return;
    }
    for (auto& stop : colorStops) {
        stop.position = std::clamp(stop.position, 0.f, 1.f);
    }
    std::sort(colorStops.begin(), colorStops.end(), [](ColorStop elem1, ColorStop elem2) {
        return elem1.position < elem2.position;
    });
    colorStops.front().position = 0.f;
    colorStops.back().position  = 1.f;
    data.front()                = colorStops.front().color;
    data.back()                 = colorStops.back().color;
    for (size_t i = 1; i < gradientResolution - 1; i++) {
        float val = static_cast<float>(i) / (gradientResolution - 1);
        auto gt = std::upper_bound(colorStops.begin(), colorStops.end(), val, [](float val, ColorStop elem) {
            return val < elem.position;
        });
        if (gt == colorStops.begin()) {
            data[i] = colorStops.front().color;
        }
        auto lt = std::prev(gt);
        float t = (val - lt->position) / (gt->position - lt->position + 0.001f);
        data[i] = mix(t, ColorF(lt->color).premultiply(), ColorF(gt->color).premultiply(),
                      AlphaMode::Premultiplied);
    }
}

GradientData::GradientData(const function<ColorF(float)>& func) {
    for (size_t i = 0; i < gradientResolution; i++) {
        data[i] = ColorF(func(static_cast<float>(i) / (gradientResolution - 1))).premultiply();
    }
}

GradientData::GradientData(const std::vector<ColorF>& list, float gamma) {
    for (size_t i = 0; i < gradientResolution; i++) {
        const float x          = std::pow(static_cast<float>(i) / (gradientResolution - 1), gamma);
        const size_t max_index = list.size() - 1;
        float index            = x * max_index;
        if (index <= 0)
            data[i] = ColorF(list[0]).premultiply();
        else if (index >= max_index)
            data[i] = ColorF(list[max_index]).premultiply();
        else {
            const float mu = fract(index);
            data[i] =
                mix(mu, ColorF(list[static_cast<size_t>(index)]).premultiply(),
                    ColorF(list[static_cast<size_t>(index) + 1]).premultiply(), AlphaMode::Premultiplied);
        }
    }
}

ColorF GradientData::operator()(float x) const {
    const size_t max_index = gradientResolution - 1;
    float index            = x * max_index;
    if (index <= 0)
        return data[0];
    else if (index >= max_index)
        return data[max_index];
    else {
        const float mu = fract(index);
        return mix(mu, data[static_cast<size_t>(index)], data[static_cast<size_t>(index) + 1],
                   AlphaMode::Premultiplied);
    }
}

} // namespace Brisk
