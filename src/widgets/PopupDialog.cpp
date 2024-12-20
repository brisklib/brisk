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
#include <brisk/widgets/PopupDialog.hpp>
#include <brisk/widgets/Button.hpp>
#include <brisk/widgets/Spacer.hpp>
#include <brisk/widgets/Text.hpp>
#include <brisk/widgets/Layouts.hpp>

namespace Brisk {

Widget::Ptr PopupDialog::cloneThis() const { BRISK_CLONE_IMPLEMENTATION }

PopupDialog::PopupDialog(Construction construction, Value<bool> visibleProp, ArgumentsView<Widget> args)
    : Widget{
          construction,
          std::tuple{
              Arg::visible          = std::move(visibleProp),
              Arg::placement        = Placement::Window,
              Arg::layout           = Layout::Vertical,
              Arg::dimensions       = { 100_perc, 100_perc },
              Arg::absolutePosition = { 0, 0 },
              Arg::anchor           = { 0, 0 },
              Arg::zorder           = ZOrder::TopMost,
              rcnew Spacer{},
              rcnew Widget{
                  Arg::classes   = { "dialog" },
                  Arg::layout    = Layout::Vertical,
                  Arg::alignSelf = AlignSelf::Center,
                  asAttributes(args),
              },
              rcnew Spacer{},
          },
      } {}

Widget::Ptr PopupOKDialog::cloneThis() const { BRISK_CLONE_IMPLEMENTATION }

PopupOKDialog::PopupOKDialog(Construction construction, std::string title, Value<bool> visibleProp,
                             VoidFunc accepted, ArgumentsView<Widget> args)
    : PopupDialog{
          construction,
          visibleProp,
          std::tuple{
              rcnew Text{
                  std::move(title),
                  Arg::classes = { "dialog-title" },
              },
              rcnew VLayout{
                  Arg::classes = { "dialog-body" },
                  asAttributes(args),
                  rcnew Button{ rcnew Text{ "OK" }, Arg::classes = { "dialog-button" },
                                Arg::alignSelf = AlignSelf::Center,
                                Arg::onClick   = listener(
                                    [accepted = std::move(accepted), visibleProp]() {
                                        visibleProp.set(false);
                                        accepted();
                                    },
                                    this) },
              },
          },
      } {}

} // namespace Brisk
