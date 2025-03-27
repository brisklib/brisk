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
#pragma once

#include <brisk/gui/GUI.hpp>

namespace Brisk {

class WIDGET Spinner : public Widget {
    BRISK_DYNAMIC_CLASS(Spinner, Widget)
public:
    using Base                                   = Widget;
    constexpr static std::string_view widgetType = "spinner";

    template <WidgetArgument... Args>
    explicit Spinner(const Args&... args) : Spinner{ Construction{ widgetType }, std::tuple{ args... } } {
        endConstruction();
    }

protected:
    bool m_active = false;
    void onAnimationFrame() override;
    void paint(Canvas& canvas) const override;
    void start();
    explicit Spinner(Construction construction, ArgumentsView<Spinner> args);

public:
    BRISK_PROPERTIES_BEGIN
    Property<Spinner, bool, &Spinner::m_active, nullptr, nullptr, &Spinner::start> active;
    BRISK_PROPERTIES_END
};

template <typename T>
void applier(Spinner* target, ArgVal<Tag::Named<"active">, T> value) {
    target->active = value.value;
}

inline namespace Arg {
constexpr inline Argument<Tag::Named<"active">> active{};
}

void spinnerPainter(Canvas& canvas, const Widget& widget);

} // namespace Brisk
