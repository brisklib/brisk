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
#include <brisk/widgets/Progress.hpp>

namespace Brisk {

void paintProgressIndicator(Canvas& canvas, RectangleF rect, int circles) {
    for (int i = 0; i < circles; ++i) {
        const float t = frameStartTime * std::numbers::pi_v<float> * 2 * 0.25f * (1 + i);
        const float s = std::sin(t);
        const float c = std::cos(t);
        canvas.raw().drawArc(PointF(rect.center()), idp(20 + i * 5), idp(15 + i * 5), 0, 10.f,
                             std::tuple{ fillColors    = { ColorF{ 1, 1, 1, 0 }, ColorF{ 0, 0, 0, 0 } },
                                         angleGradient = {
                                             rect.center(),
                                             PointF(rect.center()) + PointOf<float>{ c, s },
                                         } });
    }
}

void ProgressBar::updateValue() {
    if (Progress* progress = dynamic_cast<Progress*>(parent())) {
        float value = progress->normalizedValue.get();
        setRect(progress->clientRect().slice(progress->clientRect().orientation(), 0.f, value));
    }
}

void ProgressBar::onLayoutUpdated() {
    Base::onLayoutUpdated();
    updateValue();
}

void Progress::onLayoutUpdated() {
    Base::onLayoutUpdated();
    if (auto bar = find<ProgressBar>(MatchAny{}))
        bar->updateValue();
}

void Progress::onChanged() {
    Base::onChanged();
    if (auto bar = find<ProgressBar>(MatchAny{}))
        bar->updateValue();
}

Progress::Progress(Construction construction, ArgumentsView<Progress> args) : Base(construction, nullptr) {
    m_minimum = 0;
    m_maximum = 1;
    args.apply(this);
    if (!find<ProgressBar>(MatchAny{})) {
        apply(rcnew ProgressBar{});
    }
}
} // namespace Brisk
