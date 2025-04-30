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

#include <brisk/gui/Gui.hpp>
#include <brisk/gui/GuiWindow.hpp>
#include <brisk/gui/Action.hpp>

namespace Brisk {

/**
 * @brief Base class for creating a UI component.
 *
 * This class provides the basic structure and behavior for any UI component
 * in the application. It manages the lifecycle of the component, its event
 * handling, and its associated window.
 */
class Component : public BindableObject<Component, &uiScheduler> {
public:
    ~Component() override {}

    /**
     * @brief Gets the `GuiWindow` associated with this component.
     *
     * @note May return nullptr if the component has no associated window or if it has not been created yet.
     *
     * @return Rc<GuiWindow> A reference-counted pointer to the GuiWindow.
     */
    Rc<GuiWindow> window();

    /**
     * @brief Returns the `WidgetTree` for the component.
     *
     * @return WidgetTree& A reference to the WidgetTree.
     */
    WidgetTree& tree();

    /**
     * @brief Gets the window associated with this component, creating it if it does not exist.
     *
     * @return Rc<GuiWindow> A reference-counted pointer to the GuiWindow.
     */
    Rc<GuiWindow> makeWindow();

    /**
     * @brief Closes the associated window.
     *
     * This will hide and optionally destroy the window tied to the component.
     */
    void closeWindow();

protected:
    friend class GuiWindow;

    /**
     * @brief Called to build the component's widget hierarchy.
     *
     * This function should be overridden to construct and return the root
     * widget of the component.
     *
     * @return Rc<Widget> A reference-counted pointer to the root widget.
     */
    virtual Rc<Widget> build();

    /**
     * @brief This method is called on the main thread and is expected to return
     * the window object that the component will use.
     *
     * @return Rc<GuiWindow> A reference-counted pointer to the GuiWindow.
     */
    virtual Rc<GuiWindow> createWindow();

    /**
     * @brief Handles any unhandled events.
     *
     * If an event is not handled by the widget tree, this function will be called.
     * It can be overridden to provide custom handling for specific events.
     *
     * @param event The unhandled event.
     */
    virtual void unhandledEvent(Event& event);

    void handleDebugKeystrokes(Event& event);

    void handleActionShortcuts(Event& event, std::initializer_list<const Action*> actions);

    /**
     * @brief Called when the UI scale is changed.
     *
     * This can be used to adjust the component's appearance for different screen scales.
     */
    virtual void onScaleChanged();

    /**
     * @brief Configures the window for the component.
     *
     * This is called before the window is shown and allows customization of the window (title, size etc).
     *
     * @param window The window to configure.
     */
    virtual void configureWindow(Rc<GuiWindow> window);

    /**
     * @brief Called before rendering a new frame.
     *
     * This is typically used for pre-frame updates, such as preparing data or animations.
     */
    virtual void beforeFrame();

private:
    WeakRc<GuiWindow> m_window;
};

template <std::derived_from<Component> ComponentClass>
void createComponent(Rc<ComponentClass>& component) {
    uiScheduler->dispatchAndWait([&]() {
        component = rcnew ComponentClass();
    });
}

template <std::derived_from<Component> ComponentClass>
Rc<ComponentClass> createComponent() {
    Rc<ComponentClass> component;
    createComponent<ComponentClass>(component);
    return component;
}

} // namespace Brisk
