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

#include <brisk/gui/Event.hpp>
#include <brisk/graphics/Canvas.hpp>
#include <memory>
#include <brisk/core/internal/Function.hpp>
#include <brisk/core/Binding.hpp>
#include <brisk/core/Utilities.hpp>
#include <brisk/graphics/Geometry.hpp>
#include <stack>

namespace Brisk {

class WidgetTree;
class Widget;

struct WidgetGroup {
    std::vector<Widget*> widgets;

    virtual ~WidgetGroup() {
        clean();
    }

    virtual void beforeRefresh() {}

    virtual void beforeFrame() {}

    virtual void beforeLayout(bool dirty) {}

    virtual void beforePaint() {}

    virtual void afterFrame() {}

    void clean();
};

using Drawable = function<void(Canvas&)>;

class WidgetTree {
public:
    WidgetTree(InputQueue* inputQueue = nullptr) noexcept;
    ~WidgetTree();

    std::shared_ptr<Widget> root() const noexcept;
    void setRoot(std::shared_ptr<Widget> root);
    void rescale();
    void onLayoutUpdated();
    uint32_t layoutCounter() const noexcept;

    void setViewportRectangle(Rectangle rect);

    Rectangle viewportRectangle() const noexcept;

    Rectangle paintRect() const;
    Rectangle paint(Canvas& canvas, ColorW backgroundColor, bool fullRepaint);
    void update();
    void requestLayer(Drawable drawable);

    void invalidateRect(Rectangle rect);
    void setInputQueue(InputQueue* inputQueue);
    Nullable<InputQueue> inputQueue() const;

    void disableTransitions();
    void disableRealtimeMode();

    Callbacks<Widget*> onAttached;
    Callbacks<Widget*> onDetached;

private:
    friend class Widget;

    void processAnimation();
    void processRebuild();
    void requestAnimationFrame(std::weak_ptr<Widget> widget);
    void requestRebuild(std::weak_ptr<Widget> widget);
    void requestUpdateGeometry();
    void requestUpdateVisibility();
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
    void applyStyleChanges();
    void processEventsAndAnimations();
    void updateLayoutAndGeometry();
    std::shared_ptr<Widget> m_root;
    std::vector<std::weak_ptr<Widget>> m_animationQueue;
    std::vector<std::weak_ptr<Widget>> m_rebuildQueue;
    std::vector<Drawable> m_layer;
    uint32_t m_layoutCounter         = 0;
    double m_refreshTime             = 0;
    bool m_transitions               = true;
    bool m_updateGeometryRequested   = false;
    bool m_updateVisibilityRequested = true;
    std::set<WidgetGroup*> m_groups;
    Rectangle m_viewportRectangle{};
    bool m_viewportRectangleChanged = true;
    std::optional<Rectangle> m_dirtyRect;
    std::vector<Rectangle> m_dirtyRects;
    bool m_fullRepaint          = true;
    bool m_painting             = false;
    bool m_savedDebugBoundaries = false;
    bool m_realtime             = true;
    bool m_layoutIsActual       = false;
    InputQueue* m_inputQueue    = nullptr;
};
} // namespace Brisk
