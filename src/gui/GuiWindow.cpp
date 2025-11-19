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
#include <brisk/gui/GuiWindow.hpp>
#include <brisk/gui/Styles.hpp>
#include <brisk/core/internal/Functional.hpp>
#include <brisk/core/Text.hpp>
#include <brisk/core/Log.hpp>
#include <brisk/gui/Component.hpp>

namespace Brisk {

bool GuiWindow::processEvent(Event&& e) {
    if (m_inputQueue.processEvent(e))
        return true;

    if (m_component)
        m_component->unhandledEvent(e);
    if (e)
        this->unhandledEvent(e);
    return !e;
}

bool GuiWindow::onKeyEvent(KeyCode key, int scancode, KeyAction action, KeyModifiers mods) {
    if (action != KeyAction::Release)
        return processEvent(EventKeyPressed{ { { {}, mods }, key }, action == KeyAction::Repeat });
    else
        return processEvent(EventKeyReleased{ { { {}, mods }, key } });
}

bool GuiWindow::onCharEvent(char32_t character) {
    return processEvent(EventCharacterTyped{ { {}, m_mods }, character });
}

void GuiWindow::onMouseEnter() {
    //
}

void GuiWindow::onMouseLeave() {
    m_inputQueue.mouseLeave();
}

void GuiWindow::onNonClientClicked() {
    m_inputQueue.finishMenu();
}

void GuiWindow::onFocusChange(bool gained) {
    m_inputQueue.finishMenu();
}

bool GuiWindow::handleKeyEvent(KeyCode key, int scancode, KeyAction action, KeyModifiers mods) {
    return keyEvent(key, scancode, action, mods);
}

bool GuiWindow::handleCharEvent(char32_t character) {
    return charEvent(character);
}

bool GuiWindow::onMouseEvent(MouseButton button, MouseAction action, KeyModifiers mods, PointF point,
                             int conseqClicks) {
    if (action == MouseAction::Press) {
        bool handled =
            processEvent(EventMouseButtonPressed{ { { { {}, mods }, point, m_downPoint }, button } });
        if (conseqClicks == 3)
            processEvent(EventMouseTripleClicked{ { { {}, mods }, point, m_downPoint } });
        else if (conseqClicks == 2)
            processEvent(EventMouseDoubleClicked{ { { {}, mods }, point, m_downPoint } });
        return handled;
    } else {
        return processEvent(EventMouseButtonReleased{ { { { {}, mods }, point, m_downPoint }, button } });
    }
}

bool GuiWindow::onMouseMove(PointF point) {
    return processEvent(EventMouseMoved{ { { {}, m_mods }, point, m_downPoint } });
}

bool GuiWindow::onWheelEvent(float x, float y) {
    if (y)
        return processEvent(EventMouseYWheel{ { { {}, m_mods }, m_mousePoint, m_downPoint }, y });
    if (x)
        return processEvent(EventMouseXWheel{ { { {}, m_mods }, m_mousePoint, m_downPoint }, x });
    return false;
}

void GuiWindow::attachedToApplication() {
    Window::attachedToApplication();

    bindings->connect(Value{ &m_renderSettings.blueLightFilter },
                      Value{ &windowApplication->blueLightFilter });
    bindings->connect(Value{ &m_renderSettings.gamma }, Value{ &windowApplication->globalGamma });
    bindings->connect(Value{ &m_renderSettings.subPixelText }, Value{ &windowApplication->subPixelText });
}

GuiWindow::GuiWindow(Rc<Component> component) : Window(), m_component(std::move(component)) {
    registerBuiltinFonts();

    BRISK_LOG_INFO("Done creating GuiWindow");
}

void GuiWindow::rebuild() {
    BRISK_ASSERT(m_component);
    rebuildRoot();
}

void GuiWindow::rebuildRoot() {
    BRISK_ASSERT(m_component);
    m_tree.setRoot(Rc<Widget>(m_component->build()));
    BRISK_ASSERT(m_tree.root());
    m_backgroundColor = m_tree.root()->getStyleVar<ColorW>(windowColor.id).value_or(ColorW(0.f, 0.f));
}

void GuiWindow::beforeFrame() {
    if (m_component) {
        m_component->beforeFrame();
    }
}

void GuiWindow::paintImmediate(RenderContext& context) {
    if (Internal::debugDirtyRect) {
        if (!m_savedPaintRect.empty()) {
            Canvas canvas(context);
            canvas.setFillColor(0xFF8000'30_rgba);
            canvas.fillRect(m_savedPaintRect);
        }
    }
}

bool GuiWindow::update() {
    m_tree.setViewportRectangle(getFramebufferBounds());
    if (!m_tree.root()) {
        rebuild();
    }
    uint32_t layoutCounter = m_tree.layoutCounter();

    if (m_tree.root()) {
        m_tree.update();
        setCursor(m_inputQueue.getCursorAtMouse().value_or(Cursor::Arrow));
    }
    if (m_tree.layoutCounter() != layoutCounter && m_tree.root()) {
        updateWindowLimits();
    }
    return !m_tree.paintRect().empty();
}

void GuiWindow::paint(RenderContext& context, bool fullRepaint) {
    Canvas canvas(context);

    beforeDraw(canvas);
    if (m_tree.root()) {
        m_savedPaintRect = m_tree.paint(canvas, m_backgroundColor, fullRepaint);
    }
    afterDraw(canvas);
}

void GuiWindow::updateWindowLimits() {
    if (m_windowFit == WindowFit::None)
        return;
    if (!m_tree.root())
        rebuild();
    SizeF maxContentSize = m_tree.root()->computeSize(AvailableSize{ undef, undef });

    Size framebufferSize = getFramebufferSize();
    Size windowSize      = getSize();
    if (windowSize.area() > 0 && framebufferSize.area() > 0) {
        // convert framebuffer pixels to screen
        Size newWindowSize = convertUnit(Unit::Screen, maxContentSize, Unit::Framebuffer);
        auto display       = this->display();
        Size resolution    = display ? display->workarea().size() : Size{ 4096, 2048 };
        newWindowSize      = min(newWindowSize, resolution);

        if (newWindowSize != windowSize) {
            if (m_windowFit == WindowFit::MinimumSize) {
                setMinimumSize(newWindowSize);
            } else {
                setMinimumMaximumSizes(newWindowSize, newWindowSize);
            }
            if (Rc<Window> owner = m_owner.lock()) {
                Rectangle r = owner->getRectangle();
                r           = r.alignedRect(newWindowSize, { 0.5f, 0.5f });
                setPosition(r.p1);
            }
        }
    }
}

void GuiWindow::clearRoot() {
    m_tree.setRoot(nullptr);
}

Rc<Widget> GuiWindow::root() const {
    return m_tree.root();
}

GuiWindow::~GuiWindow() {
    beforeDestroying();
}

void GuiWindow::pixelRatioChanged() {
    rescale();
}

void GuiWindow::rescale() {
    m_tree.rescale();
}

void GuiWindow::setId(std::string id) {
    m_id = std::move(id);
}

const std::string& GuiWindow::getId() const {
    return m_id;
}

void GuiWindow::afterDraw(Canvas& canvas) {}

void GuiWindow::beforeDraw(Canvas& canvas) {}

void GuiWindow::beforeOpeningWindow() {
    updateWindowLimits();
}

WidgetTree& GuiWindow::tree() {
    return m_tree;
}

void GuiWindow::unhandledEvent(Event& event) {}
} // namespace Brisk
