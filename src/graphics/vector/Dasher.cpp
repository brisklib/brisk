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

#include <cmath>

#include "Dasher.hpp"
#include "Line.hpp"

namespace Brisk {

static constexpr float tolerance = 0.05f;

Dasher::Dasher(const float* dashArray, size_t size) {
    mDashArray = reinterpret_cast<const Dasher::Dash*>(dashArray);
    mArraySize = size / 2;
    if (size % 2)
        mDashOffset = dashArray[size - 1];
    mIndex         = 0;
    mCurrentLength = 0;
    mDiscard       = false;
    // if the dash array contains ZERO length
    //  segments or ZERO lengths gaps we could
    //  optimize those usecase.
    for (size_t i = 0; i < mArraySize; i++) {
        if (!vCompare(mDashArray[i].length, 0.0f))
            mNoLength = false;
        if (!vCompare(mDashArray[i].gap, 0.0f))
            mNoGap = false;
    }
}

void Dasher::moveTo(const PointF& p) {
    mDiscard         = false;
    mStartNewSegment = true;
    mCurPt           = p;
    mIndex           = 0;

    if (!vCompare(mDashOffset, 0.0f)) {
        float totalLength = 0.0;
        for (size_t i = 0; i < mArraySize; i++) {
            totalLength = mDashArray[i].length + mDashArray[i].gap;
        }
        float normalizeLen = std::fmod(mDashOffset, totalLength);
        if (normalizeLen < 0.0f) {
            normalizeLen = totalLength + normalizeLen;
        }
        // now the length is less than total length and +ve
        // findout the current dash index , dashlength and gap.
        for (size_t i = 0; i < mArraySize; i++) {
            if (normalizeLen < mDashArray[i].length) {
                mIndex         = i;
                mCurrentLength = mDashArray[i].length - normalizeLen;
                mDiscard       = false;
                break;
            }
            normalizeLen -= mDashArray[i].length;
            if (normalizeLen < mDashArray[i].gap) {
                mIndex         = i;
                mCurrentLength = mDashArray[i].gap - normalizeLen;
                mDiscard       = true;
                break;
            }
            normalizeLen -= mDashArray[i].gap;
        }
    } else {
        mCurrentLength = mDashArray[mIndex].length;
    }
    if (vIsZero(mCurrentLength))
        updateActiveSegment();
}

void Dasher::addLine(const PointF& p) {
    if (mDiscard)
        return;

    if (mStartNewSegment) {
        mResult->moveTo(mCurPt);
        mStartNewSegment = false;
    }
    mResult->lineTo(p);
}

void Dasher::updateActiveSegment() {
    mStartNewSegment = true;

    if (mDiscard) {
        mDiscard       = false;
        mIndex         = (mIndex + 1) % mArraySize;
        mCurrentLength = mDashArray[mIndex].length;
    } else {
        mDiscard       = true;
        mCurrentLength = mDashArray[mIndex].gap;
    }
    if (vIsZero(mCurrentLength))
        updateActiveSegment();
}

void Dasher::lineTo(const PointF& p) {
    VLine left, right;
    VLine line(mCurPt, p);
    float length = line.length();

    if (length <= mCurrentLength) {
        mCurrentLength -= length;
        addLine(p);
    } else {
        while (length > mCurrentLength) {
            length -= mCurrentLength;
            line.splitAtLength(mCurrentLength, left, right);

            addLine(left.p2());
            updateActiveSegment();

            line   = right;
            mCurPt = line.p1();
        }
        // handle remainder
        if (length > tolerance) {
            mCurrentLength -= length;
            addLine(line.p2());
        }
    }

    if (mCurrentLength < tolerance)
        updateActiveSegment();

    mCurPt = p;
}

void Dasher::addCubic(const PointF& cp1, const PointF& cp2, const PointF& e) {
    if (mDiscard)
        return;

    if (mStartNewSegment) {
        mResult->moveTo(mCurPt);
        mStartNewSegment = false;
    }
    mResult->cubicTo(cp1, cp2, e);
}

void Dasher::cubicTo(const PointF& cp1, const PointF& cp2, const PointF& e) {
    Bezier left, right;
    Bezier b     = Bezier::fromPoints(mCurPt, cp1, cp2, e);
    float bezLen = b.length();

    if (bezLen <= mCurrentLength) {
        mCurrentLength -= bezLen;
        addCubic(cp1, cp2, e);
    } else {
        while (bezLen > mCurrentLength) {
            bezLen -= mCurrentLength;
            b.splitAtLength(mCurrentLength, &left, &right);

            addCubic(left.pt2(), left.pt3(), left.pt4());
            updateActiveSegment();

            b      = right;
            mCurPt = b.pt1();
        }
        // handle remainder
        if (bezLen > tolerance) {
            mCurrentLength -= bezLen;
            addCubic(b.pt2(), b.pt3(), b.pt4());
        }
    }

    if (mCurrentLength < tolerance)
        updateActiveSegment();

    mCurPt = e;
}

void Dasher::dashHelper(const Path& path, Path& result) {
    mResult = &result;
    mResult->reserve(path.points().size(), path.elements().size());
    mIndex                                 = 0;
    const std::vector<Path::Element>& elms = path.elements();
    const std::vector<PointF>& pts         = path.points();
    const PointF* ptPtr                    = pts.data();

    for (auto& i : elms) {
        switch (i) {
        case Path::Element::MoveTo: {
            moveTo(*ptPtr++);
            break;
        }
        case Path::Element::LineTo: {
            lineTo(*ptPtr++);
            break;
        }
        case Path::Element::CubicTo: {
            cubicTo(*ptPtr, *(ptPtr + 1), *(ptPtr + 2));
            ptPtr += 3;
            break;
        }
        case Path::Element::Close: {
            // The point is already joined to start point in Path
            // no need to do anything here.
            break;
        }
        }
    }
    mResult = nullptr;
}

void Dasher::dashed(const Path& path, Path& result) {
    if (mNoLength && mNoGap) {
        result.reset();
        return;
    }

    if (path.empty() || mNoLength) {
        result.reset();
        return;
    }

    if (mNoGap) {
        result = path;
        return;
    }

    result.reset();

    dashHelper(path, result);
}

Path Dasher::dashed(const Path& path) {
    if (mNoLength && mNoGap)
        return path;

    if (path.empty() || mNoLength)
        return Path();

    if (mNoGap)
        return path;

    Path result;

    dashHelper(path, result);

    return result;
}

} // namespace Brisk
