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

#include <brisk/graphics/Geometry.hpp>
#include <brisk/graphics/Path.hpp>

namespace Brisk {

class Dasher {
public:
    Dasher(const float* dashArray, size_t size);
    Path dashed(const Path& path);
    void dashed(const Path& path, Path& result);

private:
    void moveTo(const PointF& p);
    void lineTo(const PointF& p);
    void cubicTo(const PointF& cp1, const PointF& cp2, const PointF& e);
    void close();
    void addLine(const PointF& p);
    void addCubic(const PointF& cp1, const PointF& cp2, const PointF& e);
    void updateActiveSegment();

private:
    void dashHelper(const Path& path, Path& result);

    struct Dash {
        float length;
        float gap;
    };

    const Dasher::Dash* mDashArray;
    size_t mArraySize{ 0 };
    PointF mCurPt;
    size_t mIndex{ 0 }; /* index to the dash Array */
    float mCurrentLength;
    float mDashOffset{ 0 };
    Path* mResult{ nullptr };
    bool mDiscard{ false };
    bool mStartNewSegment{ true };
    bool mNoLength{ true };
    bool mNoGap{ true };
};

} // namespace Brisk
