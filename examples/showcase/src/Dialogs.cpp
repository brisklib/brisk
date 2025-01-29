#include "Dialogs.hpp"
#include "brisk/gui/Component.hpp"
#include "brisk/widgets/DialogComponent.hpp"
#include <brisk/window/OSDialogs.hpp>
#include <brisk/widgets/Graphene.hpp>

namespace Brisk {

static RC<Widget> osDialogButton(std::string text, Value<Trigger<>> fn) {
    return rcnew HLayout{
        rcnew Button{
            rcnew Text{ std::move(text) },
            onClick = std::move(fn),
        },
    };
}

class SmallComponent : public Component {
public:
    RC<Widget> build() final {
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

RC<Widget> ShowcaseDialogs::build(RC<Notifications> notifications) {
    return rcnew VLayout{
        flexGrow = 1,
        padding  = 16_apx,
        gapRow   = 8_apx,

        rcnew Text{ "Multiple windows (gui/Component.hpp)", classes = { "section-header" } },

        rcnew HLayout{
            rcnew Button{
                rcnew Text{ "Open window" },
                onClick = m_lifetime |
                          [this]() {
                              RC<SmallComponent> comp = rcnew SmallComponent();
                              windowApplication->addWindow(comp->makeWindow());
                          },
            },

            rcnew Button{
                rcnew Text{ "Open modal window" },
                onClick = m_lifetime |
                          [this]() {
                              RC<SmallComponent> comp = rcnew SmallComponent();
                              windowApplication->showModalWindow(comp->makeWindow());
                          },
            },
        },
        rcnew HLayout{
            rcnew Button{
                rcnew Text{ "TextInputDialog" },
                onClick = m_lifetime |
                          []() {
                              RC<TextInputDialog> dialog = rcnew TextInputDialog{ "Enter name", "World" };
                              windowApplication->showModalWindow(dialog->makeWindow());
                              if (dialog->result)
                                  showMessage("title", "Hello, " + dialog->value, MessageBoxType::Info);
                              else
                                  showMessage("title", "Hello, nobody", MessageBoxType::Warning);
                          },
            },
        },

        rcnew Text{ "PopupDialog (widgets/PopupDialog.hpp)", classes = { "section-header" } },

        rcnew HLayout{
            rcnew Button{
                rcnew Text{ "Open Dialog" },
                onClick = m_lifetime |
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

        rcnew Text{ "OS dialogs (window/OSDialogs.hpp)", classes = { "section-header" } },
        osDialogButton(
            "Open URL", m_lifetime |
                            []() {
                                openURLInBrowser("https://www.brisklib.com/");
                            }),
        osDialogButton(
            "Open folder", m_lifetime |
                               []() {
                                   openFolder(defaultFolder(DefaultFolder::Documents));
                               }),

        osDialogButton(
            "Message box (Info)", m_lifetime |
                                      []() {
                                          showMessage("title", "message", MessageBoxType::Info);
                                      }),
        osDialogButton(
            "Message box (Warning)", m_lifetime |
                                         []() {
                                             showMessage("title", "message", MessageBoxType::Warning);
                                         }),
        osDialogButton(
            "Message box (Error)", m_lifetime |
                                       []() {
                                           showMessage("title", "message", MessageBoxType::Error);
                                       }),
        osDialogButton(
            "Dialog (OK, Cancel)", m_lifetime |
                                       [this]() {
                                           if (showDialog("title", "message", DialogButtons::OKCancel,
                                                          MessageBoxType::Info) == DialogResult::OK)
                                               m_text += "OK clicked\n";
                                           else
                                               m_text += "Cancel clicked\n";
                                           bindings->notify(&m_text);
                                       }),
        osDialogButton(
            "Dialog (Yes, No, Cancel)", m_lifetime |
                                            [this]() {
                                                if (DialogResult r = showDialog("title", "message",
                                                                                DialogButtons::YesNoCancel,
                                                                                MessageBoxType::Warning);
                                                    r == DialogResult::Yes)
                                                    m_text += "Yes clicked\n";
                                                else if (r == DialogResult::No)
                                                    m_text += "No clicked\n";
                                                else
                                                    m_text += "Cancel clicked\n";
                                                bindings->notify(&m_text);
                                            }),
        osDialogButton(
            "Open File", m_lifetime |
                             [this]() {
                                 auto file = showOpenDialog({ { "*.txt", "Text files" } },
                                                            defaultFolder(DefaultFolder::Documents));
                                 if (file)
                                     m_text += file->string() + "\n";
                                 else
                                     m_text += "(nullopt)\n";
                                 bindings->notify(&m_text);
                             }),
        osDialogButton(
            "Open Files", m_lifetime |
                              [this]() {
                                  auto files = showOpenDialogMulti({ { "*.txt", "Text files" }, anyFile() },
                                                                   defaultFolder(DefaultFolder::Documents));
                                  for (fs::path file : files)
                                      m_text += file.string() + "\n";
                                  bindings->notify(&m_text);
                              }),
        osDialogButton(
            "Pick folder", m_lifetime |
                               [this]() {
                                   auto folder = showFolderDialog(defaultFolder(DefaultFolder::Documents));
                                   if (folder)
                                       m_text += folder->string() + "\n";
                                   else
                                       m_text += "(nullopt)\n";
                                   bindings->notify(&m_text);
                               }),
        rcnew Text{
            text       = Value{ &m_text },
            fontFamily = Font::Monospace,
        },
    };
}
} // namespace Brisk
