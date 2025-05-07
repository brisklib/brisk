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

#include <brisk/widgets/Notifications.hpp>

namespace Brisk {

class ShowcaseVisual : public BindableObject<ShowcaseVisual, &uiScheduler> {
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
