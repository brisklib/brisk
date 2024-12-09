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
#include <brisk/widgets/Pages.hpp>

namespace Brisk {

void Tabs::clearTabs() {
    clear();
}

void Tabs::createTab(Value<bool> visible, Page* page) {
    apply(new TabButton(Arg::value = std::move(visible), new Text{ Arg::text = page->title }));
}

void Pages::updateTabs() {
    std::shared_ptr<Tabs> tabs = this->tabs.get(this);
    if (!tabs)
        return;
    tabs->clearTabs();
    int index = 0;
    for (Widget::Ptr w : *this) {
        if (Page* p = dynamic_cast<Page*>(w.get())) {
            auto prop = Value{ &this->value } == index;
            tabs->createTab(prop, p);
            ++index;
        }
    }
    onChanged();
}

void Pages::childrenAdded() {
    Base::childrenAdded();
    updateTabs();
}

void Pages::onChanged() {
    if (m_value == Horizontal) {
        layout = Layout::Horizontal;
    } else if (m_value == Vertical) {
        layout = Layout::Vertical;
    }
    int index = 0;
    for (Widget::Ptr w : *this) {
        if (Page* p = dynamic_cast<Page*>(w.get())) {
            p->visible = m_value < 0 || m_value == index;
            ++index;
        }
    }
}

TabButton::TabButton(Construction construction, ArgumentsView<TabButton> args) : Base(construction, nullptr) {
    args.apply(this);
}

Widget::Ptr TabButton::cloneThis() {
    BRISK_CLONE_IMPLEMENTATION;
}

Widget::Ptr Pages::cloneThis() {
    BRISK_CLONE_IMPLEMENTATION;
}

Widget::Ptr Page::cloneThis() {
    BRISK_CLONE_IMPLEMENTATION;
}

Widget::Ptr Tabs::cloneThis() {
    BRISK_CLONE_IMPLEMENTATION;
}

Tabs::Tabs(Construction construction, ArgumentsView<Tabs> args)
    : Base{ construction, std::tuple{ Arg::tabGroup = true } } {
    args.apply(this);
}

Page::Page(Construction construction, std::string title, ArgumentsView<Page> args)
    : Base(construction, std::tuple{ Arg::layout = Layout::Horizontal, Arg::flexGrow = true,
                                     Arg::alignItems = AlignItems::Stretch }),
      m_title(std::move(title)) {
    args.apply(this);
}

Pages::Pages(Construction construction, ArgumentsView<Pages> args)
    : Base(construction,
           std::tuple{ Arg::layout = Layout::Vertical, Arg::alignItems = AlignItems::Stretch }) {
    args.apply(this);
    updateTabs();
}

void Pages::onConstructed() {
    Base::onConstructed();
    tabs.create(this);
}

} // namespace Brisk
