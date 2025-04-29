#pragma once

#include <brisk/widgets/Notifications.hpp>

namespace Brisk {

class ShowcaseVisual : public BindingObject<ShowcaseVisual, &uiScheduler> {
public:
    ShowcaseVisual();

    Rc<Widget> build(Rc<Notifications> notifications);

private:
    WidthGroup m_group;
    bool m_active         = true;
    bool m_progressActive = true;
    float m_progress      = 0.f;
    bool m_hintActive     = true;
    TextAlign m_textAlign = TextAlign::Start;
    float m_fontSize      = 2.f;
    float m_shadowSize    = 32.f;

    struct Row {
        std::string firstName;
        std::string lastName;
        bool checkBox;
        int index;
    };

    std::array<Row, 6> m_rows{
        Row{ "Emma", "Johnson", false, 0 },   //
        Row{ "Liam", "Anderson", false, 2 },  //
        Row{ "Olivia", "Martinez", true, 1 }, //
        Row{ "Noah", "Brown", false, 3 },     //
        Row{ "Sophia", "Wilson", true, 2 },   //
        Row{ "Ethan", "Robinson", false, 0 }, //
    };
};
} // namespace Brisk
