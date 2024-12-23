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
#include <brisk/widgets/ContextPopup.hpp>
#include <brisk/widgets/Item.hpp>

namespace Brisk {

RC<Widget> ContextPopup::cloneThis() const { BRISK_CLONE_IMPLEMENTATION }

ContextPopup::ContextPopup(Construction construction, ArgumentsView<ContextPopup> args)
    : PopupBox(construction,
               std::tuple{ Arg::absolutePosition = { 0, 0 }, Arg::tabGroup = true, Arg::visible = false }) {
    args.apply(this);
}

void ContextPopup::append(RC<Widget> widget) {
    if (Item* it = dynamic_cast<Item*>(widget.get())) {
        it->dynamicFocus = true;
        Base::append(std::move(widget));
    } else {
        Base::append(rcnew Item{ std::move(widget), dynamicFocus = true });
    }
}
} // namespace Brisk
