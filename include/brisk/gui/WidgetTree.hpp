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

#include <brisk/graphics/RawCanvas.hpp>
#include <memory>
#include <brisk/core/internal/Function.hpp>
#include <brisk/core/Binding.hpp>
#include <brisk/graphics/Geometry.hpp>
#include <stack>

namespace Brisk {

class WidgetTree;
class Widget;

struct WidgetGroup {
    std::vector<Widget*> widgets;

    ~WidgetGroup();

    virtual void beforeRefresh() {}

    virtual void beforeFrame() {}

    virtual void beforeLayout(bool dirty) {}

    virtual void beforePaint() {}

    virtual void afterFrame() {}
};

using Drawable = function<void(Canvas&)>;

class WidgetTree {
public:
    std::shared_ptr<Widget> root() const noexcept;
    void setRoot(std::shared_ptr<Widget> root);
    void rescale();
    void onLayoutUpdated();
    uint32_t layoutCounter() const noexcept;

    void setViewportRectangle(Rectangle rect);

    Rectangle viewportRectangle() const noexcept;

    Rectangle paintRect() const;
    Rectangle updateAndPaint(Canvas& canvas, ColorF backgroundColor, bool fullRepaint);
    void requestLayer(Drawable drawable);
    void disableTransitions();

    void invalidateRect(Rectangle rect);

    Callbacks<Widget*> onAttached;
    Callbacks<Widget*> onDetached;

private:
    friend class Widget;

    void processAnimation();
    void processRebuild();
    void requestAnimationFrame(std::weak_ptr<Widget> widget);
    void requestRebuild(std::weak_ptr<Widget> widget);
    void requestUpdateGeometry();
    bool transitionsAllowed();
    void attach(Widget* widget);
    void detach(Widget* widget);
    void addGroup(WidgetGroup* group);
    void removeGroup(WidgetGroup* group);
    bool isDirty(Rectangle rect) const;
    void groupsBeforeFrame();
    void groupsBeforePaint();
    void groupsAfterFrame();
    void groupsBeforeLayout();
    std::shared_ptr<Widget> m_root;
    std::vector<std::weak_ptr<Widget>> m_animationQueue;
    std::vector<std::weak_ptr<Widget>> m_rebuildQueue;
    std::vector<Drawable> m_layer;
    uint32_t m_layoutCounter       = 0;
    double m_refreshTime           = 0;
    bool m_disableTransitions      = false;
    bool m_updateGeometryRequested = false;
    std::set<WidgetGroup*> m_groups;
    Rectangle m_viewportRectangle{};
    bool m_viewportRectangleChanged = true;
    std::optional<Rectangle> m_dirtyRect;
    std::vector<Rectangle> m_dirtyRects;
    bool m_fullRepaint = true;
    bool m_painting    = false;
};
} // namespace Brisk
