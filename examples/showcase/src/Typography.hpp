#pragma once

#include <brisk/widgets/Notifications.hpp>

namespace Brisk {

class ShowcaseTypography : public BindingObject<ShowcaseTypography, &uiScheduler> {
public:
    Rc<Widget> build(Rc<Notifications> notifications);

private:
    OpenTypeFeatureFlags m_fontFeatures{
        { OpenTypeFeature::salt, false },
        { OpenTypeFeature::liga, true },
        { OpenTypeFeature::onum, false },
        { OpenTypeFeature::kern, true },
    };
    float m_letterSpacing           = 0.f;
    float m_wordSpacing             = 0.f;
    TextDecoration m_textDecoration = TextDecoration::None;
};
} // namespace Brisk
