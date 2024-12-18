#pragma once

#include <brisk/widgets/Notifications.hpp>

namespace Brisk {

class ShowcaseButtons : public BindingObject<ShowcaseButtons, &uiThread> {
public:
    ShowcaseButtons() {}

    RC<Widget> build(RC<Notifications> notifications);

private:
    WidthGroup m_group;
    HorizontalVisualGroup m_btnGroup;
    int m_clicked  = 0;
    bool m_toggled = false;
};
} // namespace Brisk
