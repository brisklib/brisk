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
#include <brisk/graphics/Gradients.hpp>

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
    data.front()                = ColorF(colorStops.front().color).premultiply();
    data.back()                 = ColorF(colorStops.back().color).premultiply();
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

GradientData::GradientData(function_ref<ColorW(float)> func) {
    for (size_t i = 0; i < gradientResolution; i++) {
        data[i] = ColorF(func(static_cast<float>(i) / (gradientResolution - 1))).premultiply();
    }
}

GradientData::GradientData(const std::vector<ColorW>& list, float gamma) {
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

Gradient::Gradient(GradientType type) : Gradient(type, {}, {}, {}) {}

Gradient::Gradient(GradientType type, PointF startPoint, PointF endPoint)
    : Gradient(type, startPoint, endPoint, {}) {}

Gradient::Gradient(GradientType type, PointF startPoint, PointF endPoint, ColorW startColor, ColorW endColor)
    : Gradient(type, startPoint, endPoint, { { 0.f, startColor }, { 1.f, endColor } }) {}

Gradient::Gradient(GradientType type, PointF startPoint, PointF endPoint, ColorStopArray colorStops)
    : m_type(type), m_startPoint(startPoint), m_endPoint(endPoint), m_colorStops(std::move(colorStops)) {}

PointF Gradient::getStartPoint() const {
    return m_startPoint;
}

void Gradient::setStartPoint(PointF pt) {
    m_startPoint = pt;
}

PointF Gradient::getEndPoint() const {
    return m_endPoint;
}

void Gradient::setEndPoint(PointF pt) {
    m_endPoint = pt;
}

void Gradient::addStop(float position, ColorW color) {
    addStop({ position, color });
}

void Gradient::addStop(ColorStop colorStop) {
    m_colorStops.push_back(colorStop);
}

GradientType Gradient::getType() const noexcept {
    return m_type;
}

const ColorStopArray& Gradient::colorStops() const {
    return m_colorStops;
}

LinearGradient::LinearGradient() : Gradient{ GradientType::Linear } {}

LinearGradient::LinearGradient(PointF startPoint, PointF endPoint)
    : Gradient{ GradientType::Linear, startPoint, endPoint } {}

LinearGradient::LinearGradient(PointF startPoint, PointF endPoint, ColorStopArray colorStops)
    : Gradient{ GradientType::Linear, startPoint, endPoint, std::move(colorStops) } {}

LinearGradient::LinearGradient(PointF startPoint, PointF endPoint, ColorW startColor, ColorW endColor)
    : Gradient{ GradientType::Linear, startPoint, endPoint, startColor, endColor } {}

RadialGradient::RadialGradient() : Gradient{ GradientType::Radial } {}

RadialGradient::RadialGradient(PointF point, float radius)
    : Gradient{ GradientType::Radial, point, point + PointF(radius, 0.f) } {}

RadialGradient::RadialGradient(PointF point, float radius, ColorStopArray colorStops)
    : Gradient{ GradientType::Radial, point, point + PointF(radius, 0.f), std::move(colorStops) } {}

RadialGradient::RadialGradient(PointF point, float radius, ColorW startColor, ColorW endColor)
    : Gradient{ GradientType::Radial, point, point + PointF(radius, 0.f), startColor, endColor } {}
} // namespace Brisk
