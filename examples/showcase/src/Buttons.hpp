#pragma once

#include <brisk/widgets/Notifications.hpp>

namespace Brisk {

class ShowcaseButtons : public BindableObject<ShowcaseButtons, &uiScheduler> {
public:
    ShowcaseButtons() {}

    Rc<Widget> build(Rc<Notifications> notifications);

private:
    WidthGroup m_group;
    HorizontalVisualGroup m_btnGroup;
    int m_clicked  = 0;
    bool m_toggled = false;
};
} // namespace Brisk
