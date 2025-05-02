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
#include <brisk/widgets/DialogComponent.hpp>
#include <brisk/widgets/Button.hpp>
#include <brisk/widgets/Spacer.hpp>
#include <brisk/widgets/Text.hpp>
#include <brisk/widgets/TextEditor.hpp>
#include <brisk/widgets/Layouts.hpp>
#include <brisk/widgets/Graphene.hpp>

namespace Brisk {

DialogComponent::DialogComponent() {}

void DialogComponent::unhandledEvent(Event& event) {
    if (event.keyPressed(KeyCode::Enter)) {
        accept();
        event.stopPropagation();
    } else if (event.keyPressed(KeyCode::Escape)) {
        reject();
        event.stopPropagation();
    }
}

void DialogComponent::close(bool result) {
    bindings->assign(this->m_result, result);
    if (result) {
        accepted();
    } else {
        rejected();
    }
    closeWindow();
}

Rc<Widget> DialogComponent::dialogButtons(DialogButtons buttons, std::string okBtn, std::string cancelBtn) {
    return rcnew HLayout{ margin = { 15, 10 }, //
                          rcnew Spacer{},
                          (buttons && DialogButtons::OK)
                              ? rcnew Button{ rcnew Text{ std::move(okBtn) }, id = "dialog-ok",
                                              onClick = lifetime() |
                                                        [this] {
                                                            accept();
                                                        },
                                              margin = { 4, 0 } }
                              : nullptr,
                          (buttons && DialogButtons::Cancel)
                              ? rcnew Button{ rcnew Text{ std::move(cancelBtn) }, id = "dialog-cancel",
                                              onClick = lifetime() |
                                                        [this] {
                                                            reject();
                                                        },
                                              margin = { 4, 0 } }
                              : nullptr,
                          rcnew Spacer{} };
}

void DialogComponent::rejected() {}

void DialogComponent::accepted() {}

void DialogComponent::reject() {
    close(false);
}

void DialogComponent::accept() {
    close(true);
}

DialogComponent::~DialogComponent() {}

void DialogComponent::configureWindow(Rc<GuiWindow> window) {
    Component::configureWindow(window);
    window->setTitle("Dialog"_tr);
    window->windowFit = WindowFit::FixedSize;
    window->setStyle(WindowStyle::Dialog);
}

TextInputDialog::TextInputDialog(std::string prompt, std::string defaultValue)
    : m_prompt(std::move(prompt)), m_value(std::move(defaultValue)) {}

Rc<Widget> TextInputDialog::build() {
    return rcnew VLayout{
        stylesheet = Graphene::stylesheet(),
        Graphene::darkColors(),
        rcnew Text{ this->prompt, margin = { 15, 10 } },
        rcnew TextEditor{ Value{ &this->value }, autofocus = true, margin = { 15, 10 } },
        dialogButtons(DialogButtons::OK | DialogButtons::Cancel),
    };
}

MessageDialog::MessageDialog(std::string text, std::string icon)
    : m_text(std::move(text)), m_icon(std::move(icon)) {}

Rc<Widget> MessageDialog::build() {
    return rcnew VLayout{ stylesheet = Graphene::stylesheet(), Graphene::darkColors(),
                          rcnew Text{ this->text, margin = { 15, 10 } },
                          dialogButtons(DialogButtons::OK | DialogButtons::Cancel) };
}

ConfirmDialog::ConfirmDialog(std::string text, std::string icon)
    : m_text(std::move(text)), m_icon(std::move(icon)) {}

Rc<Widget> ConfirmDialog::build() {
    return rcnew VLayout{ stylesheet = Graphene::stylesheet(), Graphene::darkColors(),
                          rcnew Text{ this->text, margin = { 15, 10 } },
                          dialogButtons(DialogButtons::OK | DialogButtons::Cancel) };
}
} // namespace Brisk
