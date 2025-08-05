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

#include "Common.hpp"

#ifdef __SSE2__
#ifdef _MSC_VER
#include <intrin.h>
#else
#include <x86intrin.h>
#endif
#endif

namespace Brisk {

class VLine {
public:
    VLine() = default;

    VLine(float x1, float y1, float x2, float y2) : mX1(x1), mY1(y1), mX2(x2), mY2(y2) {}

    VLine(PointF p1, PointF p2) : mX1(p1.x), mY1(p1.y), mX2(p2.x), mY2(p2.y) {}

    float length() const {
        return length(mX1, mY1, mX2, mY2);
    }

    void splitAtLength(float length, VLine& left, VLine& right) const;

    PointF p1() const {
        return { mX1, mY1 };
    }

    PointF p2() const {
        return { mX2, mY2 };
    }

    float angle() const;
    static float length(float x1, float y1, float x2, float y2);

private:
    float mX1{ 0 };
    float mY1{ 0 };
    float mX2{ 0 };
    float mY2{ 0 };
};

inline float VLine::angle() const {
    const float dx    = mX2 - mX1;
    const float dy    = mY2 - mY1;

    const float theta = std::atan2(dy, dx) * 180.0f / std::numbers::pi_v<float>;
    return theta;
}

inline BRISK_INLINE float VLine::length(float x1, float y1, float x2, float y2) {
    float x = x2 - x1;
    float y = y2 - y1;
#ifdef __SSE2__
    return _mm_cvtss_f32(_mm_rcp_ss(_mm_rsqrt_ss(_mm_set_ss(x * x + y * y))));
#else
    return std::sqrt(x * x + y * y);
#endif
#if 0
            // approximate sqrt(x*x + y*y) using alpha max plus beta min algorithm.
            // With alpha = 1, beta = 3/8, giving results with the largest error less
            // than 7% compared to the exact value.
            
                x = x < 0 ? -x : x;
                y = y < 0 ? -y : y;
            
                return (x > y ? x + 0.375f * y : y + 0.375f * x);
#endif
}

inline void VLine::splitAtLength(float lengthAt, VLine& left, VLine& right) const {
    float len = length();
    float dx  = ((mX2 - mX1) / len) * lengthAt;
    float dy  = ((mY2 - mY1) / len) * lengthAt;

    left.mX1  = mX1;
    left.mY1  = mY1;
    left.mX2  = left.mX1 + dx;
    left.mY2  = left.mY1 + dy;

    right.mX1 = left.mX2;
    right.mY1 = left.mY2;
    right.mX2 = mX2;
    right.mY2 = mY2;
}

} // namespace Brisk
