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
 */                                                                                                          \
#pragma once

#include <brisk/widgets/DialogComponent.hpp>

namespace App {
using namespace Brisk;

class SettingsComponent final : public DialogComponent {
public:
    Rc<Widget> build() override;

protected:
    void configureWindow(Rc<GuiWindow> window) override;

private:
    int m_refreshInterval = 1000;
    bool m_showPlots      = true;

public:
    static const auto& properties() noexcept {
        static constexpr tuplet::tuple props{
            Internal::PropField{ &SettingsComponent::m_refreshInterval, "refreshInterval" },
            Internal::PropField{ &SettingsComponent::m_showPlots, "showPlots" },
        };
        return props;
    }

public:
    BRISK_PROPERTIES_BEGIN
    Property<SettingsComponent, int, 0> refreshInterval;
    Property<SettingsComponent, bool, 1> showPlots;
    BRISK_PROPERTIES_END
};
} // namespace App
