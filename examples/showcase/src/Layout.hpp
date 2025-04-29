#pragma once

#include <brisk/widgets/Notifications.hpp>

namespace Brisk {

class ShowcaseLayout : public BindingObject<ShowcaseLayout, &uiScheduler> {
public:
    Rc<Widget> build(Rc<Notifications> notifications);

private:
    WidthGroup m_group;
};
} // namespace Brisk
