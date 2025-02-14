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

#include <brisk/window/WindowApplication.hpp>
#include <brisk/core/Memory.hpp>
#include <brisk/core/Binding.hpp>
#include <memory>
#include <deque>
#include <brisk/core/internal/Function.hpp>
#include <brisk/core/internal/Generation.hpp>
#include <atomic>
#include <variant>
#include <brisk/window/Types.hpp>
#include <brisk/graphics/Geometry.hpp>
#include <brisk/core/Utilities.hpp>

namespace Brisk {

enum class MouseInteraction : uint8_t {
    Inherit,
    Enable,
    Disable,
};

class Widget;

/**
 * @brief Global atomic event counter.
 */
extern std::atomic_uint32_t eventCookie;

/**
 * @brief Enum representing drag event subtypes.
 */
enum class DragEventSubtype {
    Over,  ///< Dragging over a target.
    Enter, ///< Dragging has entered a target.
    Exit,  ///< Dragging has exited a target.
};

/**
 * @brief Enum representing drop event subtypes.
 */
enum class DropEventSubtype {
    Drop,   ///< A drop event.
    Cancel, ///< A canceled drop event.
};

/**
 * @brief Base struct for events, holding a unique event cookie.
 */
struct EventBase {
    uint32_t cookie = ++eventCookie; ///< Unique identifier for the event.
};

/**
 * @brief Base struct for input events, derived from EventBase.
 */
struct EventInput : public EventBase {
    KeyModifiers mods; ///< Modifiers active during the input event.
};

/**
 * @brief Struct representing a mouse event, derived from EventInput.
 */
struct EventMouse : public EventInput {
    PointF point; ///< The current mouse position.
    std::optional<PointF>
        downPoint; ///< The mouse position at the time the button was pressed, if applicable.
};

/**
 * @brief Struct representing a mouse button event, derived from EventMouse.
 */
struct EventMouseButton : public EventMouse {
    MouseButton button; ///< The button involved in the event.
};

/**
 * @brief Struct representing a mouse moved event.
 */
struct EventMouseMoved : public EventMouse {};

/**
 * @brief Struct representing a mouse wheel event.
 */
struct EventMouseYWheel : public EventMouse {
    float delta; ///< The amount the mouse wheel was scrolled vertically.
};

/**
 * @brief Struct representing a mouse wheel event.
 */
struct EventMouseXWheel : public EventMouse {
    float delta; ///< The amount the mouse wheel was scrolled horizontally.
};

/**
 * @brief Struct representing a mouse entered event.
 */
struct EventMouseEntered : public EventMouse {};

/**
 * @brief Struct representing a mouse exited event.
 */
struct EventMouseExited : public EventMouse {};

/**
 * @brief Struct representing a mouse button pressed event.
 */
struct EventMouseButtonPressed : public EventMouseButton {};

/**
 * @brief Struct representing a mouse button released event.
 */
struct EventMouseButtonReleased : public EventMouseButton {};

/**
 * @brief Struct representing a mouse double-clicked event.
 */
struct EventMouseDoubleClicked : public EventMouse {};

/**
 * @brief Struct representing a mouse triple-clicked event.
 */
struct EventMouseTripleClicked : public EventMouse {};

/**
 * @brief Struct representing a drag-and-drop event.
 */
struct EventDragNDrop : public EventInput {
    PointF point;                    ///< The current mouse position during the drag.
    std::optional<PointF> downPoint; ///< The initial mouse position when the drag started.
    std::shared_ptr<Object> object;  ///< The object being dragged.
    std::shared_ptr<Widget> source;  ///< The widget initiating the drag.
    std::shared_ptr<Widget> target;  ///< The target widget where the drop may occur.
};

/**
 * @brief Struct representing a dragging event, derived from EventDragNDrop.
 */
struct EventDragging : public EventDragNDrop {
    DragEventSubtype subtype; ///< Subtype of the drag event.
};

/**
 * @brief Struct representing a dropped event, derived from EventDragNDrop.
 */
struct EventDropped : public EventDragNDrop {
    DropEventSubtype subtype; ///< Subtype of the drop event.
};

/**
 * @brief Struct representing a source dragging event.
 */
struct EventSourceDragging : public EventDragging {};

/**
 * @brief Struct representing a source dropped event.
 */
struct EventSourceDropped : public EventDropped {};

/**
 * @brief Struct representing a target dragging event.
 */
struct EventTargetDragging : public EventDragging {};

/**
 * @brief Struct representing a target dropped event.
 */
struct EventTargetDropped : public EventDropped {};

/**
 * @brief Struct representing a key event, derived from EventInput.
 */
struct EventKey : public EventInput {
    KeyCode key; ///< The key involved in the event.
};

/**
 * @brief Struct representing a key pressed event.
 */
struct EventKeyPressed : public EventKey {
    bool repeat; ///< Indicates if the key press is a repeat.
};

/**
 * @brief Struct representing a key released event.
 */
struct EventKeyReleased : public EventKey {};

/**
 * @brief Struct representing a character typed event.
 */
struct EventCharacterTyped : public EventInput {
    char32_t character; ///< The character that was typed.
};

/**
 * @brief Struct representing a targeted event, derived from EventBase.
 */
struct EventTargeted : public EventBase {
    std::weak_ptr<Widget> target; ///< The target widget of the event.
};

/**
 * @brief Struct representing a focused event.
 */
struct EventFocused : public EventTargeted {
    bool keyboard; ///< Indicates if the focus is keyboard-based.
};

/**
 * @brief Struct representing a blurred event.
 */
struct EventBlurred : public EventTargeted {};

/**
 * @brief Enum representing various event types.
 */
enum class EventType {
    Undefined = 0,       ///< Undefined event type.
    MouseMoved,          ///< Mouse moved event.
    MouseYWheel,         ///< Mouse wheel event (vertical).
    MouseXWheel,         ///< Mouse wheel event (horizontal).
    MouseButtonPressed,  ///< Mouse button pressed event.
    MouseButtonReleased, ///< Mouse button released event.
    MouseDoubleClicked,  ///< Mouse double-clicked event.
    MouseTripleClicked,  ///< Mouse triple-clicked event.
    KeyPressed,          ///< Key pressed event.
    KeyReleased,         ///< Key released event.
    CharacterTyped,      ///< Character typed event.
    TargetDragging,      ///< Target dragging event.
    TargetDropped,       ///< Target dropped event.

    Focused,        ///< Focused event.
    Blurred,        ///< Blurred event.
    MouseEntered,   ///< Mouse entered event.
    MouseExited,    ///< Mouse exited event.
    SourceDragging, ///< Source dragging event.
    SourceDropped,  ///< Source dropped event.
    Count           ///< Total number of event types.
};

/**
 * @brief Helper function to convert EventType to an integer.
 * @param t The event type to convert.
 * @return The integer representation of the event type.
 */
constexpr int operator+(EventType t) {
    return static_cast<int>(t);
}

/**
 * @brief Array of event type names.
 */
extern const char* const eventTypeNames[+EventType::Count];

/**
 * @brief Typedef representing a variant of all possible events.
 */
using EventVariant = std::variant<std::monostate,           //
                                  EventMouseMoved,          //
                                  EventMouseYWheel,         //
                                  EventMouseXWheel,         //
                                  EventMouseButtonPressed,  //
                                  EventMouseButtonReleased, //
                                  EventMouseDoubleClicked,  //
                                  EventMouseTripleClicked,  //
                                  EventKeyPressed,          //
                                  EventKeyReleased,         //
                                  EventCharacterTyped,      //
                                  EventTargetDragging,      //
                                  EventTargetDropped,       //
                                                            //
                                  EventFocused,             //
                                  EventBlurred,             //
                                  EventMouseEntered,        //
                                  EventMouseExited,         //
                                  EventSourceDragging,      //
                                  EventSourceDropped>;      //

enum class WheelOrientation {
    X = +Orientation::Horizontal,
    Y = +Orientation::Vertical,
};

/**
 * @brief Event class representing a generic event with type and utility methods.
 */
struct Event : public EventVariant {
    using EventVariant::EventVariant;

    /**
     * @brief Returns the event type.
     */
    EventType type() const;

    /**
     * @brief Returns the name of the event.
     */
    std::string name() const;

    /**
     * @brief Checks if the event is valid.
     */
    explicit operator bool() const {
        return index() > 0;
    }

    /**
     * @brief Checks if the event should propagate.
     */
    bool shouldBubble() const;

    template <typename T>
    std::optional<T> as() const;

    /**
     * @brief Returns the event's unique cookie.
     */
    uint32_t cookie() const;

    /**
     * @brief Stops the event from propagating.
     */
    void stopPropagation();

    /**
     * @brief Re-injects the event into the queue.
     */
    void reinject();

    /**
     * @brief Marks the event as pass-through.
     */
    void passThrough();

    static inline const Rectangle anywhere{ -32768, -32768, 32768, 32768 };

    bool pressed(Rectangle rect, MouseButton btn = MouseButton::Left,
                 KeyModifiers mods = KeyModifiers::None) const;
    bool pressed(MouseButton btn = MouseButton::Left, KeyModifiers mods = KeyModifiers::None) const;
    bool released(Rectangle rect, MouseButton btn = MouseButton::Left,
                  KeyModifiers mods = KeyModifiers::None) const;
    bool released(MouseButton btn = MouseButton::Left, KeyModifiers mods = KeyModifiers::None) const;

    bool doubleClicked(Rectangle rect) const;
    bool tripleClicked(Rectangle rect) const;
    bool doubleClicked() const;
    bool tripleClicked() const;

    float wheelScrolled(Rectangle rect, KeyModifiers mods = KeyModifiers::None) const;
    float wheelScrolled(KeyModifiers mods = KeyModifiers::None) const;
    float wheelScrolled(WheelOrientation orientation, Rectangle rect,
                        KeyModifiers mods = KeyModifiers::None) const;
    float wheelScrolled(WheelOrientation orientation, KeyModifiers mods = KeyModifiers::None) const;

    bool keyPressed(KeyCode key, KeyModifiers mods = KeyModifiers::None) const;
    bool keyReleased(KeyCode key, KeyModifiers mods = KeyModifiers::None) const;

    bool focused() const;
    bool blurred() const;

    std::optional<char32_t> characterTyped() const;

    std::tuple<DragEvent, PointF, KeyModifiers> dragged(bool& dragActive) const;

    std::tuple<DragEvent, PointF, KeyModifiers> dragged(Rectangle rect, bool& dragActive) const;
};

/**
 * @brief Struct for managing hit test information for widgets.
 */
struct HitTestMap {
    void add(std::shared_ptr<Widget> w, Rectangle rect, bool anywhere);

    struct HTEntry {
        std::weak_ptr<Widget> widget; ///< The widget involved in the hit test.
        int zindex;                   ///< Z-index for rendering order.
        Rectangle rect;               ///< Rectangle area for the hit test.
        bool anywhere;                ///< Indicates if the widget is valid anywhere.
    };

    std::vector<HTEntry> list; ///< List of hit test entries.

    /**
     * @brief Retrieves the widget at the specified coordinates.
     * @param x X-coordinate.
     * @param y Y-coordinate.
     * @param respect_anywhere Whether to respect the "anywhere" flag.
     * @return The widget at the specified coordinates, if any.
     */
    std::shared_ptr<Widget> get(float x, float y, bool respect_anywhere) const;

    std::shared_ptr<Widget> get(PointF p, bool respect_anywhere) const {
        return get(p.x, p.y, respect_anywhere);
    }

    /**
     * @brief Clears all hit test entries.
     */
    void clear();

    struct {
        int zindex            = 0;
        bool visible          = true;
        bool inTabGroup       = false;
        bool mouseTransparent = false;
        Rectangle scissors    = Event::anywhere;
    } state;

    int tabGroupId = 0;
};

/**
 * @brief Manages the input queue, handling event dispatch and processing.
 */
struct InputQueue {
    HitTestMap hitTest;
    std::weak_ptr<Widget> focused;
    std::weak_ptr<const Widget> activeHint;
    std::vector<std::weak_ptr<Widget>> capturingMouse; ///< List of widgets capturing the mouse input.
    std::vector<std::weak_ptr<Widget>> capturingKeys;  ///< List of widgets capturing keyboard input.
    std::vector<std::weak_ptr<Widget>> tabList;
    std::weak_ptr<Widget> autoFocus;
    std::weak_ptr<Widget> dragSource;
    std::shared_ptr<Object> dragObject;
    std::weak_ptr<Widget> dragTarget;
    MouseButton dragButton   = MouseButton::Left;
    bool dropAllowed         = false;
    bool draggingOnSource    = false;

    int focusCaptureLevel    = 0;
    int maxFocusCaptureLevel = 0;

    std::deque<Event> events;
    std::vector<Event> injectedEvents;
    function<void(Event&)> unhandledEvent;
    bool passThroughFlag = false;
    std::weak_ptr<Widget> passedThroughBy;
    std::optional<EventMouse> lastMouseEvent;
    std::optional<EventInput> lastInputEvent;
    std::shared_ptr<Widget>
        eventTarget; ///< The target widget of the event currently processed, unaffected by bubbling.

    PointF mousePos{ -1.f, -1.f }; ///< Mouse position relative to the window.
    KeyModifiers keyModifiers{ KeyModifiers::None };
    Trigger<> trigMousePos;
    Trigger<> trigKeyModifiers;

    /**
     * Adds a widget to the tab stop list.
     * @param ptr The widget to add.
     */
    void addTabStop(std::weak_ptr<Widget> ptr);

    /**
     * Sets the auto-focus widget.
     * @param ptr The widget to auto-focus.
     */
    void setAutoFocus(std::weak_ptr<Widget> ptr);

    /**
     * Increases the focus capture level.
     */
    void enterFocusCapture();

    /**
     * Decreases the focus capture level.
     */
    void leaveFocusCapture();

    /**
     * Checks if any widget currently has focus.
     * @return True if a widget has focus, false otherwise.
     */
    bool hasFocus();

    /**
     * Handles mouse leave events.
     */
    void mouseLeave();

    /**
     * Processes the mouse state for the given target widget.
     * @param target The target widget.
     */
    void processMouseState(const std::shared_ptr<Widget>& target);

    /**
     * Begins dragging an object from the given source widget.
     * @param dragSource The source widget initiating the drag.
     * @param dragObject The object being dragged.
     * @param btn The mouse button involved in the drag.
     */
    void beginDrag(std::shared_ptr<Widget> dragSource, std::shared_ptr<Object> dragObject, MouseButton btn);

    /**
     * Checks if an object is currently being dragged.
     * @return True if dragging, false otherwise.
     */
    bool isDragging() const;

    /**
     * Allows the current drag to be dropped.
     */
    void allowDrop();

    /**
     * Cancels the current drag operation.
     */
    void cancelDragging();

    /**
     * Sets the last mouse event processed.
     * @param e The last mouse event.
     */
    void setLastMouseEvent(EventMouse e);

    /**
     * Sets the last input event processed.
     * @param e The last input event.
     */
    void setLastInputEvent(EventInput e);

    /**
     * Returns the mouse position relative to the window if it lies within the given widget.
     * @param widget The widget to check.
     * @return The mouse position relative to the widget, if applicable.
     */
    std::optional<PointF> mousePosFor(Widget* widget) const;

    /**
     * Returns the mouse position relative to the widget's client area, if within its bounds.
     * @param widget The widget to check.
     * @return The mouse position relative to the widget's client area, if applicable.
     */
    std::optional<PointF> mousePosForClient(Widget* widget) const;

    /**
     * Gets the widget at the specified point, starting from a specified offset.
     * @param pt The point to check.
     * @param offset The offset to start from (-1 starts from capturingMouse.front()).
     * @param respect_anywhere Whether to respect the "anywhere" flag.
     * @return A tuple containing the widget at the point and the corresponding index.
     */
    std::tuple<std::shared_ptr<Widget>, int> getAt(Point pt, int offset = -1,
                                                   bool respect_anywhere = true) const;

    /**
     * Calls the provided function for each widget at the current mouse position, optionally bubbling.
     * @param fn The function to call for each widget.
     * @param bubble Whether to allow event bubbling.
     * @param useMouseCapture Whether to use mouse capture information.
     * @return True if the function succeeds, false otherwise.
     */
    bool mouseAtBubble(const function<bool(Widget*)>& fn, bool bubble = true,
                       bool useMouseCapture = true) const;

    template <typename T>
    std::optional<T> getAtMouse(const function<std::optional<T>(Widget*)>& fn, bool bubble = true,
                                bool useMouseCapture = true) const {
        std::optional<T> value;
        if (mouseAtBubble(
                [&](Widget* w) BRISK_INLINE_LAMBDA -> bool {
                    if ((value = fn(w))) {
                        return false;
                    }
                    return true;
                },
                bubble, useMouseCapture)) {
            return value;
        }
        return std::nullopt;
    }

    /**
     * Gets the description of the widget under the mouse, if any.
     * @return The description of the widget, if applicable.
     */
    std::optional<std::string> getDescriptionAtMouse() const;

    /**
     * Gets the cursor type for the widget under the mouse, if any.
     * @return The cursor type, if applicable.
     */
    std::optional<Cursor> getCursorAtMouse() const;

    /**
     * Sets the focus to the specified widget.
     * @param focus The widget to focus.
     * @param keyboard Whether the focus is set by keyboard.
     */
    void setFocus(std::shared_ptr<Widget> focus, bool keyboard);

    /**
     * Resets the current focus.
     */
    void resetFocus();

    /**
     * Resets the input queue state.
     */
    void reset();

    /**
     * Captures the mouse for the specified widget.
     * @param target The widget to capture the mouse.
     */
    void captureMouse(const std::shared_ptr<Widget>& target);

    /**
     * Captures keyboard input for the specified widget.
     * @param target The widget to capture keyboard input.
     */
    void captureKeys(const std::shared_ptr<Widget>& target);

    /**
     * Stops capturing the mouse for the specified widget.
     * @param target The widget to stop capturing the mouse.
     */
    void stopCaptureMouse(const std::shared_ptr<Widget>& target);

    /**
     * Stops capturing keyboard input for the specified widget.
     * @param target The widget to stop capturing keyboard input.
     */
    void stopCaptureKeys(const std::shared_ptr<Widget>& target);

    /**
     * Handles focus-related events.
     * @param e The event to handle.
     */
    void handleFocusEvents(Event& e);

    /**
     * Injects an event into the queue to be processed at the next frame.
     * @param event The event to inject.
     */
    void injectEvent(Event event);

    /**
     * Adds an event to the queue to be processed in the next call to processEvent.
     * @param event The event to add.
     */
    void addEvent(Event event);

    /**
     * Processes all events in the queue.
     */
    void processEvents();

    /**
     * Processes a key event.
     * @param e The key event to process.
     */
    void processKeyEvent(Event e);

    /**
     * Processes a mouse event.
     * @param e The mouse event to process.
     */
    void processMouseEvent(Event e);

    /**
     * Processes a drag event.
     * @param e The drag event to process.
     */
    void processDragEvent(Event e);

    /**
     * Processes a targeted event.
     * @param e The targeted event to process.
     */
    void processTargetedEvent(Event e);

    /**
     * Constructor for the InputQueue class.
     */
    InputQueue();

private:
    BindingRegistration registration;
};

extern ImplicitContext<InputQueue*, InputQueue*, briskMultithreadRender> inputQueue;

using InputQueueScope = ImplicitContextScope<InputQueue*>;

} // namespace Brisk
