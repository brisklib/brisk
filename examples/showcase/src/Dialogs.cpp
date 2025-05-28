/*
 * Brisk
 *
 * Cross-platform application framework
 * --------------------------------------------------------------
 *
 * Copyright (C) 2025 Brisk Developers
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
#include "Dialogs.hpp"
#include <brisk/gui/Component.hpp>
#include <brisk/widgets/DialogComponent.hpp>
#include <brisk/window/OsDialogs.hpp>
#include <brisk/widgets/Graphene.hpp>

namespace Brisk {

static Rc<Widget> osDialogButton(std::string text, BindableCallback<> fn, Value<bool> globalEnabled) {
    return rcnew HLayout{
        rcnew Button{
            rcnew Text{ std::move(text) },
            enabled = globalEnabled,
            onClick = std::move(fn),
        },
    };
}

class SmallComponent : public Component {
public:
    ~SmallComponent() override = default;

    Rc<Widget> build() final {
        return rcnew Widget{
            stylesheet = Graphene::stylesheet(),
            rcnew Spacer{},
            rcnew Text{
                "Separate window based on Brisk::Component",
                flexGrow  = 1,
                textAlign = TextAlign::Center,
            },
            rcnew Spacer{},
        };
    }
};

Rc<Widget> ShowcaseDialogs::build(Rc<Notifications> notifications, Value<bool> globalEnabled) {
    return rcnew VLayout{
        flexGrow = 1,
        padding  = 16_apx,
        gapRow   = 8_apx,

        rcnew Text{ "Multiple windows (gui/Component.hpp)", classes = { "section-header" } },

        rcnew HLayout{
            rcnew Button{
                rcnew Text{ "Open window" },
                enabled = globalEnabled,
                onClick = lifetime() |
                          [this]() {
                              Rc<SmallComponent> comp = rcnew SmallComponent();
                              windowApplication->addWindow(comp->makeWindow());
                          },
            },

            rcnew Button{
                rcnew Text{ "Open modal window" },
                enabled = globalEnabled,
                onClick = lifetime() |
                          [this]() {
                              Rc<SmallComponent> comp = rcnew SmallComponent();
                              windowApplication->showModalWindow(comp->makeWindow());
                          },
            },
        },
        rcnew HLayout{
            rcnew Button{
                rcnew Text{ "TextInputDialog" },
                enabled = globalEnabled,
                onClick = lifetime() |
                          []() {
                              Rc<TextInputDialog> dialog = rcnew TextInputDialog{ "Enter name", "World" };
                              windowApplication->showModalWindow(dialog->makeWindow());
                              if (dialog->result)
                                  Shell::showMessage("title", "Hello, " + dialog->value.get(),
                                                     MessageBoxType::Info);
                              else
                                  Shell::showMessage("title", "Hello, nobody", MessageBoxType::Warning);
                          },
            },
        },

        rcnew Text{ "PopupDialog (widgets/PopupDialog.hpp)", classes = { "section-header" } },

        rcnew HLayout{
            rcnew Button{
                rcnew Text{ "Open Dialog" },
                enabled = globalEnabled,
                onClick = lifetime() |
                          [this]() {
                              bindings->assign(m_popupDialog, true);
                          },
            },
            rcnew PopupOKDialog{
                "Dialog title",
                Value{ &m_popupDialog },
                [notifications]() {
                    notifications->show(rcnew Text{ "Dialog closed" });
                },
                rcnew Text{ "Dialog" },
            },
        },

        rcnew Text{ "OS dialogs (window/OsDialogs.hpp)", classes = { "section-header" } },
        osDialogButton(
            "Open URL",
            lifetime() |
                []() {
                    Shell::openURLInBrowser("https://www.brisklib.com/");
                },
            globalEnabled),
        osDialogButton(
            "Open folder",
            lifetime() |
                []() {
                    Shell::openFolder(defaultFolder(DefaultFolder::Documents));
                },
            globalEnabled),

        osDialogButton(
            "Message box (Info)",
            lifetime() |
                []() {
                    Shell::showMessage("title", "message", MessageBoxType::Info);
                },
            globalEnabled),
        osDialogButton(
            "Message box (Warning)",
            lifetime() |
                []() {
                    Shell::showMessage("title", "message", MessageBoxType::Warning);
                },
            globalEnabled),
        osDialogButton(
            "Message box (Error)",
            lifetime() |
                []() {
                    Shell::showMessage("title", "message", MessageBoxType::Error);
                },
            globalEnabled),
        osDialogButton(
            "Dialog (OK, Cancel)",
            lifetime() |
                [this]() {
                    if (Shell::showDialog("title", "message", DialogButtons::OKCancel,
                                          MessageBoxType::Info) == DialogResult::OK)
                        m_text += "OK clicked\n";
                    else
                        m_text += "Cancel clicked\n";
                    bindings->notify(&m_text);
                },
            globalEnabled),
        osDialogButton(
            "Dialog (Yes, No, Cancel)",
            lifetime() |
                [this]() {
                    if (DialogResult r = Shell::showDialog("title", "message", DialogButtons::YesNoCancel,
                                                           MessageBoxType::Warning);
                        r == DialogResult::Yes)
                        m_text += "Yes clicked\n";
                    else if (r == DialogResult::No)
                        m_text += "No clicked\n";
                    else
                        m_text += "Cancel clicked\n";
                    bindings->notify(&m_text);
                },
            globalEnabled),
        osDialogButton(
            "Open File",
            lifetime() |
                [this]() {
                    auto file = Shell::showOpenDialog({ { "*.txt", "Text files" } },
                                                      defaultFolder(DefaultFolder::Documents));
                    if (file)
                        m_text += file->string() + "\n";
                    else
                        m_text += "(nullopt)\n";
                    bindings->notify(&m_text);
                },
            globalEnabled),
        osDialogButton(
            "Open Files",
            lifetime() |
                [this]() {
                    auto files = Shell::showOpenDialogMulti({ { "*.txt", "Text files" }, Shell::anyFile() },
                                                            defaultFolder(DefaultFolder::Documents));
                    for (fs::path file : files)
                        m_text += file.string() + "\n";
                    bindings->notify(&m_text);
                },
            globalEnabled),
        osDialogButton(
            "Pick folder",
            lifetime() |
                [this]() {
                    auto folder = Shell::showFolderDialog(defaultFolder(DefaultFolder::Documents));
                    if (folder)
                        m_text += folder->string() + "\n";
                    else
                        m_text += "(nullopt)\n";
                    bindings->notify(&m_text);
                },
            globalEnabled),
        rcnew Text{
            text       = Value{ &m_text },
            fontFamily = Font::Monospace,
        },
    };
}
} // namespace Brisk
