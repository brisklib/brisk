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
#include <brisk/gui/GUIWindow.hpp>
#include <brisk/gui/Styles.hpp>
#include <brisk/core/internal/Functional.hpp>
#include <brisk/core/Text.hpp>
#include <brisk/core/Log.hpp>
#include <brisk/gui/Component.hpp>

namespace Brisk {

void GUIWindow::onKeyEvent(KeyCode key, int scancode, KeyAction action, KeyModifiers mods) {
    if (action != KeyAction::Release)
        m_inputQueue.addEvent(EventKeyPressed{ { { {}, mods }, key }, action == KeyAction::Repeat });
    else
        m_inputQueue.addEvent(EventKeyReleased{ { { {}, mods }, key } });
}

void GUIWindow::onCharEvent(char32_t character) {
    m_inputQueue.addEvent(EventCharacterTyped{ { {}, m_mods }, character });
}

void GUIWindow::onMouseEnter() {
    //
}

void GUIWindow::onMouseLeave() {
    m_inputQueue.mouseLeave();
}

bool GUIWindow::handleEvent(function<void()> fn) {
    // The function fn creates an Event and pushes it to the m_inputQueue.
    // This method ensures that the event is processed by the event handlers
    // and doesn't end up in m_unhandledEvents.
    bool result = false;
    std::atomic_bool finished{ false };

    uiScheduler->dispatchAndWait([fn = std::move(fn), this, &result, &finished] {
        fn();
        uint32_t cookie = m_inputQueue.events.back().cookie();
        LOG_DEBUG(gui, "wait cookie={:08X}", cookie);

        windowApplication->afterRenderQueue->dispatch(
            [this, &result, cookie, &finished] {
                LOG_DEBUG(gui, "m_unhandledEvents.size={}", m_unhandledEvents.size());
                LOG_DEBUG(gui, "unhandled cookies = {}",
                          join(map(m_unhandledEvents,
                                   [](uint32_t c) {
                                       return fmt::format("{:08}", c);
                                   }),
                               ","));

                auto it = std::find(m_unhandledEvents.begin(), m_unhandledEvents.end(), cookie);
                // Event is considered handled if not found in m_unhandledEvents.
                result  = it == m_unhandledEvents.end();
                finished.store(true, std::memory_order::release);
            },
            ExecuteImmediately::Never); // Ensure the lambda is executed at the right time.
    });

    // waitUsingFunc(nullptr, finished, nullptr, 0.2);

    return result;
}

bool GUIWindow::handleKeyEvent(KeyCode key, int scancode, KeyAction action, KeyModifiers mods) {
    return handleEvent([=, this] {
        m_mods = mods;
        keyEvent(key, scancode, action, mods);
    });
}

bool GUIWindow::handleCharEvent(char32_t character) {
    return handleEvent([=, this] {
        charEvent(character);
    });
}

void GUIWindow::onMouseEvent(MouseButton button, MouseAction action, KeyModifiers mods, PointF point,
                             int conseqClicks) {
    if (action == MouseAction::Press) {
        m_inputQueue.addEvent(EventMouseButtonPressed{ { { { {}, mods }, point, m_downPoint }, button } });
        if (conseqClicks == 3)
            m_inputQueue.addEvent(EventMouseTripleClicked{ { { {}, mods }, point, m_downPoint } });
        else if (conseqClicks == 2)
            m_inputQueue.addEvent(EventMouseDoubleClicked{ { { {}, mods }, point, m_downPoint } });
    } else {
        m_inputQueue.addEvent(EventMouseButtonReleased{ { { { {}, mods }, point, m_downPoint }, button } });
    }
}

void GUIWindow::onMouseMove(PointF point) {
    m_inputQueue.addEvent(EventMouseMoved{ { { {}, m_mods }, point, m_downPoint } });
}

void GUIWindow::onWheelEvent(float x, float y) {
    if (y)
        m_inputQueue.addEvent(EventMouseYWheel{ { { {}, m_mods }, m_mousePoint, m_downPoint }, y });
    if (x)
        m_inputQueue.addEvent(EventMouseXWheel{ { { {}, m_mods }, m_mousePoint, m_downPoint }, x });
}

void GUIWindow::attachedToApplication() {
    Window::attachedToApplication();

    bindings->connect(Value{ &m_renderSettings.blueLightFilter },
                      Value{ &windowApplication->blueLightFilter });
    bindings->connect(Value{ &m_renderSettings.gamma }, Value{ &windowApplication->globalGamma });
    bindings->connect(Value{ &m_renderSettings.subPixelText }, Value{ &windowApplication->subPixelText });
}

GUIWindow::GUIWindow(RC<Component> component) : Window(), m_component(std::move(component)) {
    registerBuiltinFonts();
    m_inputQueue.unhandledEvent = [this](Event& event) BRISK_INLINE_LAMBDA {
        if (m_component)
            m_component->unhandledEvent(event);
        if (!event)
            return;
        this->unhandledEvent(event);
        if (!event)
            return;
        m_unhandledEvents.push_back(event.cookie());
    };

    LOG_INFO(window, "Done creating GUIWindow");
}

void GUIWindow::rebuild() {
    BRISK_ASSERT(m_component);
    rebuildRoot();
}

void GUIWindow::rebuildRoot() {
    BRISK_ASSERT(m_component);
    m_tree.setRoot(RC<Widget>(m_component->build()));
    BRISK_ASSERT(m_tree.root());
    m_backgroundColor = m_tree.root()->getStyleVar<ColorW>(windowColor.id).value_or(ColorW(0.f, 0.f));
}

void GUIWindow::beforeFrame() {
    if (m_component) {
        m_component->beforeFrame();
    }
}

void GUIWindow::paintImmediate(RenderContext& context) {
    if (Internal::debugDirtyRect) {
        if (!m_savedPaintRect.empty()) {
            Canvas canvas(context);
            canvas.setFillColor(0xFF8000'30_rgba);
            canvas.fillRect(m_savedPaintRect);
        }
    }
}

bool GUIWindow::update() {
    m_unhandledEvents.clear();
    InputQueueScope inputQueueScope(&m_inputQueue);
    m_tree.setViewportRectangle(getFramebufferBounds());
    if (!m_tree.root()) {
        rebuild();
    }
    uint32_t layoutCounter = m_tree.layoutCounter();

    if (m_tree.root()) {
        m_tree.update();
        setCursor(m_inputQueue.getCursorAtMouse().value_or(Cursor::Arrow));
    }
    if (m_tree.layoutCounter() != layoutCounter && m_tree.root() && m_windowFit != WindowFit::None) {
        updateWindowLimits();
    }
    return !m_tree.paintRect().empty();
}

void GUIWindow::paint(RenderContext& context, bool fullRepaint) {
    Canvas canvas(context);

    beforeDraw(canvas);
    if (m_tree.root()) {
        m_savedPaintRect = m_tree.paint(canvas, m_backgroundColor, fullRepaint);
    }
    afterDraw(canvas);
}

void GUIWindow::updateWindowLimits() {
    if (!m_tree.root())
        rebuild();
    SizeF maxContentSize = m_tree.root()->computeSize(AvailableSize{ undef, undef });

    Size framebufferSize = getFramebufferSize();
    Size windowSize      = getSize();
    if (windowSize.area() > 0 && framebufferSize.area() > 0) {
        // convert physical pixels to screen
        Size newWindowSize;
        newWindowSize.x      = maxContentSize.x * windowSize.x / framebufferSize.x;
        newWindowSize.y      = maxContentSize.y * windowSize.y / framebufferSize.y;
        newWindowSize.width  = std::min(newWindowSize.width, 4096);
        newWindowSize.height = std::min(newWindowSize.height, 4096);
        if (newWindowSize != windowSize) {
            if (m_windowFit == WindowFit::MinimumSize) {
                setMinimumSize(newWindowSize);
            } else {
                setFixedSize(newWindowSize);
            }
        }
        if (RC<Window> owner = m_owner.lock()) {
            Rectangle r = owner->getRectangle();
            r           = r.alignedRect(newWindowSize, { 0.5f, 0.5f });
            setPosition(r.p1);
        }
    }
}

void GUIWindow::clearRoot() {
    m_tree.setRoot(nullptr);
}

RC<Widget> GUIWindow::root() const {
    return m_tree.root();
}

GUIWindow::~GUIWindow() {
    beforeDestroying();
}

void GUIWindow::dpiChanged() {
    rescale();
}

void GUIWindow::rescale() {
    m_tree.rescale();
}

void GUIWindow::setId(std::string id) {
    m_id = std::move(id);
}

const std::string& GUIWindow::getId() const {
    return m_id;
}

void GUIWindow::afterDraw(Canvas& canvas) {}

void GUIWindow::beforeDraw(Canvas& canvas) {}

void GUIWindow::beforeOpeningWindow() {
    uiScheduler->dispatchAndWait([this]() {
        updateWindowLimits();
    });
}

WidgetTree& GUIWindow::tree() {
    return m_tree;
}

void GUIWindow::unhandledEvent(Event& event) {}
} // namespace Brisk
