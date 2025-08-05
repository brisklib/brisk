/*
 * Copyright (c) 2020 Samsung Electronics Co., Ltd. All rights reserved.

 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "Bezier.hpp"

namespace Brisk {

Bezier Bezier::fromPoints(PointF p1, PointF p2, PointF p3, PointF p4) {
    Bezier b;
    b.x1 = p1.x;
    b.y1 = p1.y;
    b.x2 = p2.x;
    b.y2 = p2.y;
    b.x3 = p3.x;
    b.y3 = p3.y;
    b.x4 = p4.x;
    b.y4 = p4.y;
    return b;
}

float Bezier::length() const {
    const auto len =
        VLine::length(x1, y1, x2, y2) + VLine::length(x2, y2, x3, y3) + VLine::length(x3, y3, x4, y4);

    const auto chord = VLine::length(x1, y1, x4, y4);

    if ((len - chord) > 0.01) {
        Bezier left, right;
        split(&left, &right);
        return left.length() + right.length();
    }

    return len;
}

Bezier Bezier::onInterval(float t0, float t1) const {
    if (t0 == 0 && t1 == 1)
        return *this;

    Bezier bezier = *this;

    Bezier result;
    bezier.parameterSplitLeft(t0, &result);
    float trueT = (t1 - t0) / (1 - t0);
    bezier.parameterSplitLeft(trueT, &result);

    return result;
}

float Bezier::tAtLength(float l, float totalLength) const {
    float t           = 1.0;
    const float error = 0.01f;
    if (l > totalLength || vCompare(l, totalLength))
        return t;

    t *= 0.5;

    float lastBigger = 1.0;
    for (int num = 0; num < 100500; num++) {
        Bezier right = *this;
        Bezier left;
        right.parameterSplitLeft(t, &left);
        float lLen = left.length();
        if (fabs(lLen - l) < error)
            return t;

        if (lLen < l) {
            t += (lastBigger - t) * 0.5f;
        } else {
            lastBigger = t;
            t -= t * 0.5f;
        }
    }
    return t;
}

void Bezier::splitAtLength(float len, Bezier* left, Bezier* right) {
    float t;

    *right = *this;
    t      = right->tAtLength(len);
    right->parameterSplitLeft(t, left);
}

PointF Bezier::derivative(float t) const {
    // p'(t) = 3 * (-(1-2t+t^2) * p0 + (1 - 4 * t + 3 * t^2) * p1 + (2 * t - 3 *
    // t^2) * p2 + t^2 * p3)

    float m_t = 1.0f - t;

    float d   = t * t;
    float a   = -m_t * m_t;
    float b   = 1 - 4 * t + 3 * d;
    float c   = 2 * t - 3 * d;

    return 3 * PointF(a * x1 + b * x2 + c * x3 + d * x4, a * y1 + b * y2 + c * y3 + d * y4);
}

float Bezier::angleAt(float t) const {
    if (t < 0 || t > 1) {
        return 0;
    }
    return VLine({}, derivative(t)).angle();
}
} // namespace Brisk
