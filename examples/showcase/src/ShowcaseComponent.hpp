#pragma once

#include "brisk/gui/Icons.hpp"
#include <brisk/core/Binding.hpp>
#include <brisk/gui/GUIWindow.hpp>
#include <brisk/gui/Component.hpp>
#include <brisk/core/Reflection.hpp>
#include <brisk/widgets/Notifications.hpp>
#include <brisk/graphics/Palette.hpp>
#include "Buttons.hpp"
#include "Editors.hpp"
#include "Dropdowns.hpp"
#include "Layout.hpp"
#include "Dialogs.hpp"
#include "Messenger.hpp"
#include "Visual.hpp"
#include "Typography.hpp"
#include "Binding.hpp"

namespace Brisk {

class ShowcaseComponent final : public Component {
public:
    explicit ShowcaseComponent();

    using This = ShowcaseComponent;

protected:
    Notifications m_notifications;
    RC<ShowcaseButtons> m_buttons       = rcnew ShowcaseButtons();
    RC<ShowcaseDropdowns> m_dropdowns   = rcnew ShowcaseDropdowns();
    RC<ShowcaseLayout> m_layout         = rcnew ShowcaseLayout();
    RC<ShowcaseDialogs> m_dialogs       = rcnew ShowcaseDialogs();
    RC<ShowcaseEditors> m_editors       = rcnew ShowcaseEditors();
    RC<ShowcaseVisual> m_visual         = rcnew ShowcaseVisual();
    RC<ShowcaseMessenger> m_messenger   = rcnew ShowcaseMessenger();
    RC<ShowcaseTypography> m_typography = rcnew ShowcaseTypography();
    RC<ShowcaseBinding> m_binding       = rcnew ShowcaseBinding();

    int m_activePage                    = 0;
    float m_progress                    = 0.f;
    int m_comboBoxValue                 = 0;
    int m_comboBoxValue2                = 0;
    int m_index                         = 0;
    double m_spinValue                  = 0;
    std::string m_chatMessage;
    bool m_popupDialog = false;
    std::string m_text;
    std::string m_editable = "ABCDEF";

    bool m_lightTheme      = false;

    RC<Widget> build() final;
    void unhandledEvent(Event& event) final;
    void configureWindow(RC<GUIWindow> window) final;
    void captureScreenshot();
    void saveScreenshot(RC<Image> image);

public:
    BRISK_PROPERTIES_BEGIN
    Property<This, float, &This::m_progress> progress;
    BRISK_PROPERTIES_END
};

} // namespace Brisk
