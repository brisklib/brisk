#pragma once

#include <brisk/widgets/Notifications.hpp>

namespace Brisk {

class ShowcaseDialogs : public BindingObject<ShowcaseDialogs, &uiScheduler> {
public:
    RC<Widget> build(RC<Notifications> notifications);

private:
    WidthGroup m_group;
    std::string m_text;
    bool m_popupDialog = false;
};
} // namespace Brisk
