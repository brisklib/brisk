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

#include <brisk/window/Window.hpp>
#include <brisk/core/Localization.hpp>
#include "Gui.hpp"

namespace Brisk {

enum class WindowFit {
    None,
    MinimumSize,
    FixedSize,
};

class GuiWindow;

class Component;

class GuiWindow : public Window {
public:
    void pixelRatioChanged() override;
    void rebuild();
    const std::string& getId() const;
    void setId(std::string id);
    bool handleKeyEvent(KeyCode key, int scancode, KeyAction action, KeyModifiers mods);
    bool handleCharEvent(char32_t character);
    bool handleEvent(function<void()> fn);
    WidgetTree& tree();

    explicit GuiWindow(Rc<Component> component);
    ~GuiWindow();

protected:
    Rc<Component> m_component;
    ColorW m_backgroundColor = Palette::black;
    WindowFit m_windowFit    = WindowFit::MinimumSize;

    bool update() override;
    void paint(RenderContext& context, bool fullRepaint) override;
    void paintImmediate(RenderContext& context) override;
    virtual void rescale();
    virtual void unhandledEvent(Event& event);
    virtual void beforeDraw(Canvas& canvas);
    virtual void afterDraw(Canvas& canvas);
    Rc<Widget> root() const;
    void clearRoot();
    void rebuildRoot();
    void beforeFrame() override;
    void beforeOpeningWindow() override;

    void onKeyEvent(KeyCode key, int scancode, KeyAction action, KeyModifiers mods) override;
    void onCharEvent(char32_t character) override;
    void onMouseEvent(MouseButton button, MouseAction action, KeyModifiers mods, PointF point,
                      int conseqClicks) override;
    void onMouseMove(PointF point) override;
    void onWheelEvent(float x, float y) override;
    void onMouseEnter() override;
    void onMouseLeave() override;
    void onNonClientClicked() override;
    void onFocusChange(bool gained) override;
    void attachedToApplication() final;
    InputQueue m_inputQueue;
    WidgetTree m_tree{ &m_inputQueue };

private:
    std::string m_id;
    bool m_frameSkipTestState = false;
    std::vector<uint32_t> m_unhandledEvents;
    Rectangle m_savedPaintRect{};

    void updateWindowLimits();

public:
    static const auto& properties() noexcept {
        static constexpr tuplet::tuple props{
            /*0*/ Internal::PropField{ &GuiWindow::m_windowFit, "windowFit" },
        };
        return props;
    }

public:
    BRISK_PROPERTIES_BEGIN
    Property<GuiWindow, WindowFit, 0> windowFit;
    BRISK_PROPERTIES_END
};

;

} // namespace Brisk
