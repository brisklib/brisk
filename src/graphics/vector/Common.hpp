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

#pragma once

#include <cmath>
#include <numbers>
#include <brisk/graphics/Geometry.hpp>

namespace Brisk {

static const double EPSILON_DOUBLE = 0.000000000001f;
static const float EPSILON_FLOAT   = 0.000001f;

static inline bool vCompare(float p1, float p2) {
    return (std::abs(p1 - p2) < EPSILON_FLOAT);
}

static inline bool vIsZero(float f) {
    return (std::abs(f) <= EPSILON_FLOAT);
}

static inline bool vIsZero(double f) {
    return (std::abs(f) <= EPSILON_DOUBLE);
}

inline bool fuzzyCompare(PointF p1, PointF p2) {
    return (vCompare(p1.x, p2.x) && vCompare(p1.y, p2.y));
}
} // namespace Brisk
