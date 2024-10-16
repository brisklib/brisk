#pragma once

#include <brisk/widgets/Notifications.hpp>

namespace Brisk {

class ShowcaseDropdowns : public BindingObject<ShowcaseDropdowns, &uiThread> {
public:
    RC<Widget> build(RC<Notifications> notifications);

private:
    WidthGroup m_group;
    int m_month         = 0;
    int m_selectedItem  = 0;
    int m_selectedItem2 = 5;
    int m_fruit         = 0;
};
} // namespace Brisk
