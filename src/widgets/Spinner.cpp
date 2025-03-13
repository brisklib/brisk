/*
 * Brisk
 *
 * Cross-platform application framework
 * --------------------------------------------------------------
 *
 * Copyright (C) 2024 Brisk Developers
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
#include <brisk/widgets/Spinner.hpp>
#include <brisk/graphics/Palette.hpp>

namespace Brisk {

static auto cosine = [](float x) -> float {
    return 0.5f * (1.f - std::cos(x * std::numbers::pi_v<float>));
};

static auto interp(float v, std::span<const float> list) -> float {
    v       = fract(v) * (list.size() - 1);
    float x = list[int(v)];
    if (fract(v) > 0)
        x = mix(cosine(fract(v)), x, list.begin()[int(v) + 1]);
    return x;
};

void spinnerPainter(Canvas& canvas_, const Widget& widget) {
    RawCanvas& canvas      = canvas_.raw();

    const Spinner* spinner = dynamic_cast<const Spinner*>(&widget);
    bool active            = spinner ? spinner->active.get() : true;
    const float time       = active ? frameStartTime * 0.25f : 0;

    RectangleF rect        = RectangleF(widget.rect())
                          .alignedRect(SizeF(widget.rect().shortestSide(), widget.rect().shortestSide()),
                                       PointF{ 0.5f, 0.5f });

    constexpr int num = 3;
    float side        = (rect.width() / num) + 0.5_dp;
    for (int i = 0; i < num + 1; ++i) {
        for (int j = 0; j < num; ++j) {
            int k        = (std::floor(time) + i) * num + j;
            auto a       = std::array{ 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 1.f };
            float factor = interp(time - float(k) / num / num, a);
            canvas.drawRectangle(rect.at((i + 0.5f) / (num), (j + 0.5f) / (num))
                                     .alignedRect(SizeF{ factor * side, factor * side }, { 0.5f, 0.5f })
                                     .withOffset({ -fract(time) * side, 0.f }),
                                 0.f, std::tuple{ fillColor = Palette::Standard::blue, strokeWidth = 0 });
        }
    }
}

void Spinner::paint(Canvas& canvas_) const {
    spinnerPainter(canvas_, *this);
}

void Spinner::start() {
    requestAnimationFrame();
}

void Spinner::onAnimationFrame() {
    Base::onAnimationFrame();
    invalidate();
    if (m_active)
        requestAnimationFrame();
}

Spinner::Spinner(Construction construction, ArgumentsView<Spinner> args) : Widget(construction, nullptr) {
    args.apply(this);
}
} // namespace Brisk
