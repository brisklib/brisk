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
#include <limits>
#include <vector>

#include "Rle.hpp"
#include "brisk/core/internal/Debug.hpp"

namespace Brisk {

using Result = std::array<Rle::Span, 255>;

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
    addSpans(&span, 1);
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

static inline void _opIntersectPrepare(Rle::View& a, Rle::View& b) {
    auto aPtr = a.data();
    auto aEnd = a.data() + a.size();
    auto bPtr = b.data();
    auto bEnd = b.data() + b.size();

    // 1. advance a till it intersects with b
    while ((aPtr != aEnd) && (aPtr->y < bPtr->y))
        aPtr++;

    // 2. advance b till it intersects with a
    if (aPtr != aEnd)
        while ((bPtr != bEnd) && (bPtr->y < aPtr->y))
            bPtr++;

    // update a and b object
    a = { aPtr, size_t(aEnd - aPtr) };
    b = { bPtr, size_t(bEnd - bPtr) };
}

static size_t _opIntersect(Rle::View& obj, Rle::View& clip, Result& result);

static void _opIntersect(Rle::View a, Rle::View b, Rle::RleSpanCb cb, void* userData) {
    if (!cb)
        return;

    _opIntersectPrepare(a, b);
    Result result;
    while (a.size()) {
        auto count = _opIntersect(a, b, result);
        if (count)
            cb(count, result.data(), userData);
    }
}

/*
 * This function will clip a rle list with another rle object
 * tmp_clip  : The rle list that will be use to clip the rle
 * tmp_obj   : holds the list of spans that has to be clipped
 * result    : will hold the result after the processing
 * NOTE: if the algorithm runs out of the result buffer list
 *       it will stop and update the tmp_obj with the span list
 *       that are yet to be processed as well as the tpm_clip object
 *       with the unprocessed clip spans.
 */

static size_t _opIntersect(Rle::View& obj, Rle::View& clip, Result& result) {
    auto out       = result.data();
    auto available = result.max_size();
    auto spans     = obj.data();
    auto end       = obj.data() + obj.size();
    auto clipSpans = clip.data();
    auto clipEnd   = clip.data() + clip.size();
    int sx1, sx2, cx1, cx2, x, len;

    while (available && spans < end) {
        if (clipSpans >= clipEnd) {
            spans = end;
            break;
        }
        if (clipSpans->y > spans->y) {
            ++spans;
            continue;
        }
        if (spans->y != clipSpans->y) {
            ++clipSpans;
            continue;
        }
        // assert(spans->y == (clipSpans->y + clip_offset_y));
        sx1 = spans->x;
        sx2 = sx1 + spans->len;
        cx1 = clipSpans->x;
        cx2 = cx1 + clipSpans->len;

        if (cx1 < sx1 && cx2 < sx1) {
            ++clipSpans;
            continue;
        } else if (sx1 < cx1 && sx2 < cx1) {
            ++spans;
            continue;
        }
        x   = std::max(sx1, cx1);
        len = std::min(sx2, cx2) - x;
        if (len) {
            out->x        = std::max(sx1, cx1);
            out->len      = (std::min(sx2, cx2) - out->x);
            out->y        = spans->y;
            out->coverage = divBy255(spans->coverage * clipSpans->coverage);
            ++out;
            --available;
        }
        if (sx2 < cx2) {
            ++spans;
        } else {
            ++clipSpans;
        }
    }

    // update the obj view yet to be processed
    obj  = { spans, size_t(end - spans) };

    // update the clip view yet to be processed
    clip = { clipSpans, size_t(clipEnd - clipSpans) };

    return result.max_size() - available;
}

/*
 * This function will clip a rle list with a given rect
 * clip      : The clip rect that will be use to clip the rle
 * tmp_obj   : holds the list of spans that has to be clipped
 * result    : will hold the result after the processing
 * NOTE: if the algorithm runs out of the result buffer list
 *       it will stop and update the tmp_obj with the span list
 *       that are yet to be processed
 */
static size_t _opIntersect(Rectangle clip, Rle::View& obj, Result& result) {
    auto out        = result.data();
    auto available  = result.max_size();
    auto ptr        = obj.data();
    auto end        = obj.data() + obj.size();

    const auto minx = clip.x1;
    const auto miny = clip.y1;
    const auto maxx = clip.x2 - 1;
    const auto maxy = clip.y2 - 1;

    while (available && ptr < end) {
        const auto& span = *ptr;
        if (span.y > maxy) {
            ptr = end; // update spans so that we can breakout
            break;
        }
        if (span.y < miny || span.x > maxx || span.x + span.len <= minx) {
            ++ptr;
            continue;
        }
        if (span.x < minx) {
            out->len = std::min(span.len - (minx - span.x), maxx - minx + 1);
            out->x   = minx;
        } else {
            out->x   = span.x;
            out->len = std::min(span.len, uint16_t(maxx - span.x + 1));
        }
        if (out->len != 0) {
            out->y        = span.y;
            out->coverage = span.coverage;
            ++out;
            --available;
        }
        ++ptr;
    }

    // update the span list that yet to be processed
    obj = { ptr, size_t(end - ptr) };

    return result.max_size() - available;
}

static void blitXor(Rle::Span* spans, int count, uint8_t* buffer, int offsetX) {
    while (count--) {
        int x        = spans->x + offsetX;
        int l        = spans->len;
        uint8_t* ptr = buffer + x;
        while (l--) {
            int da = *ptr;
            *ptr   = divBy255((255 - spans->coverage) * (da) + spans->coverage * (255 - da));
            ptr++;
        }
        spans++;
    }
}

static void blitDestinationOut(Rle::Span* spans, int count, uint8_t* buffer, int offsetX) {
    while (count--) {
        int x        = spans->x + offsetX;
        int l        = spans->len;
        uint8_t* ptr = buffer + x;
        while (l--) {
            *ptr = divBy255((255 - spans->coverage) * (*ptr));
            ptr++;
        }
        spans++;
    }
}

static void blitSrcOver(Rle::Span* spans, int count, uint8_t* buffer, int offsetX) {
    while (count--) {
        int x        = spans->x + offsetX;
        int l        = spans->len;
        uint8_t* ptr = buffer + x;
        while (l--) {
            *ptr = spans->coverage + divBy255((255 - spans->coverage) * (*ptr));
            ptr++;
        }
        spans++;
    }
}

void blitSrc(Rle::Span* spans, int count, uint8_t* buffer, int offsetX) {
    while (count--) {
        int x        = spans->x + offsetX;
        int l        = spans->len;
        uint8_t* ptr = buffer + x;
        while (l--) {
            *ptr = std::max(spans->coverage, *ptr);
            ptr++;
        }
        spans++;
    }
}

size_t bufferToRle(uint8_t* buffer, int size, int offsetX, int y, Rle::Span* out) {
    size_t count  = 0;
    uint8_t value = buffer[0];
    int curIndex  = 0;

    // size = offsetX < 0 ? size + offsetX : size;
    for (int i = 0; i < size; i++) {
        uint8_t curValue = buffer[0];
        if (value != curValue) {
            if (value) {
                out->y        = y;
                out->x        = offsetX + curIndex;
                out->len      = i - curIndex;
                out->coverage = value;
                out++;
                count++;
            }
            curIndex = i;
            value    = curValue;
        }
        buffer++;
    }
    if (value) {
        out->y        = y;
        out->x        = offsetX + curIndex;
        out->len      = size - curIndex;
        out->coverage = value;
        count++;
    }
    return count;
}

struct SpanMerger {
    explicit SpanMerger(Rle::Op op) {
        switch (op) {
        case Rle::Op::Add:
            _blitter = &blitSrcOver;
            break;
        case Rle::Op::Xor:
            _blitter = &blitXor;
            break;
        case Rle::Op::Substract:
            _blitter = &blitDestinationOut;
            break;
        }
    }

    using blitter = void (*)(Rle::Span*, int, uint8_t*, int);
    blitter _blitter;
    std::array<Rle::Span, 256> _result;
    std::array<uint8_t, 1024> _buffer;
    Rle::Span* _aStart{ nullptr };
    Rle::Span* _bStart{ nullptr };

    void revert(Rle::Span*& aPtr, Rle::Span*& bPtr) {
        aPtr = _aStart;
        bPtr = _bStart;
    }

    Rle::Span* data() {
        return _result.data();
    }

    size_t merge(Rle::Span*& aPtr, const Rle::Span* aEnd, Rle::Span*& bPtr, const Rle::Span* bEnd);
};

size_t SpanMerger::merge(Rle::Span*& aPtr, const Rle::Span* aEnd, Rle::Span*& bPtr, const Rle::Span* bEnd) {
    BRISK_ASSERT(aPtr->y == bPtr->y);

    _aStart = aPtr;
    _bStart = bPtr;
    int lb  = std::min(aPtr->x, bPtr->x);
    int y   = aPtr->y;

    while (aPtr < aEnd && aPtr->y == y)
        aPtr++;
    while (bPtr < bEnd && bPtr->y == y)
        bPtr++;

    int ub     = std::max((aPtr - 1)->x + (aPtr - 1)->len, (bPtr - 1)->x + (bPtr - 1)->len);
    int length = (lb < 0) ? ub + lb : ub - lb;

    if (length <= 0 || size_t(length) >= _buffer.max_size()) {
        // can't handle merge . skip
        return 0;
    }

    // clear buffer
    memset(_buffer.data(), 0, length);

    // blit a to buffer
    blitSrc(_aStart, aPtr - _aStart, _buffer.data(), -lb);

    // blit b to buffer
    _blitter(_bStart, bPtr - _bStart, _buffer.data(), -lb);

    // convert buffer to span
    return bufferToRle(_buffer.data(), length, lb, y, _result.data());
}

static size_t _opGeneric(Rle::View& a, Rle::View& b, Result& result, Rle::Op op) {
    SpanMerger merger{ op };

    auto out         = result.data();
    size_t available = result.max_size();
    auto aPtr        = const_cast<Rle::Span*>(a.data());
    auto aEnd        = a.data() + a.size();
    auto bPtr        = const_cast<Rle::Span*>(b.data());
    auto bEnd        = b.data() + b.size();

    // only logic change for substract operation.
    const bool keep  = op != (Rle::Op::Substract);

    while (available && aPtr < aEnd && bPtr < bEnd) {
        if (aPtr->y < bPtr->y) {
            *out++ = *aPtr++;
            available--;
        } else if (bPtr->y < aPtr->y) {
            if (keep) {
                *out++ = *bPtr;
                available--;
            }
            bPtr++;
        } else { // same y
            auto count = merger.merge(aPtr, aEnd, bPtr, bEnd);
            if (available >= count) {
                if (count) {
                    memcpy(out, merger.data(), count * sizeof(Rle::Span));
                    out += count;
                    available -= count;
                }
            } else {
                // not enough space try next time.
                merger.revert(aPtr, bPtr);
                break;
            }
        }
    }
    // update the span list that yet to be processed
    a = { aPtr, size_t(aEnd - aPtr) };
    b = { bPtr, size_t(bEnd - bPtr) };

    return result.max_size() - available;
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

static bool rectIntersects(Rectangle a, Rectangle b) {
    return a.x2 > b.x1 && a.x1 < b.x2 && a.y2 > b.y1 && a.y1 < b.y2;
}

static bool rectContains(Rectangle a, Rectangle b, bool proper = false) {
    return proper ? ((a.x1 < b.x1) && (a.x2 > b.x2) && (a.y1 < b.y1) && (a.y2 > b.y2))
                  : ((a.x1 <= b.x1) && (a.x2 >= b.x2) && (a.y1 <= b.y1) && (a.y2 >= b.y2));
}

void Rle::opGeneric(const Rle& aObj, const Rle& bObj, Op op) {

    // This routine assumes, obj1(span_y) < obj2(span_y).

    auto a = aObj.view();
    auto b = bObj.view();

    // reserve some space for the result vector.
    mSpans.reserve(a.size() + b.size());

    // if two rle are disjoint
    if (!rectIntersects(aObj.boundingRect(), bObj.boundingRect())) {
        if (a.data()[0].y < b.data()[0].y) {
            copy(a.data(), a.size(), mSpans);
            copy(b.data(), b.size(), mSpans);
        } else {
            copy(b.data(), b.size(), mSpans);
            copy(a.data(), a.size(), mSpans);
        }
    } else {
        auto aPtr = a.data();
        auto aEnd = a.data() + a.size();
        auto bPtr = b.data();
        auto bEnd = b.data() + b.size();

        // 1. forward a till it intersects with b
        while ((aPtr != aEnd) && (aPtr->y < bPtr->y))
            aPtr++;

        auto count = aPtr - a.data();
        if (count)
            copy(a.data(), count, mSpans);

        // 2. forward b till it intersects with a
        if (aPtr != aEnd)
            while ((bPtr != bEnd) && (bPtr->y < aPtr->y))
                bPtr++;

        count = bPtr - b.data();
        if (count)
            copy(b.data(), count, mSpans);

        // update a and b object
        a = { aPtr, size_t(aEnd - aPtr) };
        b = { bPtr, size_t(bEnd - bPtr) };

        // 3. calculate the intersect region
        Result result;

        // run till all the spans are processed
        while (a.size() && b.size()) {
            auto count = _opGeneric(a, b, result, op);
            if (count)
                copy(result.data(), count, mSpans);
        }
        // 3. copy the rest
        if (b.size())
            copy(b.data(), b.size(), mSpans);
        if (a.size())
            copy(a.data(), a.size(), mSpans);
    }

    mBboxDirty = true;
}

void Rle::opSubstract(const Rle& aObj, const Rle& bObj) {
    // if two rle are disjoint
    if (!aObj.boundingRect().intersects(bObj.boundingRect())) {
        mSpans = aObj.mSpans;
    } else {
        auto a    = aObj.view();
        auto b    = bObj.view();

        auto aPtr = a.data();
        auto aEnd = a.data() + a.size();
        auto bPtr = b.data();
        auto bEnd = b.data() + b.size();

        // 1. forward a till it intersects with b
        while ((aPtr != aEnd) && (aPtr->y < bPtr->y))
            aPtr++;
        auto count = aPtr - a.data();
        if (count)
            copy(a.data(), count, mSpans);

        // 2. forward b till it intersects with a
        if (aPtr != aEnd)
            while ((bPtr != bEnd) && (bPtr->y < aPtr->y))
                bPtr++;

        // update a and b object
        a = { aPtr, size_t(aEnd - aPtr) };
        b = { bPtr, size_t(bEnd - bPtr) };

        // 3. calculate the intersect region
        Result result;

        // run till all the spans are processed
        while (a.size() && b.size()) {
            auto count = _opGeneric(a, b, result, Op::Substract);
            if (count)
                copy(result.data(), count, mSpans);
        }

        // 4. copy the rest of a
        if (a.size())
            copy(a.data(), a.size(), mSpans);
    }

    mBboxDirty = true;
}

void Rle::opIntersect(Rle::View a, Rle::View b) {
    _opIntersectPrepare(a, b);
    Result result;
    while (a.size()) {
        auto count = _opIntersect(a, b, result);
        if (count)
            copy(result.data(), count, mSpans);
    }

    updateBbox();
}

void Rle::opIntersect(Rectangle r, Rle::RleSpanCb cb, void* userData) const {

    if (empty())
        return;

    if (rectContains(r, boundingRect())) {
        cb(mSpans.size(), mSpans.data(), userData);
        return;
    }

    auto obj = view();
    Result result;
    // run till all the spans are processed
    while (obj.size()) {
        auto count = _opIntersect(r, obj, result);
        if (count)
            cb(count, result.data(), userData);
    }
}

inline bool boolOp(Rle::BinOp op, bool a, bool b) {
    switch (op) {
    case Rle::BinOp::And:
        return a && b;
    case Rle::BinOp::AndNot:
        return a && !b;
    case Rle::BinOp::Or:
        return a || b;
    case Rle::BinOp::Xor:
        return a != b;
    default:
        BRISK_UNREACHABLE();
    }
}

inline uint8_t coverageOp(Rle::BinOp op, uint8_t a, uint8_t b) {
    switch (op) {
    case Rle::BinOp::And:
        return divBy255(a * b);
    case Rle::BinOp::AndNot:
        return divBy255(a * (255 - b));
    case Rle::BinOp::Or:
        return a + b - divBy255(a * b);
    case Rle::BinOp::Xor:
        return a + b - 2 * divBy255(a * b);
    default:
        BRISK_UNREACHABLE();
    }
}

static bool spansIntersects(Rle::View a, Rle::View b) {
    return !a.empty() && !b.empty() &&
           Range<int, true>(a.front().y, a.back().y).intersects(Range<int, true>(b.front().y, b.back().y));
}

Rle Rle::binary(const Rle& left, const Rle& right, BinOp op) {
    if (left.empty() && right.empty())
        return {};

    bool singleLeft  = boolOp(op, true, false);
    bool singleRight = boolOp(op, false, true);

    if (left.empty()) {
        if (singleRight) {
            return right;
        } else {
            return {};
        }
    }
    if (right.empty()) {
        if (singleLeft) {
            return left;
        } else {
            return {};
        }
    }

    if (!left.boundingRect().intersects(right.boundingRect())) {
        // if two rle are disjoint
        switch (op) {
        case BinOp::And:
            return {};
        case BinOp::AndNot:
            return left;
        case BinOp::Or:
        case BinOp::Xor: {
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

    for (; !l.empty() & !r.empty();) {
        if (!l.front().before(r.front()) && !r.front().before(l.front())) {
            // spans intersect
            int16_t x = std::min(l.front().x, r.front().x);
            for (;;) {
                if (l.front().x <= x && r.front().x <= x) {
                    int16_t endX = std::min(l.front().end(), r.front().end());
                    if (boolOp(op, true, true)) {
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
                        // add the span from the view that is not intersecting
                        result.addSpan(view.front().slice(x, other.front().x));
                    }
                    x = other.front().x;
                }
                if (l.front().end() == x) {
                    l = l.subspan(1);
                }
                if (r.front().end() == x) {
                    r = r.subspan(1);
                }
                if (l.empty() || r.empty()) {
                    break; // only one span left
                    // this also breaks the outer loop
                }
                if (l.front().x > x && r.front().x > x) {
                    break; // no more intersection
                }
            }
        } else {
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
                    result.addSpans(view.subspan(0, c));
                }
                view = view.subspan(c);
                if (view.empty())
                    break;
            }
        }
    }

    // One of the rle is empty
    if (l.empty()) {
        if (singleRight) {
            result.addSpans(r);
        }
        return result;
    } else { // if (r.empty())
        if (singleLeft) {
            result.addSpans(l);
        }
        return result;
    }
}

} // namespace Brisk
