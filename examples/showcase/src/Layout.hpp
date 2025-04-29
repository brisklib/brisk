#pragma once

#include <brisk/widgets/Notifications.hpp>

namespace Brisk {

class ShowcaseLayout : public BindableObject<ShowcaseLayout, &uiScheduler> {
public:
    Rc<Widget> build(Rc<Notifications> notifications);

private:
    WidthGroup m_group;
};
} // namespace Brisk
