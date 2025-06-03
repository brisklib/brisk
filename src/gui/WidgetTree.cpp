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
#include <brisk/gui/WidgetTree.hpp>
#include <brisk/gui/Gui.hpp>

namespace Brisk {

void WidgetGroup::clean() {
    for (Widget* w : widgets) {
        w->removeFromGroup(this);
    }
}

void WidgetTree::setRoot(std::shared_ptr<Widget> root) {
    if (root != m_root) {
        if (m_root) {
            m_root->setTree(nullptr);
        }
        m_root = std::move(root);
        if (m_root) {
            m_root->setTree(this);
        }
    }
}

void WidgetTree::rescale() {
    if (m_root) {
        LOG_INFO(tree, "rescale");
        m_root->resolveProperties(PropFlags::AffectLayout | PropFlags::AffectFont);
    }
}

void WidgetTree::onLayoutUpdated() {
    ++m_layoutCounter;
}

void WidgetTree::processAnimation() {
    auto queue       = std::move(m_animationQueue);
    m_animationQueue = {};
    for (const auto& weak : queue) {
        if (auto strong = weak.lock()) {
            strong->animationFrame();
        }
    }
}

void WidgetTree::processRebuild() {
    auto queue     = std::move(m_rebuildQueue);
    m_rebuildQueue = {};
    for (const auto& weak : queue) {
        if (auto strong = weak.lock()) {
            strong->doRebuild();
        }
    }
}

void WidgetTree::requestAnimationFrame(std::weak_ptr<Widget> widget) {
    m_animationQueue.push_back(std::move(widget));
}

void WidgetTree::requestRebuild(std::weak_ptr<Widget> widget) {
    m_rebuildQueue.push_back(std::move(widget));
}

void WidgetTree::requestLayer(Drawable drawable) {
    m_layer.push_back(std::move(drawable));
}

uint32_t WidgetTree::layoutCounter() const noexcept {
    return m_layoutCounter;
}

std::shared_ptr<Widget> WidgetTree::root() const noexcept {
    return m_root;
}

void WidgetTree::detach(Widget* widget) {
    onDetached(widget);
}

void WidgetTree::attach(Widget* widget) {
    for (WidgetGroup* g : widget->m_groups) {
        addGroup(g);
    }
    onAttached(widget);
}

constexpr double refreshInterval = 0.1; // in seconds

static int frameNumber           = 0;

void WidgetTree::groupsBeforeFrame() {
    for (WidgetGroup* g : m_groups) {
        g->beforeFrame();
    }
}

void WidgetTree::groupsBeforePaint() {
    for (WidgetGroup* g : m_groups) {
        g->beforePaint();
    }
}

void WidgetTree::groupsAfterFrame() {
    for (WidgetGroup* g : m_groups) {
        g->afterFrame();
    }
}

void WidgetTree::groupsBeforeLayout() {
    for (WidgetGroup* g : m_groups) {
        g->beforeLayout(m_root->isLayoutDirty());
    }
}

void WidgetTree::disableRealtimeMode() {
    m_realtime = false;
}

void WidgetTree::update() {
    if (m_realtime)
        bindings->assign(frameStartTime, currentTime());

    groupsBeforeFrame();

    // Phase 1. Calling user code (Input, animation, queued tasks)
    {
        if (m_inputQueue)
            m_inputQueue->processEvents();
        processAnimation();
        processRebuild();
        if (frameStartTime >= m_refreshTime + refreshInterval) [[unlikely]] {
            for (WidgetGroup* g : m_groups) {
                g->beforeRefresh();
            }
            m_root->refreshTree();
            m_refreshTime = frameStartTime;
        }

        if (m_updateVisibilityRequested) {
            m_root->processTreeVisibility(true);
            m_updateVisibilityRequested = false;
        }
    }

    // Phase 2. Style
    {
        m_root->restyleIfRequested();
        m_disableTransitions = false;
    }

    // Phase 3. Layout
    {
        groupsBeforeLayout();

        m_root->updateLayout(m_viewportRectangle, m_viewportRectangleChanged);
        m_viewportRectangleChanged = false;

        if (m_updateGeometryRequested) {
            if (m_inputQueue)
                m_inputQueue->reset();
            HitTestMap::State state;
            m_root->updateGeometry(state);
            m_updateGeometryRequested = false;
        }
    }
}

Rectangle WidgetTree::paint(Canvas& canvas, ColorW backgroundColor, bool fullRepaint) {
    bool debugBoundaries   = Internal::debugBoundaries;
    m_fullRepaint          = fullRepaint || debugBoundaries || m_savedDebugBoundaries;
    m_savedDebugBoundaries = debugBoundaries;
    Rectangle paintRect    = this->paintRect();

    if (paintRect.empty()) {
        groupsAfterFrame();
        return {};
    }

    groupsBeforePaint();

    ++frameNumber;
    canvas.renderContext().setClipRect(paintRect);
    if (backgroundColor.a != 0) {
        canvas.setFillColor(backgroundColor);
        canvas.fillRect(m_viewportRectangle);
    }

    m_painting = true;

    // Paint the widgets per-layer
    // Clear the layer and push the root widget's drawable
    m_layer.clear();
    m_layer.push_back(m_root->drawable());
    do {
        std::vector<Drawable> layer;
        // Swap the contents of the current layer with the m_layer vector.
        // This allows us to process the current layer while m_layer gets populated with the next layer of
        // drawables.
        std::swap(layer, m_layer);
        for (const Drawable& d : layer) {
            d(canvas);
        }
        // This loop continues until m_layer is empty, which means all drawables have been painted.
        // If any drawables in the current layer trigger the addition of new drawables to m_layer,
        // the loop will process those new drawables in the subsequent iterations.
    } while (!m_layer.empty());

    m_dirtyRect   = std::nullopt;
    m_fullRepaint = false;

    m_painting    = false;

    if (Internal::debugBoundaries && m_inputQueue) {
        std::optional<Rectangle> rect = m_inputQueue->getAtMouse<Rectangle>([](Widget* w) {
            return w->rect();
        });
        if (rect) {
            canvas.setFillColor(0x102040'40_rgba);
            canvas.fillRect(*rect);
        }
    }
    groupsAfterFrame();
    return paintRect;
}

Rectangle WidgetTree::paintRect() const {
    return m_fullRepaint ? m_viewportRectangle : m_dirtyRect.value_or(Rectangle{});
}

void WidgetTree::requestUpdateGeometry() {
    m_updateGeometryRequested = true;
}

void WidgetTree::addGroup(WidgetGroup* group) {
    m_groups.insert(group);
}

bool WidgetTree::transitionsAllowed() {
    return !m_disableTransitions;
}

void WidgetTree::disableTransitions() {
    m_disableTransitions = true;
}

void WidgetTree::setViewportRectangle(Rectangle rect) {
    if (rect != m_viewportRectangle) {
        m_viewportRectangleChanged = true;
        m_viewportRectangle        = rect;
    }
}

Rectangle WidgetTree::viewportRectangle() const noexcept {
    return m_viewportRectangle;
}

bool WidgetTree::isDirty(Rectangle rect) const {
    return rect.intersects(m_viewportRectangle) &&
           (m_fullRepaint || (m_dirtyRect && rect.intersects(*m_dirtyRect)));
}

void WidgetTree::invalidateRect(Rectangle rect) {
    BRISK_ASSERT(!m_painting);
    if (rect.intersects(m_viewportRectangle)) {
        m_dirtyRect =
            m_viewportRectangle.intersection(!m_dirtyRect.has_value() ? rect : m_dirtyRect->union_(rect));
    }
}

void WidgetTree::setInputQueue(InputQueue* inputQueue) {
    m_inputQueue = inputQueue;
}

Nullable<InputQueue> WidgetTree::inputQueue() const {
    return m_inputQueue;
}

WidgetTree::WidgetTree(InputQueue* inputQueue) noexcept : m_inputQueue(inputQueue) {}

void WidgetTree::requestUpdateVisibility() {
    m_updateVisibilityRequested = true;
}
} // namespace Brisk
