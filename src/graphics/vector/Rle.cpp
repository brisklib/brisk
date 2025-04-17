/*
 * Copyright (c) 2020 Samsung Electronics Co., Ltd. All rights reserved.

 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in
 all
 * copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <algorithm>
#include <array>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <vector>

#include "Rle.hpp"

namespace Brisk {

inline static void copy(const Rle::Span* span, size_t count, std::vector<Rle::Span>& v) {
    // make sure enough memory available
    if (v.capacity() < v.size() + count)
        v.reserve(v.size() + count);
    std::copy(span, span + count, back_inserter(v));
}

void Rle::addSpan(const Rle::Span* span, size_t count) {
    copy(span, count, mSpans);
    mBboxDirty = true;
}

Rectangle Rle::boundingRect() const {
    updateBbox();
    return mBbox;
}

void Rle::setBoundingRect(const Rectangle& bbox) {
    mBboxDirty = false;
    mBbox      = bbox;
}

void Rle::reset() {
    mSpans.clear();
    mBbox      = Rectangle();
    mOffset    = Point();
    mBboxDirty = false;
}

void Rle::translate(Point p) {
    // take care of last offset if applied
    mOffset = p - mOffset;
    int x   = mOffset.x;
    int y   = mOffset.y;
    for (auto& i : mSpans) {
        i.x = i.x + x;
        i.y = i.y + y;
    }
    updateBbox();
    mBbox = mBbox.withOffset(mOffset);
}

void Rle::updateBbox() const {
    if (!mBboxDirty)
        return;

    mBboxDirty            = false;

    int l                 = std::numeric_limits<int>::max();
    const Rle::Span* span = mSpans.data();

    mBbox                 = Rectangle();
    size_t sz             = mSpans.size();
    if (sz) {
        int t = span[0].y;
        int b = span[sz - 1].y;
        int r = 0;
        for (size_t i = 0; i < sz; i++) {
            if (span[i].x < l)
                l = span[i].x;
            if (span[i].x + span[i].len > r)
                r = span[i].x + span[i].len;
        }
        mBbox = Rectangle(l, t, r - l, b - t + 1);
    }
}

} // namespace Brisk
