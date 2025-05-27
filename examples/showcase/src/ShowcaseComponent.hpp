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
#pragma once

#include <brisk/gui/Icons.hpp>
#include <brisk/core/Binding.hpp>
#include <brisk/gui/GuiWindow.hpp>
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
    Rc<ShowcaseButtons> m_buttons       = rcnew ShowcaseButtons();
    Rc<ShowcaseDropdowns> m_dropdowns   = rcnew ShowcaseDropdowns();
    Rc<ShowcaseLayout> m_layout         = rcnew ShowcaseLayout();
    Rc<ShowcaseDialogs> m_dialogs       = rcnew ShowcaseDialogs();
    Rc<ShowcaseEditors> m_editors       = rcnew ShowcaseEditors();
    Rc<ShowcaseVisual> m_visual         = rcnew ShowcaseVisual();
    Rc<ShowcaseMessenger> m_messenger   = rcnew ShowcaseMessenger();
    Rc<ShowcaseTypography> m_typography = rcnew ShowcaseTypography();
    Rc<ShowcaseBinding> m_binding       = rcnew ShowcaseBinding();

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
    bool m_globalEnabled   = true;

    Rc<Widget> build() final;
    void unhandledEvent(Event& event) final;
    void configureWindow(Rc<GuiWindow> window) final;
    void captureScreenshot();
    void saveScreenshot(Rc<Image> image);

public:
    BRISK_PROPERTIES_BEGIN
    Property<This, float, &This::m_progress> progress;
    Property<This, bool, &This::m_globalEnabled> globalEnabled;
    BRISK_PROPERTIES_END
};

} // namespace Brisk
