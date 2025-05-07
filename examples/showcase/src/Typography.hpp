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

class ShowcaseTypography : public BindableObject<ShowcaseTypography, &uiScheduler> {
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
