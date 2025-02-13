#pragma once

#include <brisk/widgets/Notifications.hpp>
#include <brisk/graphics/Palette.hpp>

namespace Brisk {

class ShowcaseEditors : public BindingObject<ShowcaseEditors, &uiScheduler> {
public:
    RC<Widget> build(RC<Notifications> notifications);

private:
    WidthGroup m_group;
    float m_value = 50.f;
    float m_y     = 50.f;
    std::string m_text;
    std::string m_html          = "The <b>quick</b> <font color=\"brown\">brown</font> <u>fox jumps</u> over "
                                  "the <small>lazy</small> dog";
    std::string m_multilineText = "abc\ndef\nghijklmnopqrstuvwxyz";
    ColorF m_color              = Palette::Standard::indigo;
    std::string m_password      = "";
    bool m_hidePassword         = true;
};

} // namespace Brisk
