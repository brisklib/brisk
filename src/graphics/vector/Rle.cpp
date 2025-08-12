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
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fmt/base.h>
#include <limits>
#include <vector>

#include "Rle.hpp"
#include "brisk/core/internal/Debug.hpp"
#include "brisk/core/Log.hpp"

namespace Brisk {

static inline uint8_t divBy255(int x) {
    return (x + (x >> 8) + 0x80) >> 8;
}

inline static void copy(const Rle::Span* span, size_t count, std::vector<Rle::Span>& v) {
    // make sure enough memory available
    if (v.capacity() < v.size() + count)
        v.reserve(v.size() + count);
    std::copy(span, span + count, back_inserter(v));
}

void Rle::addSpans(const Rle::Span* span, size_t count) {
    copy(span, count, mSpans);
    mBboxDirty = true;
}

void Rle::addSpans(std::span<const Rle::Span> spans) {
    addSpans(spans.data(), spans.size());
}

void Rle::addSpan(Rle::Span span) {
    if (!mSpans.empty() && mSpans.back().y == span.y && mSpans.back().end() == span.x &&
        mSpans.back().coverage == span.coverage) {
        // merge with last span
        mSpans.back().len += span.len;
    } else {
        mSpans.push_back(span);
    }
    mBboxDirty = true;
}

Rectangle Rle::boundingRect() const {
    updateBbox();
    return mBbox;
}

void Rle::setBoundingRect(Rectangle bbox) {
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
        mBbox = Rectangle(l, t, r, b + 1);
    }
}

void Rle::addRect(Rectangle rect) {
    int x      = rect.x1;
    int y      = rect.y1;
    int width  = rect.width();
    int height = rect.height();

    mSpans.reserve(size_t(height));

    Rle::Span span;
    for (int i = 0; i < height; i++) {
        span.x        = x;
        span.y        = y + i;
        span.len      = width;
        span.coverage = 255;
        mSpans.push_back(span);
    }
    mBbox = rect;
}

inline bool boolOp(MaskOp op, bool a, bool b) {
    switch (op) {
    case MaskOp::And:
        return a && b;
    case MaskOp::AndNot:
        return a && !b;
    case MaskOp::Or:
        return a || b;
    case MaskOp::Xor:
        return a != b;
    default:
        BRISK_UNREACHABLE();
    }
}

inline uint8_t coverageOp(MaskOp op, uint8_t a, uint8_t b) {
    switch (op) {
    case MaskOp::And:
        return divBy255(a * b);
    case MaskOp::AndNot:
        return divBy255(a * (255 - b));
    case MaskOp::Or:
        return a + b - divBy255(a * b);
    case MaskOp::Xor:
        return a + b - 2 * divBy255(a * b);
    default:
        BRISK_UNREACHABLE();
    }
}

Rle Rle::binary(const Rle& left, const Rle& right, MaskOp op) {
    if (left.empty() && right.empty())
        return {};

    bool singleLeft  = boolOp(op, true, false);
    bool singleRight = boolOp(op, false, true);

#ifdef RLE_DEBUG
#define PRINTLN(fmtstr, ...) fmt::println(fmtstr, __VA_ARGS__)
#else
#define PRINTLN(fmtstr, ...)                                                                                 \
    do {                                                                                                     \
    } while (0)
#endif

    switch (op) {
    case MaskOp::And:
        PRINTLN("Rle::binary: performing AND operation");
        break;
    case MaskOp::AndNot:
        PRINTLN("Rle::binary: performing AND NOT operation");
        break;
    case MaskOp::Or:
        PRINTLN("Rle::binary: performing OR operation");
        break;
    case MaskOp::Xor:
        PRINTLN("Rle::binary: performing XOR operation");
        break;
    default:
        BRISK_UNREACHABLE();
    }

    if (left.empty()) {
        if (singleRight) {
            PRINTLN("Rle::binary: left is empty, returning right");
            return right;
        } else {
            PRINTLN("Rle::binary: left is empty, returning empty Rle");
            return {};
        }
    }
    if (right.empty()) {
        if (singleLeft) {
            PRINTLN("Rle::binary: right is empty, returning left");
            return left;
        } else {
            PRINTLN("Rle::binary: right is empty, returning empty Rle");
            return {};
        }
    }

    if (!left.boundingRect().intersects(right.boundingRect())) {
        PRINTLN("Rle::binary: bounding rectangles do not intersect");
        // if two rle are disjoint
        switch (op) {
        case MaskOp::And:
            PRINTLN("Rle::binary: returning empty Rle for AND operation");
            return {};
        case MaskOp::AndNot:
            PRINTLN("Rle::binary: returning left for AND NOT operation");
            return left;
        case MaskOp::Or:
        case MaskOp::Xor: {
            PRINTLN("Rle::binary: returning merged Rle for OR/XOR operation");
            // merge the two rle objects
            Rle result;
            result.mSpans.reserve(left.mSpans.size() + right.mSpans.size());
            if (left.mSpans.front().y < right.mSpans.front().y) { // keep sorted by y
                copy(left.mSpans.data(), left.mSpans.size(), result.mSpans);
                copy(right.mSpans.data(), right.mSpans.size(), result.mSpans);
            } else {
                copy(right.mSpans.data(), right.mSpans.size(), result.mSpans);
                copy(left.mSpans.data(), left.mSpans.size(), result.mSpans);
            }
            return result;
        }
        default:
            BRISK_UNREACHABLE();
        }
    }

    Rle result;
    auto l = left.view();
    auto r = right.view();

#undef PRINTLN
#ifdef RLE_DEBUG
#define PRINTLN(fmtstr, ...)                                                                                 \
    do {                                                                                                     \
        fmt::println("    l={} r={} o={}", fmt::join(l, " "), fmt::join(r, " "),                             \
                     fmt::join(result.mSpans, " "));                                                         \
        fmt::println(fmtstr, __VA_ARGS__);                                                                   \
    } while (0)
#else
#define PRINTLN(fmtstr, ...)                                                                                 \
    do {                                                                                                     \
    } while (0)
#endif

    PRINTLN("Rle::binary: starting binary operation with spans intersecting");

    for (; !l.empty() && !r.empty();) {
        if (!l.front().before(r.front()) && !r.front().before(l.front())) {
            // spans intersect
            int16_t y = l.front().y;
            int16_t x = std::min(l.front().x, r.front().x);
            BRISK_ASSERT(l.front().y == r.front().y);
            PRINTLN("Rle::binary: spans intersect at y = {} x = {}", l.front().y, x);
            for (;;) {
                if (l.front().x <= x && r.front().x <= x) {
                    PRINTLN("Rle::binary: spans intersect at x = {}", x);
                    int16_t endX = std::min(l.front().end(), r.front().end());
                    if (boolOp(op, true, true)) {
                        PRINTLN("Rle::binary: adding span at x = {} with length = {} and coverage = {}", x,
                                endX - x, coverageOp(op, l.front().coverage, r.front().coverage));
                        // add the span with the coverage operation
                        result.addSpan({ x, l.front().y, static_cast<uint16_t>(endX - x),
                                         coverageOp(op, l.front().coverage, r.front().coverage) });
                    }
                    x = endX;
                } else {
                    bool isLeft = l.front().x < r.front().x;
                    View& view  = isLeft ? l : r;
                    View& other = isLeft ? r : l;
                    bool single = isLeft ? singleLeft : singleRight;

                    if (single) {
                        PRINTLN("Rle::binary: adding span from {} at x = {}", isLeft ? "left" : "right", x);
                        // add the span from the view that is not intersecting
                        result.addSpan(view.front().slice(x, other.front().x));
                    }
                    x = other.front().x;
                }
                if (l.front().end() == x) {
                    PRINTLN("Rle::binary: skipping span from left at x = {} y = {}", x, l.front().y);
                    l = l.subspan(1);
                }
                if (r.front().end() == x) {
                    PRINTLN("Rle::binary: skipping span from right at x = {} y = {}", x, r.front().y);
                    r = r.subspan(1);
                }
                if (l.empty() && r.empty()) {
                    PRINTLN("Rle::binary: both spans are empty, breaking out");
                    break;
                    // this also breaks the outer loop
                }
                if (r.empty() && !l.empty()) {
                    if (singleLeft) {
                        PRINTLN("Rle::binary: adding remaining left span at y = {}", l.front().y);
                        result.addSpan(l.front().slice(x));
                    }
                    PRINTLN("Rle::binary: skipping remaining left span at y = {}", l.front().y);
                    l = l.subspan(1);
                    break;
                }
                if (l.empty() && !r.empty()) {
                    if (singleRight) {
                        PRINTLN("Rle::binary: adding remaining right span at y = {}", r.front().y);
                        result.addSpan(r.front().slice(x));
                    }
                    PRINTLN("Rle::binary: skipping remaining right span at y = {}", r.front().y);
                    r = r.subspan(1);
                    break;
                }
                if (l.front().y != y) {
                    PRINTLN("Rle::binary: left span y changed to {}, breaking out", l.front().y);
                    break;
                }
                if (r.front().y != y) {
                    PRINTLN("Rle::binary: right span y changed to {}, breaking out", r.front().y);
                    break;
                }
                if (l.front().x > x && r.front().x > x) {
                    PRINTLN("Rle::binary: no more intersection at x = {}, breaking out", x);
                    break; // no more intersection
                }
            }
        } else {
            PRINTLN("Rle::binary: spans do not intersect at y = {}", l.front().y);
            // spans do not intersect
            bool isLeft = l.front().before(r.front());
            View& view  = isLeft ? l : r;
            View& other = isLeft ? r : l;
            bool single = isLeft ? singleLeft : singleRight;

            size_t c    = 0;
            while (c < view.size() && view[c].before(other.front())) {
                c++;
            }
            if (c) {
                // add the spans from the view that is not intersecting
                if (single) {
                    PRINTLN("Rle::binary: adding {} spans from {} at y = {}", c, isLeft ? "left" : "right",
                            view.front().y);
                    result.addSpans(view.subspan(0, c));
                }
                PRINTLN("Rle::binary: skipping {} spans from {} at y = {}", c, isLeft ? "left" : "right",
                        view.front().y);
                view = view.subspan(c);
                if (view.empty()) {
                    PRINTLN("Rle::binary: {} spans are empty, breaking out", isLeft ? "left" : "right");
                    break;
                }
            }
        }
    }

    // One of the rle is empty
    if (l.empty()) {
        if (singleRight) {
            PRINTLN("Rle::binary: left is empty, adding {} right spans", r.size());
            result.addSpans(r);
        }
        return result;
    } else { // if (r.empty())
        if (singleLeft) {
            PRINTLN("Rle::binary: right is empty, adding {} left spans", l.size());
            result.addSpans(l);
        }
        return result;
    }
}

} // namespace Brisk
