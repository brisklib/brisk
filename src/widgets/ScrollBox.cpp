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
#include <brisk/widgets/ScrollBox.hpp>

namespace Brisk {

ScrollBox::ScrollBox(Construction construction, Orientation orientation, ArgumentsView<ScrollBox> args)
    : Widget{ construction,
              std::tuple{
                  Arg::overflowScroll =
                      OverflowScrollBoth{ OverflowScroll::Disable, OverflowScroll::Enable }.flippedIf(
                          orientation == Orientation::Horizontal),
                  Arg::contentOverflow =
                      ContentOverflowBoth{ ContentOverflow::Default, ContentOverflow::Allow }.flippedIf(
                          orientation == Orientation::Horizontal),
                  Arg::alignItems = AlignItems::FlexStart } } {
    args.apply(this);
}

RC<Widget> ScrollBox::cloneThis() const { BRISK_CLONE_IMPLEMENTATION }

RC<Widget> HScrollBox::cloneThis() const { BRISK_CLONE_IMPLEMENTATION }

RC<Widget> VScrollBox::cloneThis() const {
    BRISK_CLONE_IMPLEMENTATION
}

} // namespace Brisk
