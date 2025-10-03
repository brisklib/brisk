/*
 * Copyright (c) 2020 Samsung Electronics Co., Ltd. All rights reserved.

 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#pragma once

#include <cstdint>
#include <vector>

#include "Common.hpp"

namespace Brisk {

class Rle {
public:
    struct Span {
        int16_t x{ 0 };
        int16_t y{ 0 };
        uint16_t len{ 0 };
        uint8_t coverage{ 0 };

        bool operator==(const Span& other) const noexcept = default;
    };

    using View = std::span<const Span>;

    bool empty() const {
        return mSpans.empty();
    }

    Rectangle boundingRect() const;

    void setBoundingRect(Rectangle bbox);

    void addSpans(const Rle::Span* span, size_t count);
    void addSpans(std::span<const Rle::Span> spans);
    void addSpan(Rle::Span span);

    void reset();

    void translate(Point p);

    const std::vector<Rle::Span>& spans() const {
        return mSpans;
    }

    void addRect(Rectangle rect);

private:
    void updateBbox() const;

    std::vector<Rle::Span> mSpans;
    Point mOffset;
    mutable Rectangle mBbox;
    mutable bool mBboxDirty = true;
};
} // namespace Brisk
