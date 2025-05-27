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
#include <brisk/graphics/Palette.hpp>

namespace Brisk {

class ShowcaseEditors : public BindableObject<ShowcaseEditors, &uiScheduler> {
public:
    Rc<Widget> build(Rc<Notifications> notifications, Value<bool> globalEnabled);

private:
    WidthGroup m_group;
    float m_value = 50.f;
    float m_y     = 50.f;
    std::string m_text;
    std::string m_html          = "The <b>quick</b> <font color=\"brown\">brown</font> <u>fox jumps</u> over "
                                  "the <small>lazy</small> dog";
    std::string m_multilineText = "abc\ndef\nghijklmnopqrstuvwxyz";
    ColorW m_color              = Palette::Standard::indigo;
    std::string m_password      = "";
    bool m_hidePassword         = true;
};

} // namespace Brisk
