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

#include "vector/Line.hpp"
#include "vector/Bezier.hpp"
#include "vector/Dasher.hpp"
#include "vector/Rle.hpp"
#include "vector/Raster.hpp"

#include <brisk/graphics/Path.hpp>
#include <brisk/graphics/Image.hpp>

namespace Brisk {

PerformanceDuration Internal::performancePathScanline{ 0 };
PerformanceDuration Internal::performancePathRasterization{ 0 };

void Path::checkNewSegment() {
    if (mNewSegment) {
        moveTo(0, 0);
        mNewSegment = false;
    }
}

bool Path::empty() const {
    return m_elements.empty();
}

void Path::moveTo(PointF p) {
    mStartPoint = p;
    mNewSegment = false;
    m_elements.emplace_back(Element::MoveTo);
    m_points.emplace_back(p.x, p.y);
    m_segments++;
    mLengthDirty = true;
}

void Path::lineTo(PointF p) {
    checkNewSegment();
    m_elements.emplace_back(Element::LineTo);
    m_points.emplace_back(p.x, p.y);
    mLengthDirty = true;
}

void Path::cubicTo(PointF c1, PointF c2, PointF e) {
    checkNewSegment();
    m_elements.emplace_back(Element::CubicTo);
    m_points.emplace_back(c1.x, c1.y);
    m_points.emplace_back(c2.x, c2.y);
    m_points.emplace_back(e.x, e.y);
    mLengthDirty = true;
}

void Path::quadraticTo(PointF c, PointF e) {
    PointF s                  = m_points.empty() ? PointF{ 0.f, 0.f } : m_points.back();
    constexpr float twoThirds = 2.f / 3.f;
    PointF c1                 = s + twoThirds * (c - s);
    PointF c2                 = s + twoThirds * (c - e);
    return cubicTo(c1, c2, e);
}

static float tForArcAngle(float angle);

static constexpr float PATH_KAPPA          = 0.5522847498f;
static constexpr float K_PI                = std::numbers::pi_v<float>;

static constexpr float PATH_KAPPA_SQUIRCLE = 0.915f;

void findEllipseCoords(const RectangleF& r, float angle, float length, PointF* startPoint, PointF* endPoint) {
    if (r.empty()) {
        if (startPoint)
            *startPoint = PointF();
        if (endPoint)
            *endPoint = PointF();
        return;
    }

    float w2          = r.width() / 2;
    float h2          = r.height() / 2;

    float angles[2]   = { angle, angle + length };
    PointF* points[2] = { startPoint, endPoint };

    for (int i = 0; i < 2; ++i) {
        if (!points[i])
            continue;

        float theta  = angles[i] - 360 * floorf(angles[i] / 360);
        float t      = theta / 90;
        // truncate
        int quadrant = int(t);
        t -= quadrant;

        t = tForArcAngle(90 * t);

        // swap x and y?
        if (quadrant & 1)
            t = 1 - t;

        float a, b, c, d;
        Bezier::coefficients(t, a, b, c, d);
        PointF p(a + b + c * PATH_KAPPA, d + c + b * PATH_KAPPA);

        // left quadrants
        if (quadrant == 1 || quadrant == 2)
            p.x = -p.x;

        // top quadrants
        if (quadrant == 0 || quadrant == 1)
            p.y = -p.y;

        *points[i] = r.center() + PointF(w2 * p.x, h2 * p.y);
    }
}

static float tForArcAngle(float angle) {
    float radians, cos_angle, sin_angle, tc, ts, t;

    if (vCompare(angle, 0.f))
        return 0;
    if (vCompare(angle, 90.0f))
        return 1;

    radians   = (angle / 180) * K_PI;

    cos_angle = cosf(radians);
    sin_angle = sinf(radians);

    // initial guess
    tc        = angle / 90;

    // do some iterations of newton's method to approximate cos_angle
    // finds the zero of the function b.pointAt(tc).x - cos_angle
    tc -= ((((2 - 3 * PATH_KAPPA) * tc + 3 * (PATH_KAPPA - 1)) * tc) * tc + 1 - cos_angle) // value
          / (((6 - 9 * PATH_KAPPA) * tc + 6 * (PATH_KAPPA - 1)) * tc);                     // derivative
    tc -= ((((2 - 3 * PATH_KAPPA) * tc + 3 * (PATH_KAPPA - 1)) * tc) * tc + 1 - cos_angle) // value
          / (((6 - 9 * PATH_KAPPA) * tc + 6 * (PATH_KAPPA - 1)) * tc);                     // derivative

    // initial guess
    ts = tc;
    // do some iterations of newton's method to approximate sin_angle
    // finds the zero of the function b.pointAt(tc).y - sin_angle
    ts -= ((((3 * PATH_KAPPA - 2) * ts - 6 * PATH_KAPPA + 3) * ts + 3 * PATH_KAPPA) * ts - sin_angle) /
          (((9 * PATH_KAPPA - 6) * ts + 12 * PATH_KAPPA - 6) * ts + 3 * PATH_KAPPA);
    ts -= ((((3 * PATH_KAPPA - 2) * ts - 6 * PATH_KAPPA + 3) * ts + 3 * PATH_KAPPA) * ts - sin_angle) /
          (((9 * PATH_KAPPA - 6) * ts + 12 * PATH_KAPPA - 6) * ts + 3 * PATH_KAPPA);

    // use the average of the t that best approximates cos_angle
    // and the t that best approximates sin_angle
    t = 0.5f * (tc + ts);
    return t;
}

// The return value is the starting point of the arc
static PointF curvesForArc(const RectangleF& rect, float startAngle, float sweepLength, PointF* curves,
                           size_t* point_count) {
    if (rect.empty()) {
        return {};
    }

    float x           = rect.x1;
    float y           = rect.y1;

    float w           = rect.width();
    float w2          = rect.width() / 2;
    float w2k         = w2 * PATH_KAPPA;

    float h           = rect.height();
    float h2          = rect.height() / 2;
    float h2k         = h2 * PATH_KAPPA;

    PointF points[16] = { // start point
                          PointF(x + w, y + h2),

                          // 0 -> 270 degrees
                          PointF(x + w, y + h2 + h2k), PointF(x + w2 + w2k, y + h), PointF(x + w2, y + h),

                          // 270 -> 180 degrees
                          PointF(x + w2 - w2k, y + h), PointF(x, y + h2 + h2k), PointF(x, y + h2),

                          // 180 -> 90 degrees
                          PointF(x, y + h2 - h2k), PointF(x + w2 - w2k, y), PointF(x + w2, y),

                          // 90 -> 0 degrees
                          PointF(x + w2 + w2k, y), PointF(x + w, y + h2 - h2k), PointF(x + w, y + h2)
    };

    if (sweepLength > 360)
        sweepLength = 360;
    else if (sweepLength < -360)
        sweepLength = -360;

    // Special case fast paths
    if (startAngle == 0.0f) {
        if (vCompare(sweepLength, 360)) {
            for (int i = 11; i >= 0; --i)
                curves[(*point_count)++] = points[i];
            return points[12];
        } else if (vCompare(sweepLength, -360)) {
            for (int i = 1; i <= 12; ++i)
                curves[(*point_count)++] = points[i];
            return points[0];
        }
    }

    int startSegment = int(floorf(startAngle / 90.0f));
    int endSegment   = int(floorf((startAngle + sweepLength) / 90.0f));

    float startT     = (startAngle - startSegment * 90) / 90;
    float endT       = (startAngle + sweepLength - endSegment * 90) / 90;

    int delta        = sweepLength > 0 ? 1 : -1;
    if (delta < 0) {
        startT = 1 - startT;
        endT   = 1 - endT;
    }

    // avoid empty start segment
    if (vIsZero(startT - float(1))) {
        startT = 0;
        startSegment += delta;
    }

    // avoid empty end segment
    if (vIsZero(endT)) {
        endT = 1;
        endSegment -= delta;
    }

    startT                  = tForArcAngle(startT * 90);
    endT                    = tForArcAngle(endT * 90);

    const bool splitAtStart = !vIsZero(startT);
    const bool splitAtEnd   = !vIsZero(endT - float(1));

    const int end           = endSegment + delta;

    // empty arc?
    if (startSegment == end) {
        const int quadrant = 3 - ((startSegment % 4) + 4) % 4;
        const int j        = 3 * quadrant;
        return delta > 0 ? points[j + 3] : points[j];
    }

    PointF startPoint, endPoint;
    findEllipseCoords(rect, startAngle, sweepLength, &startPoint, &endPoint);

    for (int i = startSegment; i != end; i += delta) {
        const int quadrant = 3 - ((i % 4) + 4) % 4;
        const int j        = 3 * quadrant;

        Bezier b;
        if (delta > 0)
            b = Bezier::fromPoints(points[j + 3], points[j + 2], points[j + 1], points[j]);
        else
            b = Bezier::fromPoints(points[j], points[j + 1], points[j + 2], points[j + 3]);

        // empty arc?
        if (startSegment == endSegment && vCompare(startT, endT))
            return startPoint;

        if (i == startSegment) {
            if (i == endSegment && splitAtEnd)
                b = b.onInterval(startT, endT);
            else if (splitAtStart)
                b = b.onInterval(startT, 1);
        } else if (i == endSegment && splitAtEnd) {
            b = b.onInterval(0, endT);
        }

        // push control points
        curves[(*point_count)++] = b.pt2();
        curves[(*point_count)++] = b.pt3();
        curves[(*point_count)++] = b.pt4();
    }

    curves[*(point_count)-1] = endPoint;

    return startPoint;
}

void Path::reserve(size_t points, size_t elms) {
    if (m_points.capacity() < m_points.size() + points)
        m_points.reserve(m_points.size() + points);
    if (m_elements.capacity() < m_elements.size() + elms)
        m_elements.reserve(m_elements.size() + elms);
}

static PointF curvesForArc(const RectangleF&, float, float, PointF*, size_t*);

void Path::arcTo(RectangleF rect, float startAngle, float sweepLength, bool forceMoveTo) {
    size_t point_count = 0;
    PointF m_points[15];
    PointF curve_start = curvesForArc(rect, startAngle, sweepLength, m_points, &point_count);

    reserve(point_count + 1, point_count / 3 + 1);
    if (empty() || forceMoveTo) {
        moveTo(curve_start.x, curve_start.y);
    } else {
        lineTo(curve_start.x, curve_start.y);
    }
    for (size_t i = 0; i < point_count; i += 3) {
        cubicTo(m_points[i].x, m_points[i].y, m_points[i + 1].x, m_points[i + 1].y, m_points[i + 2].x,
                m_points[i + 2].y);
    }
}

void Path::close() {
    if (empty())
        return;

    PointF lastPt = m_points.back();
    if (!fuzzyCompare(mStartPoint, lastPt)) {
        lineTo(mStartPoint.x, mStartPoint.y);
    }
    m_elements.push_back(Element::Close);
    mNewSegment  = true;
    mLengthDirty = true;
}

bool Path::isClosed() const {
    size_t closeNum = 0;
    for (Element el : m_elements) {
        closeNum += el == Element::Close;
    }
    return closeNum == m_segments;
}

void Path::reset() {
    if (empty())
        return;

    m_elements.clear();
    m_points.clear();
    m_segments   = 0;
    mLength      = 0;
    mLengthDirty = false;
}

void Path::addCircle(float cx, float cy, float radius, Direction dir) {
    addEllipse(RectangleF(cx - radius, cy - radius, cx + radius, cy + radius), dir);
}

void Path::addEllipse(RectangleF rect, Direction dir) {
    if (rect.empty())
        return;

    float x   = rect.x1;
    float y   = rect.y1;

    float w   = rect.width();
    float w2  = rect.width() / 2;
    float w2k = w2 * PATH_KAPPA;

    float h   = rect.height();
    float h2  = rect.height() / 2;
    float h2k = h2 * PATH_KAPPA;

    reserve(13, 6); // 1Move + 4Cubic + 1Close
    if (dir == Direction::CW) {
        // moveto 12 o'clock.
        moveTo(x + w2, y);
        // 12 -> 3 o'clock
        cubicTo(x + w2 + w2k, y, x + w, y + h2 - h2k, x + w, y + h2);
        // 3 -> 6 o'clock
        cubicTo(x + w, y + h2 + h2k, x + w2 + w2k, y + h, x + w2, y + h);
        // 6 -> 9 o'clock
        cubicTo(x + w2 - w2k, y + h, x, y + h2 + h2k, x, y + h2);
        // 9 -> 12 o'clock
        cubicTo(x, y + h2 - h2k, x + w2 - w2k, y, x + w2, y);
    } else {
        // moveto 12 o'clock.
        moveTo(x + w2, y);
        // 12 -> 9 o'clock
        cubicTo(x + w2 - w2k, y, x, y + h2 - h2k, x, y + h2);
        // 9 -> 6 o'clock
        cubicTo(x, y + h2 + h2k, x + w2 - w2k, y + h, x + w2, y + h);
        // 6 -> 3 o'clock
        cubicTo(x + w2 + w2k, y + h, x + w, y + h2 + h2k, x + w, y + h2);
        // 3 -> 12 o'clock
        cubicTo(x + w, y + h2 - h2k, x + w2 + w2k, y, x + w2, y);
    }
    close();
}

void Path::addRect(RectangleF rect, Direction dir) {
    float x = rect.x1;
    float y = rect.y1;
    float w = rect.width();
    float h = rect.height();

    if (vCompare(w, 0.f) && vCompare(h, 0.f))
        return;

    reserve(5, 6); // 1Move + 4Line + 1Close
    if (dir == Direction::CW) {
        moveTo(x + w, y);
        lineTo(x + w, y + h);
        lineTo(x, y + h);
        lineTo(x, y);
        close();
    } else {
        moveTo(x + w, y);
        lineTo(x, y);
        lineTo(x, y + h);
        lineTo(x + w, y + h);
        close();
    }
}

void Path::addRoundRect(RectangleF rect, float rx, float ry, bool squircle, Direction dir) {
    if (vCompare(rx, 0.f) || vCompare(ry, 0.f)) {
        addRect(rect, dir);
        return;
    }

    float x1 = rect.x1;
    float y1 = rect.y1;
    float x2 = rect.x2;
    float y2 = rect.y2;
    // clamp the rx and ry radius value.
    if (rx > rect.width() * 0.5f)
        rx = rect.width() * 0.5f;
    if (ry > rect.height() * 0.5f)
        ry = rect.height() * 0.5f;

    float ikappa = 1.f - (squircle ? PATH_KAPPA_SQUIRCLE : PATH_KAPPA);

    reserve(17, 10); // 1Move + 4Cubic + 1Close
    if (dir != Direction::CW) {
        std::swap(x1, x2);
        rx = -rx;
    }
    moveTo(x2, y1 + ry);
    lineTo(x2, y2 - ry);
    cubicTo(x2, y2 - ry * ikappa, x2 - rx * ikappa, y2, x2 - rx, y2);
    lineTo(x1 + rx, y2);
    cubicTo(x1 + rx * ikappa, y2, x1, y2 - ry * ikappa, x1, y2 - ry);
    lineTo(x1, y1 + ry);
    cubicTo(x1, y1 + ry * ikappa, x1 + rx * ikappa, y1, x1 + rx, y1);
    lineTo(x2 - rx, y1);
    cubicTo(x2 - rx * ikappa, y1, x2, y1 + ry * ikappa, x2, y1 + ry);
    close();
}

void Path::addRoundRect(RectangleF rect, CornersF rx, CornersF ry, bool squircle, Direction dir) {
    float x1 = rect.x1;
    float y1 = rect.y1;
    float x2 = rect.x2;
    float y2 = rect.y2;
    for (float& x : rx.components)
        x = std::min(x, rect.width() * 0.5f);
    for (float& y : ry.components)
        y = std::min(y, rect.height() * 0.5f);

    float ikappa = 1.f - (squircle ? PATH_KAPPA_SQUIRCLE : PATH_KAPPA);

    reserve(17, 10); // 1Move + 4Cubic + 1Close
    if (dir != Direction::CW) {
        std::swap(x1, x2);
        for (float& x : rx.components)
            x = -x;
        std::swap(rx[0], rx[1]);
        std::swap(rx[2], rx[3]);
        std::swap(ry[0], ry[1]);
        std::swap(ry[2], ry[3]);
    }
    moveTo(x2, y1 + ry[1]);
    lineTo(x2, y2 - ry[3]);
    cubicTo(x2, y2 - ry[3] * ikappa, x2 - rx[3] * ikappa, y2, x2 - rx[3], y2);
    lineTo(x1 + rx[2], y2);
    cubicTo(x1 + rx[2] * ikappa, y2, x1, y2 - ry[2] * ikappa, x1, y2 - ry[2]);
    lineTo(x1, y1 + ry[0]);
    cubicTo(x1, y1 + ry[0] * ikappa, x1 + rx[0] * ikappa, y1, x1 + rx[0], y1);
    lineTo(x2 - rx[1], y1);
    cubicTo(x2 - rx[1] * ikappa, y1, x2, y1 + ry[1] * ikappa, x2, y1 + ry[1]);
    close();
}

void Path::addPolystar(float points, float innerRadius, float outerRadius, float innerRoundness,
                       float outerRoundness, float startAngle, float cx, float cy, Direction dir) {

    constexpr static float POLYSTAR_MAGIC_NUMBER = 0.47829f / 0.28f;
    float currentAngle                           = (startAngle - 90.0f) * K_PI / 180.0f;
    float x;
    float y;
    float partialPointRadius = 0;
    float anglePerPoint      = (2.0f * K_PI / points);
    float halfAnglePerPoint  = anglePerPoint / 2.0f;
    float partialPointAmount = points - floorf(points);
    bool longSegment         = false;
    size_t numPoints         = size_t(ceilf(points) * 2);
    float angleDir           = ((dir == Direction::CW) ? 1.0f : -1.0f);
    bool hasRoundness        = false;

    innerRoundness /= 100.0f;
    outerRoundness /= 100.0f;

    if (!vCompare(partialPointAmount, 0)) {
        currentAngle += halfAnglePerPoint * (1.0f - partialPointAmount) * angleDir;
    }

    if (!vCompare(partialPointAmount, 0)) {
        partialPointRadius = innerRadius + partialPointAmount * (outerRadius - innerRadius);
        x                  = partialPointRadius * cosf(currentAngle);
        y                  = partialPointRadius * sinf(currentAngle);
        currentAngle += anglePerPoint * partialPointAmount / 2.0f * angleDir;
    } else {
        x = outerRadius * cosf(currentAngle);
        y = outerRadius * sinf(currentAngle);
        currentAngle += halfAnglePerPoint * angleDir;
    }

    if (vIsZero(innerRoundness) && vIsZero(outerRoundness)) {
        reserve(numPoints + 2, numPoints + 3);
    } else {
        reserve(numPoints * 3 + 2, numPoints + 3);
        hasRoundness = true;
    }

    moveTo(x + cx, y + cy);

    for (size_t i = 0; i < numPoints; i++) {
        float radius = longSegment ? outerRadius : innerRadius;
        float dTheta = halfAnglePerPoint;
        if (!vIsZero(partialPointRadius) && i == numPoints - 2) {
            dTheta = anglePerPoint * partialPointAmount / 2.0f;
        }
        if (!vIsZero(partialPointRadius) && i == numPoints - 1) {
            radius = partialPointRadius;
        }
        float previousX = x;
        float previousY = y;
        x               = radius * cosf(currentAngle);
        y               = radius * sinf(currentAngle);

        if (hasRoundness) {
            float cp1Theta     = (atan2f(previousY, previousX) - K_PI / 2.0f * angleDir);
            float cp1Dx        = cosf(cp1Theta);
            float cp1Dy        = sinf(cp1Theta);
            float cp2Theta     = (atan2f(y, x) - K_PI / 2.0f * angleDir);
            float cp2Dx        = cosf(cp2Theta);
            float cp2Dy        = sinf(cp2Theta);

            float cp1Roundness = longSegment ? innerRoundness : outerRoundness;
            float cp2Roundness = longSegment ? outerRoundness : innerRoundness;
            float cp1Radius    = longSegment ? innerRadius : outerRadius;
            float cp2Radius    = longSegment ? outerRadius : innerRadius;

            float cp1x         = cp1Radius * cp1Roundness * POLYSTAR_MAGIC_NUMBER * cp1Dx / points;
            float cp1y         = cp1Radius * cp1Roundness * POLYSTAR_MAGIC_NUMBER * cp1Dy / points;
            float cp2x         = cp2Radius * cp2Roundness * POLYSTAR_MAGIC_NUMBER * cp2Dx / points;
            float cp2y         = cp2Radius * cp2Roundness * POLYSTAR_MAGIC_NUMBER * cp2Dy / points;

            if (!vIsZero(partialPointAmount) && ((i == 0) || (i == numPoints - 1))) {
                cp1x *= partialPointAmount;
                cp1y *= partialPointAmount;
                cp2x *= partialPointAmount;
                cp2y *= partialPointAmount;
            }

            cubicTo(previousX - cp1x + cx, previousY - cp1y + cy, x + cp2x + cx, y + cp2y + cy, x + cx,
                    y + cy);
        } else {
            lineTo(x + cx, y + cy);
        }

        currentAngle += dTheta * angleDir;
        longSegment = !longSegment;
    }

    close();
}

void Path::addPolygon(float points, float radius, float roundness, float startAngle, float cx, float cy,
                      Direction dir) {
    constexpr static float POLYGON_MAGIC_NUMBER = 0.25;
    float currentAngle                          = (startAngle - 90.0f) * K_PI / 180.0f;
    float x;
    float y;
    float anglePerPoint = 2.0f * K_PI / floorf(points);
    size_t numPoints    = size_t(floorf(points));
    float angleDir      = ((dir == Direction::CW) ? 1.0f : -1.0f);
    bool hasRoundness   = false;

    roundness /= 100.0f;

    currentAngle = (currentAngle - 90.0f) * K_PI / 180.0f;
    x            = radius * cosf(currentAngle);
    y            = radius * sinf(currentAngle);
    currentAngle += anglePerPoint * angleDir;

    if (vIsZero(roundness)) {
        reserve(numPoints + 2, numPoints + 3);
    } else {
        reserve(numPoints * 3 + 2, numPoints + 3);
        hasRoundness = true;
    }

    moveTo(x + cx, y + cy);

    for (size_t i = 0; i < numPoints; i++) {
        float previousX = x;
        float previousY = y;
        x               = (radius * cosf(currentAngle));
        y               = (radius * sinf(currentAngle));

        if (hasRoundness) {
            float cp1Theta = (atan2f(previousY, previousX) - K_PI / 2.0f * angleDir);
            float cp1Dx    = cosf(cp1Theta);
            float cp1Dy    = sinf(cp1Theta);
            float cp2Theta = atan2f(y, x) - K_PI / 2.0f * angleDir;
            float cp2Dx    = cosf(cp2Theta);
            float cp2Dy    = sinf(cp2Theta);

            float cp1x     = radius * roundness * POLYGON_MAGIC_NUMBER * cp1Dx;
            float cp1y     = radius * roundness * POLYGON_MAGIC_NUMBER * cp1Dy;
            float cp2x     = radius * roundness * POLYGON_MAGIC_NUMBER * cp2Dx;
            float cp2y     = radius * roundness * POLYGON_MAGIC_NUMBER * cp2Dy;

            cubicTo(previousX - cp1x + cx, previousY - cp1y + cy, x + cp2x + cx, y + cp2y + cy, x, y);
        } else {
            lineTo(x + cx, y + cy);
        }

        currentAngle += anglePerPoint * angleDir;
    }

    close();
}

void Path::addPath(const Path& path) {
    size_t segment = path.m_segments;

    // make sure enough memory available
    if (m_points.capacity() < m_points.size() + path.m_points.size())
        m_points.reserve(m_points.size() + path.m_points.size());

    if (m_elements.capacity() < m_elements.size() + path.m_elements.size())
        m_elements.reserve(m_elements.size() + path.m_elements.size());

    std::copy(path.m_points.begin(), path.m_points.end(), back_inserter(m_points));

    std::copy(path.m_elements.begin(), path.m_elements.end(), back_inserter(m_elements));

    m_segments += segment;
    mLengthDirty = true;
}

void Path::addPath(const Path& path, const Matrix& m) {
    size_t numPoints = m_points.size();

    addPath(path);

    if (m_points.size() > numPoints) {
        m.transform(std::span<PointF>{ m_points.data() + numPoints, m_points.size() - numPoints });
    }
}

void Path::transform(const Matrix& m) {
    if (m.isIdentity())
        return;
    m.transform(std::span<PointF>{ m_points.data(), m_points.size() });
}

Path Path::transformed(const Matrix& m) const& {
    Path copy = *this;
    copy.transform(m);
    return copy;
}

Path Path::transformed(const Matrix& m) && {
    transform(m);
    return std::move(*this);
}

float Path::length() const {
    if (!mLengthDirty)
        return mLength;

    mLengthDirty = false;
    mLength      = 0.0;

    size_t i     = 0;
    for (auto e : m_elements) {
        switch (e) {
        case Element::MoveTo:
            i++;
            break;
        case Element::LineTo: {
            mLength += VLine(m_points[i - 1], m_points[i]).length();
            i++;
            break;
        }
        case Element::CubicTo: {
            mLength +=
                Bezier::fromPoints(m_points[i - 1], m_points[i], m_points[i + 1], m_points[i + 2]).length();
            i += 3;
            break;
        }
        case Element::Close:
            break;
        }
    }

    return mLength;
}

RectangleF Path::boundingBoxApprox() const {
    RectangleF result{ HUGE_VALF, HUGE_VALF, -HUGE_VALF, -HUGE_VALF };
    for (auto p : m_points) {
        result.x1 = std::min(result.x1, p.x);
        result.y1 = std::min(result.y1, p.y);
        result.x2 = std::max(result.x2, p.x);
        result.y2 = std::max(result.y2, p.y);
    }
    return result;
}

Path Path::dashed(std::span<const float> pattern, float offset) const {
    Dasher dasher(pattern.data(), pattern.size());
    Path result;
    result = dasher.dashed(*this);
    return result;
}

BRISK_ALWAYS_INLINE uint32_t div255(uint32_t n) {
    return (n + 1 + (n >> 8)) >> 8;
}

BRISK_ALWAYS_INLINE static void blendRow(uint8_t* dst, uint8_t src, uint32_t len) {
    if (src == 0)
        return;
    if (src >= 255) {
        memset(dst, 255, len);
        return;
    }
    for (uint32_t j = 0; j < len; ++j) {
        *dst = *dst + div255(src * (255 - *dst));
        ++dst;
    }
}

struct GreyBitmap {
    size_t stride;
    size_t rows;
    std::unique_ptr<uint8_t[]> data;
    Size size;

    GreyBitmap() : data(nullptr), size(0, 0) {}

    GreyBitmap(Size size)
        : stride(alignUp(size.width, 4) + 4), rows(size.height + 3 + 3),
          data(std::make_unique<uint8_t[]>(stride * rows + 4)), size(size) {
        memset(data.get(), 0, stride * rows + 4);
    }

    uint8_t* line(int32_t y) const {
        return data.get() + (y + 3) * stride + 4;
    }
};

static GreyBitmap rleToMask(const Rle& rle, Size size) {
    if (rle.empty())
        return {};
    GreyBitmap bitmap(size);

    const auto& spans = rle.spans();

    int16_t yy        = INT16_MIN;
    uint8_t* line     = nullptr;
    for (size_t i = 0; i < spans.size(); ++i) {
        if (spans[i].y != yy) [[unlikely]] {
            line = bitmap.line(spans[i].y);
        }
        uint8_t* row = line + spans[i].x;
        if (row)
            blendRow(row, spans[i].coverage, spans[i].len);
    }
    return bitmap;
}

struct SparseMask {
    std::vector<Internal::Patch> patches;
    std::vector<Internal::PatchData> patchData;
};

using Internal::Patch;
using Internal::PatchData;
using Internal::PatchDataHash;

struct PatchMerger {
    std::vector<Internal::Patch>& patches;
    std::vector<Internal::PatchData>& patchData;

    PatchMerger(std::vector<Internal::Patch>& patches, std::vector<Internal::PatchData>& patchData)
        : patches(patches), patchData(patchData) {}

    void reserve(size_t size) {
        patches.reserve(size / 2);
        patchData.reserve((size + 7) / 8);
    }

    BRISK_INLINE void add(uint16_t x, uint16_t y, uint8_t len, const PatchData& data) {
        // Extend the last patch if it matches
        if (!patches.empty()) [[likely]] {
            auto& back       = patches.back();
            uint16_t back_x  = back.x();
            uint16_t back_y  = back.y();
            uint8_t back_len = back.len();

            if (x == back_x + back_len && y == back_y && back_len < 0xff && patchData[back.offset] == data)
                [[unlikely]] {
                // merge with previous patch
                back = Patch(back_x, back_y, back_len + len, back.offset);
                return;
            }
        }

        uint32_t offset;
        offset = uint32_t(patchData.size());
        patchData.push_back(data);
        patches.emplace_back(x, y, len, offset);
    }

    void add(std::span<const Patch> sourcePatches, const std::vector<PatchData>& sourcePatchData) {
        for (const Patch& patch : sourcePatches) {
            const PatchData& data = sourcePatchData[patch.offset];
            add(patch.x(), patch.y(), patch.len(), data);
        }
    }
};

static SparseMask sparseMaskFromImage(const GreyBitmap& bitmap, Rectangle bounds) {
    using namespace Internal;
    SparseMask result;

    PatchMerger merger(result.patches, result.patchData);
    merger.reserve(bounds.size().area() / 16 + 1);

    size_t stride = bitmap.stride / 4;

    // Iterate over the image in screen-aligned 4x4 patches:
    for (uint32_t y = uint32_t(bounds.y1) / 4; y * 4 < uint32_t(bounds.y2); y++) {
        const uint8_t* line = bitmap.line(y * 4 - bounds.y1);

        for (uint32_t x = uint32_t(bounds.x1) / 4; x * 4 < uint32_t(bounds.x2); x++) {
            PatchData patchData;
            const uint32_t* pixels = reinterpret_cast<const uint32_t*>(line + x * 4 - bounds.x1);
            patchData.data_u32[0]  = *pixels;
            patchData.data_u32[1]  = *(pixels + stride);
            patchData.data_u32[2]  = *(pixels + 2 * stride);
            patchData.data_u32[3]  = *(pixels + 3 * stride);

            if (patchData.empty()) [[unlikely]]
                continue;
            merger.add(uint16_t(x), uint16_t(y), 1, patchData);
        }
    }
    return result;
}

void Path::addPolyline(std::span<const PointF> points) {
    if (points.empty())
        return;
    moveTo(points.front());
    for (size_t i = 1; i < points.size(); ++i)
        lineTo(points[i]);
}

PreparedPath::PreparedPath(const Path& path, const FillParams& params, Rectangle clipRect,
                           bool optimizeRectangle) {
    if (optimizeRectangle) {
        if (std::optional<RectangleF> rect = path.asRectangle()) {
            initRect(*rect);
            return;
        }
    }
    Rle rle = rasterize(path, params.fillRule, clipRect == noClipRect ? Rectangle{} : clipRect);
    init(std::move(rle));
}

PreparedPath::PreparedPath(const Path& path, const StrokeParams& params, Rectangle clipRect) {
    Rle rle = rasterize(path, params.capStyle, params.joinStyle, params.strokeWidth, params.miterLimit,
                        clipRect == noClipRect ? Rectangle{} : clipRect);
    init(std::move(rle));
}

void PreparedPath::init(Rle&& rle) {
    if (rle.empty()) {
        return;
    }
    Rectangle bounds = rle.boundingRect();
    rle.translate(Point{ -bounds.x1, -bounds.y1 });
    GreyBitmap image      = rleToMask(rle, bounds.size());
    SparseMask sparseMask = sparseMaskFromImage(image, bounds);
    m_patches             = std::move(sparseMask.patches);
    m_patchData           = std::move(sparseMask.patchData);
}

void PreparedPath::initRect(RectangleF rect) {
    if (rect.empty()) {
        return;
    }
    m_rectangle = rect;
}

PreparedPath::PreparedPath(RectangleF rectangle, bool optimizeRectangle) {
    if (optimizeRectangle) {
        initRect(rectangle);
    } else {
        Path path;
        path.addRect(rectangle);
        Rle rle = rasterize(path, FillRule::Winding, noClipRect);
        init(std::move(rle));
    }
}

using Internal::boolOp;
using Internal::coverageOp;
using Internal::rectangleOp;

static PatchData coverageOp(MaskOp op, const PatchData& a, const PatchData& b) {
    PatchData result;
    coverageOp<16>(op, result.data_u8, a.data_u8, b.data_u8);
    return result;
}

PreparedPath PreparedPath::pathOp(MaskOp op, const PreparedPath& left, const PreparedPath& right) {
    if (left.empty() && right.empty()) [[unlikely]] {
        return {}; // empty mask
    }
    if (left.isRectangle() && right.isRectangle()) [[unlikely]] {
        // if both are rectangles, we can use the rectangle operation
        std::optional<RectangleF> result = rectangleOp(op, left.m_rectangle, right.m_rectangle);
        if (result)
            return PreparedPath(*result);
    }
    if (left.isRectangle() && right.isRectangle()) [[unlikely]] {
        return pathOp(op, left.toSparse(), right.toSparse());
    }
    if (left.isRectangle() && !right.isRectangle()) [[unlikely]] {
        return pathOp(op, left.toSparse(), right);
    }
    if (!left.isRectangle() && right.isRectangle()) [[unlikely]] {
        return pathOp(op, left, right.toSparse());
    }

    BRISK_ASSERT(left.isSparse());
    BRISK_ASSERT(right.isSparse());

    bool singleLeft  = boolOp(op, true, false);
    bool singleRight = boolOp(op, false, true);

    if (left.empty()) [[unlikely]] {
        if (singleRight) {
            return right;
        } else {
            return {};
        }
    }
    if (right.empty()) [[unlikely]] {
        if (singleLeft) {
            return left;
        } else {
            return {};
        }
    }

    if (!left.patchBounds().intersects(right.patchBounds())) [[unlikely]] {
        // if two rle are disjoint
        switch (op) {
        case MaskOp::And:
            return {};
        case MaskOp::AndNot:
            return left;
        case MaskOp::Or:
        case MaskOp::Xor: {
            // merge the two rle objects
            if (left.m_patches.front() < right.m_patches.front())
                return merge(left, right);
            else
                return merge(right, left);
        }
        default:
            BRISK_UNREACHABLE();
        }
    }

    PreparedPath result;
    PatchMerger merger(result.m_patches, result.m_patchData);

    struct PatchIterator {
        std::span<const Patch> list;
        uint8_t index;

        PatchIterator(std::span<const Patch> list) : list(list), index(0) {}

        bool empty() const {
            return list.empty();
        }

        Patch front() const {
            if (list.front().len() == 1)
                return list.front();
            return Patch(list.front().x() + index, list.front().y(), 1, list.front().offset);
        }

        Patch partial() const {
            return Patch(list.front().x() + index, list.front().y(), list.front().len() - index,
                         list.front().offset);
        }

        void next(uint8_t len = 1) {
            index += len;
            if (index >= list.front().len()) {
                index = 0;
                list  = list.subspan(1);
            }
        }
    };

    PatchIterator l{ left.m_patches };
    PatchIterator r{ right.m_patches };

    for (; !l.empty() && !r.empty();) {
        const Patch lFront = l.front();
        const Patch rFront = r.front();

        if (lFront < rFront) {
            // left patch is before right patch
            if (singleLeft) {
                const PatchData& data = left.m_patchData[l.list.front().offset];
                merger.add(lFront.x(), lFront.y(), 1, data);
            }
            l.next();
        } else if (rFront < lFront) {
            // right patch is before left patch
            if (singleRight) {
                const PatchData& data = right.m_patchData[r.list.front().offset];
                merger.add(rFront.x(), rFront.y(), 1, data);
            }
            r.next();
        } else {
            // patches are equal
            const PatchData& dataLeft  = left.m_patchData[l.list.front().offset];
            const PatchData& dataRight = right.m_patchData[r.list.front().offset];
            PatchData dataResult       = coverageOp(op, dataLeft, dataRight);
            if (!dataResult.empty())
                merger.add(lFront.x(), lFront.y(), 1, dataResult);
            l.next();
            r.next();
        }
    }

    if (l.empty()) {
        if (singleRight) {
            // left is empty, so we can add right
            if (r.index > 0) {
                Patch rightPartial = r.partial();
                merger.add(rightPartial.x(), rightPartial.y(), rightPartial.len(),
                           right.m_patchData[rightPartial.offset]);
                r.next(rightPartial.len());
            }

            merger.add(r.list, right.m_patchData);
        }
    } else { // if (r.empty())
        if (singleLeft) {
            // right is empty, so we can add left
            if (l.index > 0) {
                Patch leftPartial = l.partial();
                merger.add(leftPartial.x(), leftPartial.y(), leftPartial.len(),
                           left.m_patchData[leftPartial.offset]);
                l.next(leftPartial.len());
            }
            merger.add(l.list, left.m_patchData);
        }
    }
    return result;
}

PreparedPath PreparedPath::merge(const PreparedPath& a, const PreparedPath& b) {
    PreparedPath result;
    result.m_patches   = a.m_patches;
    result.m_patchData = a.m_patchData;
    std::map<PatchData, uint32_t> lookup;
    PatchMerger merger(result.m_patches, result.m_patchData);

    for (const Patch& patch : b.m_patches) {
        const PatchData& data = b.m_patchData[patch.offset];
        merger.add(patch.x(), patch.y(), patch.len(), data);
    }
    return result;
}

Rectangle PreparedPath::patchBounds() const {
    if (m_patchBounds.has_value())
        return m_patchBounds.value();
    Rectangle bounds{ INT32_MAX, INT32_MAX, INT32_MIN, INT32_MIN };
    for (const Patch& patch : m_patches) {
        const auto& data = m_patchData[patch.offset];
        bounds.x1        = std::min(bounds.x1, int32_t(patch.x() * 4));
        bounds.y1        = std::min(bounds.y1, int32_t(patch.y() * 4));
        bounds.x2        = std::max(bounds.x2, int32_t(patch.x() * 4) + 4 * patch.len());
        bounds.y2        = std::max(bounds.y2, int32_t(patch.y() * 4) + 4);
    }
    if (bounds.empty()) {
        // empty path
        return Rectangle{};
    }
    m_patchBounds = bounds;
    return bounds;
}

PreparedPath PreparedPath::union_(const PreparedPath& a, const PreparedPath& b) {
    return pathOp(MaskOp::Union, a, b);
}

PreparedPath PreparedPath::intersection(const PreparedPath& a, const PreparedPath& b) {
    return pathOp(MaskOp::Intersection, a, b);
}

PreparedPath PreparedPath::difference(const PreparedPath& a, const PreparedPath& b) {
    return pathOp(MaskOp::Difference, a, b);
}

PreparedPath PreparedPath::symmetricDifference(const PreparedPath& a, const PreparedPath& b) {
    return pathOp(MaskOp::SymmetricDifference, a, b);
}

std::optional<RectangleF> Path::asRectangle() const {
    if (m_segments != 1 || m_elements.size() != 6 || m_points.size() != 5 ||
        !fuzzyCompare(m_points[4], m_points[0]))
        return std::nullopt;
    if (m_elements[0] != Element::MoveTo || m_elements[1] != Element::LineTo ||
        m_elements[2] != Element::LineTo || m_elements[3] != Element::LineTo ||
        m_elements[4] != Element::LineTo || m_elements[5] != Element::Close)
        return std::nullopt;
    if (!(vCompare(m_points[1].x, m_points[0].x) || vCompare(m_points[3].x, m_points[2].x) ||
          vCompare(m_points[3].y, m_points[0].y) || vCompare(m_points[2].y, m_points[1].y)))
        return std::nullopt;
    return RectangleF{
        std::min(m_points[0].x, m_points[3].x),
        std::min(m_points[0].y, m_points[1].y),
        std::max(m_points[0].x, m_points[3].x),
        std::max(m_points[0].y, m_points[1].y),
    };
}
} // namespace Brisk
