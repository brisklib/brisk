/*
 * Brisk
 *
 * Cross-platform application framework
 * --------------------------------------------------------------
 *
 * Copyright (C) 2024 Brisk Developers
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

#include <brisk/gui/GUI.hpp>

namespace Brisk {

struct GuideFocus {
    std::string id;
    PointF sourceAnchor;
    PointF targetAnchor;
};

class WIDGET Guide final : public Widget {
    BRISK_DYNAMIC_CLASS(Guide, Widget)
public:
    using Base = Widget;

    template <WidgetArgument... Args>
    explicit Guide(std::vector<GuideFocus> focus, const Args&... args)
        : Widget{ Construction{ "guide" },
                  std::tuple{ Arg::placement = Placement::Absolute, Arg::zorder = ZOrder::TopMost,
                              Arg::alignToViewport = AlignToViewport::X, args... } },
          m_focus(std::move(focus)) {
        endConstruction();
    }

protected:
    void paint(Canvas& canvas) const override;
    std::vector<GuideFocus> m_focus;
};

} // namespace Brisk
