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
#include <brisk/gui/GUIWindow.hpp>
#include <brisk/window/OSDialogs.hpp>

namespace Brisk {

/**
 * @brief Base class for creating a UI component.
 *
 * This class provides the basic structure and behavior for any UI component
 * in the application. It manages the lifecycle of the component, its event
 * handling, and its associated window.
 */
class Component : public BindingObject<Component, &uiScheduler> {
public:
    virtual ~Component();

    /**
     * @brief Gets the `GUIWindow` associated with this component.
     *
     * @note May return nullptr if the component has no associated window or if it has not been created yet.
     *
     * @return RC<GUIWindow> A reference-counted pointer to the GUIWindow.
     */
    RC<GUIWindow> window();

    /**
     * @brief Returns the `WidgetTree` for the component.
     *
     * @return WidgetTree& A reference to the WidgetTree.
     */
    WidgetTree& tree();

    /**
     * @brief Gets the window associated with this component, creating it if it does not exist.
     *
     * @return RC<GUIWindow> A reference-counted pointer to the GUIWindow.
     */
    RC<GUIWindow> makeWindow();

protected:
    friend class GUIWindow;

    /**
     * @brief Called to build the component's widget hierarchy.
     *
     * This function should be overridden to construct and return the root
     * widget of the component.
     *
     * @return RC<Widget> A reference-counted pointer to the root widget.
     */
    virtual RC<Widget> build();

    /**
     * @brief This method is called on the main thread and is expected to return
     * the window object that the component will use.
     *
     * @return RC<GUIWindow> A reference-counted pointer to the GUIWindow.
     */
    virtual RC<GUIWindow> createWindow();

    /**
     * @brief Handles any unhandled events.
     *
     * If an event is not handled by the widget tree, this function will be called.
     * It can be overridden to provide custom handling for specific events.
     *
     * @param event The unhandled event.
     */
    virtual void unhandledEvent(Event& event);

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
    virtual void configureWindow(RC<GUIWindow> window);

    /**
     * @brief Called before rendering a new frame.
     *
     * This is typically used for pre-frame updates, such as preparing data or animations.
     */
    virtual void beforeFrame();

    /**
     * @brief Closes the associated window.
     *
     * This will hide and optionally destroy the window tied to the component.
     */
    void closeWindow();

private:
    WeakRC<GUIWindow> m_window;
};

template <std::derived_from<Component> ComponentClass>
void createComponent(RC<ComponentClass>& component) {
    uiScheduler->dispatchAndWait([&]() {
        component = rcnew ComponentClass();
    });
}

template <std::derived_from<Component> ComponentClass>
RC<ComponentClass> createComponent() {
    RC<ComponentClass> component;
    createComponent<ComponentClass>(component);
    return component;
}

} // namespace Brisk
