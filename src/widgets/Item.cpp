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
#include <brisk/widgets/Item.hpp>
#include <brisk/gui/Icons.hpp>

namespace Brisk {

void Item::postPaint(Canvas& canvas) const {
    Base::postPaint(canvas);
    std::string icon = m_checked ? ICON_check : m_icon;
    if (!icon.empty()) {
        canvas.raw().drawText(
            m_rect.alignedRect({ m_clientRect.x1 - m_rect.x1, m_rect.height() }, { 0.f, m_iconAlignY }), 0.5f,
            0.5f, icon, font()(dp(FontSize::Normal)), m_color.current);
    }
}

void Item::onEvent(Event& event) {
    Base::onEvent(event);
    if (event.released(m_rect) || event.keyPressed(KeyCode::Enter) || event.keyPressed(KeyCode::Space)) {
        if (m_checkable) {
            checked = !checked;
        }
        onClicked();
        m_onClick.trigger();
        if (m_closesPopup) {
            closeNearestPopup();
        }
        event.stopPropagation();
    }

    if (m_dynamicFocus) {
        if (auto e = event.as<EventMouseMoved>()) {
            focus();
        } else if (auto e = event.as<EventMouseExited>()) {
            toggleState(WidgetState::Selected, false);
        } else if (event.focused()) {
            toggleState(WidgetState::Selected, true);
        } else if (event.blurred()) {
            toggleState(WidgetState::Selected, false);
        }
    }
}

Widget::Ptr Item::cloneThis() const { BRISK_CLONE_IMPLEMENTATION }

Item::Item(Construction construction, ArgumentsView<Item> args)
    : Base(construction, std::tuple{ Arg::tabStop = true }) {
    m_processClicks = false;
    args.apply(this);
}

void Item::onChanged() {}

void Item::onClicked() {}
} // namespace Brisk
