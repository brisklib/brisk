#pragma once

#include <brisk/widgets/Widgets.hpp>

namespace Brisk {

class ShowcaseBinding : public BindingObject<ShowcaseBinding, &uiThread> {
public:
    RC<Widget> build(RC<Notifications> notifications);

private:
    WidthGroup m_group;
    bool m_open    = false;
    float m_value1 = 0.f;
    float m_value2 = 0.f;
    float m_value3 = 0.f;
    bool m_checkBoxes[5]{ false, false, false, false, false };
    float m_size = 30.f;
};
} // namespace Brisk