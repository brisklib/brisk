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
    BRISK_ASSERT_MSG("Gradient must have at least one color stop", !colorStops.empty());

    size_t stopCount = std::min(colorStops.size(), gradientMaxStops);
    for (size_t i = 0; i < stopCount; i++) {
        positions[i] = colorStops[i].position;
        colors[i]    = ColorF(colorStops[i].color).premultiply();
    }
    for (size_t i = stopCount; i < gradientMaxStops; i++) {
        positions[i] = 1.f;
        colors[i]    = colors[stopCount - 1];
    }
    // Ensure positions are sorted
    for (size_t i = 1; i < gradientMaxStops; i++) {
        if (positions[i] < positions[i - 1])
            positions[i] = positions[i - 1];
    }
}

ColorF GradientData::operator()(float x) const {
    if (x <= positions[0])
        return colors[0];
    if (x >= positions[gradientMaxStops - 1])
        return colors[gradientMaxStops - 1];
    size_t low  = 0;
    size_t high = gradientMaxStops - 1;
    while (high - low > 1) {
        size_t mid = (low + high) / 2;
        if (x < positions[mid])
            high = mid;
        else
            low = mid;
    }

    float p0 = positions[low];
    float p1 = positions[high];
    if (p1 > p0)
        return mix((x - p0) / (p1 - p0), colors[low], colors[high], AlphaMode::Premultiplied);
    else
        return colors[low];
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
