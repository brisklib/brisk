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
#include <brisk/gui/Event.hpp>
#include <brisk/gui/Gui.hpp>

namespace Brisk {

std::atomic_uint32_t eventCookie{ 0 };

const char* const eventTypeNames[+EventType::Count] = {
    "Undefined",
    "MouseMoved",
    "MouseYWheel",
    "MouseXWheel",
    "MouseButtonPressed",
    "MouseButtonReleased",
    "MouseDoubleClicked",
    "MouseTripleClicked",
    "KeyPressed",
    "KeyReleased",
    "CharacterTyped",
    "TargetDragging",
    "TargetDropped",

    "Focused",
    "Blurred",
    "MouseEntered",
    "MouseExited",
    "SourceDragging",
    "SourceDropped",
};

template <typename T>
std::optional<T> Event::as() const {
    std::optional<T> result;
    std::visit(
        [&](const auto& val) BRISK_INLINE_LAMBDA {
            if constexpr (std::is_base_of_v<T, std::decay_t<decltype(val)>>)
                result = val;
        },
        static_cast<const EventVariant&>(*this));
    return result;
}

template std::optional<EventMouseMoved> Event::as<EventMouseMoved>() const;
template std::optional<EventMouseYWheel> Event::as<EventMouseYWheel>() const;
template std::optional<EventMouseXWheel> Event::as<EventMouseXWheel>() const;
template std::optional<EventMouseButtonPressed> Event::as<EventMouseButtonPressed>() const;
template std::optional<EventMouseButtonReleased> Event::as<EventMouseButtonReleased>() const;
template std::optional<EventMouseDoubleClicked> Event::as<EventMouseDoubleClicked>() const;
template std::optional<EventMouseTripleClicked> Event::as<EventMouseTripleClicked>() const;
template std::optional<EventKeyPressed> Event::as<EventKeyPressed>() const;
template std::optional<EventKeyReleased> Event::as<EventKeyReleased>() const;
template std::optional<EventCharacterTyped> Event::as<EventCharacterTyped>() const;
template std::optional<EventTargetDragging> Event::as<EventTargetDragging>() const;
template std::optional<EventTargetDropped> Event::as<EventTargetDropped>() const;
template std::optional<EventFocused> Event::as<EventFocused>() const;
template std::optional<EventBlurred> Event::as<EventBlurred>() const;
template std::optional<EventMouseEntered> Event::as<EventMouseEntered>() const;
template std::optional<EventMouseExited> Event::as<EventMouseExited>() const;
template std::optional<EventSourceDragging> Event::as<EventSourceDragging>() const;
template std::optional<EventSourceDropped> Event::as<EventSourceDropped>() const;
template std::optional<EventBase> Event::as<EventBase>() const;
template std::optional<EventInput> Event::as<EventInput>() const;
template std::optional<EventMouse> Event::as<EventMouse>() const;
template std::optional<EventMouseButton> Event::as<EventMouseButton>() const;
template std::optional<EventDragNDrop> Event::as<EventDragNDrop>() const;
template std::optional<EventDragging> Event::as<EventDragging>() const;
template std::optional<EventDropped> Event::as<EventDropped>() const;
template std::optional<EventKey> Event::as<EventKey>() const;
template std::optional<EventTargeted> Event::as<EventTargeted>() const;

void HitTestMap::clear() {
    list.clear();
    tabGroupId = 0;
}

void HitTestMap::add(std::shared_ptr<Widget> w, Rectangle rect, bool anywhere, int zindex) {
    if (rect.empty())
        return;
    auto it = std::lower_bound(list.begin(), list.end(), zindex,
                               [](const HitTestEntry& e, int zindex) BRISK_INLINE_LAMBDA {
                                   return e.zindex < zindex;
                               });
    list.insert(it, HitTestEntry{ w, zindex, rect, anywhere });
}

std::shared_ptr<Widget> HitTestMap::get(float x, float y, bool respect_anywhere) const {
    for (const HitTestEntry& e : list) {
        if (e.rect.contains(Point(x, y)) || (respect_anywhere && e.anywhere)) {
            return e.widget.lock();
        }
    }
    return nullptr;
}

void InputQueue::reset() {
    hitTest.clear();
    tabList.clear();
    focusCaptureLevel    = 0;
    maxFocusCaptureLevel = 0;
}

void InputQueue::addTabStop(std::weak_ptr<Widget> ptr) {
    if (maxFocusCaptureLevel == focusCaptureLevel)
        tabList.push_back(std::move(ptr));
}

void InputQueue::enterFocusCapture() {
    ++focusCaptureLevel;
    tabList.clear();
    maxFocusCaptureLevel = focusCaptureLevel;
}

void InputQueue::leaveFocusCapture() {
    --focusCaptureLevel;
}

void InputQueue::captureMouse(const std::shared_ptr<Widget>& target) {
    capturingMouse.push_back(std::weak_ptr<Widget>(target));
}

void InputQueue::captureKeys(const std::shared_ptr<Widget>& target) {
    capturingKeys.push_back(std::weak_ptr<Widget>(target));
}

void InputQueue::stopCaptureMouse(const std::shared_ptr<Widget>& target) {
    auto it = std::find_if(capturingMouse.begin(), capturingMouse.end(),
                           [&](std::weak_ptr<Widget> w) BRISK_INLINE_LAMBDA {
                               return w.lock() == target;
                           });
    if (it != capturingMouse.end())
        capturingMouse.erase(it);
}

void InputQueue::stopCaptureKeys(const std::shared_ptr<Widget>& target) {
    auto it = std::find_if(capturingKeys.begin(), capturingKeys.end(),
                           [&](std::weak_ptr<Widget> w) BRISK_INLINE_LAMBDA {
                               return w.lock() == target;
                           });
    if (it != capturingKeys.end())
        capturingKeys.erase(it);
}

void InputQueue::beginDrag(std::shared_ptr<Widget> dragSource, std::shared_ptr<Object> dragObject,
                           MouseButton btn) {
    captureMouse(dragSource);
    captureKeys(dragSource);
    this->dragSource = std::move(dragSource);
    this->dragObject = std::move(dragObject);
    this->dragTarget.reset();
    this->dragButton       = btn;
    this->draggingOnSource = true;
}

void InputQueue::addEvent(Event event) {
    events.push_back(std::move(event));
}

void InputQueue::injectEvent(Event event) {
    injectedEvents.push_back(std::move(event));
}

void InputQueue::setFocus(std::shared_ptr<Widget> focus, bool keyboard) {
    if (auto previous = focused.lock()) {
        addEvent(EventBlurred{ { {}, std::move(previous) } });
    }
    if (focus && !focus->m_tabStop) {
        // If the widget doesn't accept focus, find the first descendant that does
        focus = focus->find(
            [](Rc<Widget> w) -> bool {
                return w->m_tabStop;
            },
            MatchAny{});
    }
    focused = focus;

    if (focus) {
        addEvent(EventFocused{ { {}, std::move(focus) }, keyboard });
    }
}

void InputQueue::resetFocus() {
    setFocus(nullptr, false);
}

void InputQueue::handleFocusEvents(Event& e) {
    auto kp = e.as<EventKeyPressed>();
    if (kp && !tabList.empty()) {
        Rc<Widget> previousFocused = focused.lock();
        std::vector<Rc<Widget>> tabList;
        for (const std::weak_ptr<Widget>& w : this->tabList) {
            if (auto sh = w.lock(); sh && !sh->isDisabled())
                tabList.push_back(std::move(sh));
        }

        int tabGroupId   = previousFocused ? previousFocused->m_tabGroupId : -1;
        bool changeFocus = false;
        bool match_group;
        int index = std::find(tabList.begin(), tabList.end(), previousFocused) - tabList.begin();
        if (index == tabList.size())
            index = -1;

        struct GroupIdCmp {
            bool operator()(const Rc<Widget>& x, int y) const {
                return x->m_tabGroupId < y;
            }

            bool operator()(int x, const Rc<Widget>& y) const {
                return x < y->m_tabGroupId;
            }
        };

        auto range = std::equal_range(tabList.begin(), tabList.end(), tabGroupId, GroupIdCmp{});
        if (range.second == range.first) {
            range.first  = tabList.begin();
            range.second = tabList.end();
        }
        int begin;
        int end; // past-the-end index
        if (kp->key == KeyCode::Tab) {
            match_group = false;
            changeFocus = true;
            if (kp->mods && KeyModifiers::Shift) {
                begin = tabList.size() - 1;
                end   = -1;
            } else {
                begin = 0;
                end   = tabList.size();
            }
            e.stopPropagation();
        } else if (kp->key == KeyCode::Left || kp->key == KeyCode::Up) {
            match_group = true;
            changeFocus = true;
            begin       = range.second - tabList.begin() - 1;
            end         = range.first - tabList.begin() - 1;
            e.stopPropagation();
        } else if (kp->key == KeyCode::Right || kp->key == KeyCode::Down) {
            match_group = true;
            changeFocus = true;
            begin       = range.first - tabList.begin();
            end         = range.second - tabList.begin();
            e.stopPropagation();
        }
        if (changeFocus && end != begin) {
            int step = end > begin ? 1 : -1;
            if (index == -1 || index == end)
                index = begin;
            else {
                int found = begin;
                for (int i = index + step; i != end; i += step) {
                    bool match = tabList[i]->m_tabGroupId == tabGroupId;
                    if (!match_group)
                        match = !match;
                    if (match) {
                        found = i;
                        break;
                    }
                }
                index = found;
            }
            BRISK_ASSERT(index >= 0);
            BRISK_ASSERT(index < tabList.size());
            setFocus(tabList[index], true);
        }
    }
}

void InputQueue::processKeyEvent(Event e) {
    if (isDragging() && e.keyPressed(KeyCode::Escape)) {
        cancelDragging();
    }
    auto capturingKeys = this->capturingKeys;
    for (auto it = capturingKeys.rbegin(); it != capturingKeys.rend(); ++it) {
        if (Rc<Widget> w = it->lock())
            w->processEvent(e);
        if (!e)
            break;
    }
    if (e) {
        if (Rc<Widget> target = focused.lock())
            target->processEvent(e);
    }

    handleFocusEvents(e);

    if (e && unhandledEvent) {
        unhandledEvent(e);
    }
}

std::optional<std::string> InputQueue::getHintAtMouse() const {
    return getAtMouse<std::string>([](Widget* w) BRISK_INLINE_LAMBDA {
        return !w->m_hint.empty() ? std::optional<std::string>(w->m_hint) : std::nullopt;
    });
}

std::optional<Cursor> InputQueue::getCursorAtMouse() const {
    if (isDragging()) {
        bool allowed = dropAllowed || draggingOnSource;
        return allowed ? Cursor::Grab : Cursor::GrabDeny;
    }
    return getAtMouse<Cursor>([](Widget* w) BRISK_INLINE_LAMBDA {
        return w->m_cursor != Cursor::NotSet ? std::optional<Cursor>(w->m_cursor) : std::nullopt;
    });
}

std::tuple<std::shared_ptr<Widget>, int> InputQueue::getAt(Point pt, int offset,
                                                           bool respect_anywhere) const {
    if (offset < 0 && !capturingMouse.empty())
        return std::tuple<std::shared_ptr<Widget>, int>{ capturingMouse.back().lock(), 0 };
    for (int i = std::max(0, offset); i < hitTest.list.size(); i++) {
        if (Rc<Widget> w = hitTest.list[i].widget.lock();
            w && (hitTest.list[i].rect.contains(pt) || (respect_anywhere && hitTest.list[i].anywhere)))
            return std::tuple<std::shared_ptr<Widget>, int>{ w, i + 1 };
    }
    return std::tuple<std::shared_ptr<Widget>, int>{ nullptr, INT_MAX };
}

bool InputQueue::mouseAtBubble(const function<bool(Widget*)>& fn, bool bubble, bool useMouseCapture) const {

    if (lastMouseEvent) {
        std::shared_ptr<Widget> w = std::get<0>(getAt(lastMouseEvent->point, useMouseCapture ? -1 : 0));
        if (w) {
            w->bubble(fn);
            return true;
        }
    }
    return false;
}

void InputQueue::cancelDragging() {
    if (!lastMouseEvent)
        return;

    std::shared_ptr<Widget> source = dragSource.lock();
    if (!source)
        return;

    std::shared_ptr<Object> object = dragObject;
    if (!object)
        return;

    std::shared_ptr<Widget> previousTarget = dragTarget.lock();

    EventDragNDrop dragBase{ *lastMouseEvent, lastMouseEvent->point, lastMouseEvent->downPoint, object,
                             source,          previousTarget };
    EventDropped event{ dragBase, DropEventSubtype::Cancel };
    source->processTemporaryEvent(EventSourceDropped{ event });
    if (previousTarget)
        previousTarget->processTemporaryEvent(EventTargetDropped{ event });
    stopCaptureMouse(source);
    stopCaptureKeys(source);
    dragObject.reset();
    dragTarget.reset();
    dragSource.reset();
    dropAllowed = false;
}

void InputQueue::processDragEvent(Event e) {
    // Process mouse event and generate all needed drag&drop events
    std::optional<EventMouse> base         = e.as<EventMouse>();

    std::shared_ptr<Widget> previousTarget = dragTarget.lock();

    std::shared_ptr<Widget> source         = dragSource.lock();
    if (!source)
        return;
    auto lookupResult              = getAt(base->point, 0, false);
    auto [target, index]           = lookupResult;

    std::shared_ptr<Object> object = dragObject;
    if (!object)
        return;

    EventDragNDrop dragBase{ *base, base->point, base->downPoint, object, source, target };
    if (e.type() == EventType::MouseMoved) {
        dropAllowed = false;
        if (previousTarget == target) {
            if (target) {
                EventDragging event{ dragBase, DragEventSubtype::Over };
                source->processTemporaryEvent(EventSourceDragging{ event });
                target->processTemporaryEvent(EventTargetDragging{ event });
            }
        } else {
            if (previousTarget) {
                EventDragging event{ dragBase, DragEventSubtype::Exit };
                source->processTemporaryEvent(EventSourceDragging{ event });
                previousTarget->processTemporaryEvent(EventTargetDragging{ event });
            }
            if (target) {
                EventDragging event{ dragBase, DragEventSubtype::Enter };
                source->processTemporaryEvent(EventSourceDragging{ event });
                target->processTemporaryEvent(EventTargetDragging{ event });
            }
        }
        dragTarget = target;
    } else if (e.type() == EventType::MouseButtonReleased &&
               e.as<EventMouseButtonReleased>()->button == dragButton) {
        EventDropped event{ dragBase,
                            dropAllowed && target ? DropEventSubtype::Drop : DropEventSubtype::Cancel };
        source->processTemporaryEvent(EventSourceDropped{ event });
        if (target)
            target->processTemporaryEvent(EventTargetDropped{ event });
        stopCaptureMouse(source);
        stopCaptureKeys(source);
        dragObject.reset();
        dragTarget.reset();
        dragSource.reset();
        dropAllowed = false;
    }

    draggingOnSource = source->rect().contains(base->point);
}

void InputQueue::processMouseEvent(Event e) {
    std::optional<EventMouse> base = e.as<EventMouse>();
    BRISK_ASSERT(!!base);

    if (isDragging()) {
        processDragEvent(e);
    }

    auto [target, index] = getAt(base->point);

    passThroughFlag      = false;
    passedThroughBy.reset();
    while (target) {
        eventTarget = target;
        target->processEvent(e);
        if (passThroughFlag || target->m_mousePassThrough) {
            passedThroughBy         = target;
            std::tie(target, index) = getAt(base->point, index);
            passThroughFlag         = false;
            stopCaptureMouse(target);
        } else {
            break;
        }
    }
    eventTarget = nullptr;
    passedThroughBy.reset();

    if (e.pressed()) {
        resetFocus();
        e.stopPropagation();
    }
    if (e && unhandledEvent) {
        unhandledEvent(e);
    }
}

void InputQueue::processTargetedEvent(Event e) {
    std::optional<EventTargeted> targeted = e.as<EventTargeted>();
    BRISK_ASSERT(!!targeted);
    if (Rc<Widget> target = targeted->target.lock()) {
        target->processEvent(e);
    }
    if (e && unhandledEvent) {
        unhandledEvent(e);
    }
}

template <typename T>
static bool isVisible(const std::weak_ptr<T>& w) {
    auto ww = w.lock();
    return ww && ww->isVisible();
}

template <typename T>
static void cleanup(std::vector<std::weak_ptr<T>>& vec) {
    vec.erase(std::remove_if(vec.begin(), vec.end(),
                             [](const std::weak_ptr<T>& w) BRISK_INLINE_LAMBDA {
                                 return !isVisible(w);
                             }),
              vec.end());
}

void InputQueue::processEvents() {
    if (isVisible(autoFocus) && !isVisible(focused)) {
        auto w = autoFocus.lock();
        if (!w->m_autofocusReceived) {
            w->m_autofocusReceived = true;
            setFocus(w, false);
        }
    }
    cleanup(tabList);
    cleanup(capturingKeys);
    cleanup(capturingMouse);
    if (!isVisible(focused)) {
        focused.reset();
    }

    if (events.empty() && injectedEvents.empty())
        return;

    while (!events.empty()) {
        Event e = std::move(events.front());
        events.pop_front();
        switch (e.type()) {
        case EventType::CharacterTyped:
        case EventType::KeyPressed:
        case EventType::KeyReleased:
            setLastInputEvent(*e.as<EventInput>());
            processKeyEvent(std::move(e));
            break;
        case EventType::MouseButtonPressed:
        case EventType::MouseButtonReleased:
        case EventType::MouseMoved:
        case EventType::MouseEntered:
        case EventType::MouseExited:
        case EventType::MouseYWheel:
        case EventType::MouseDoubleClicked:
        case EventType::MouseTripleClicked:
            setLastMouseEvent(*e.as<EventMouse>());
            setLastInputEvent(*e.as<EventInput>());
            processMouseEvent(std::move(e));
            break;
        case EventType::SourceDragging:
        case EventType::SourceDropped:
        case EventType::TargetDragging:
        case EventType::TargetDropped:
            // Drag and Drop events are routed directly to the appropriate objects; they do not go through the
            // event queue.
            BRISK_ASSERT(false);
            break;
        case EventType::Focused:
        case EventType::Blurred:
            processTargetedEvent(std::move(e));
            break;
        default:
            break;
        }
    }

    if (lastMouseEvent) {
        Rc<Widget> target = std::get<0>(getAt(lastMouseEvent->point));
        processMouseState(target);
    }

    if (!injectedEvents.empty()) {
        events.insert(events.end(), std::make_move_iterator(injectedEvents.begin()),
                      std::make_move_iterator(injectedEvents.end()));
        injectedEvents.clear();
    }
}

void InputQueue::processMouseState(const std::shared_ptr<Widget>& target) {
    if (target) {
        std::set<Widget*> parents;
        target->bubble([&](Widget* w) BRISK_INLINE_LAMBDA -> bool {
            parents.insert(w);
            return true;
        });

        for (const HitTestMap::HitTestEntry& entry : hitTest.list) {
            // All parents are sorted after their children,
            // so hover events are processed in child-parent order,
            // which is the correct behavior.

            if (Rc<Widget> ww = entry.widget.lock()) {
                if (parents.find(ww.get()) != parents.end()) {
                    // Widget is hovered
                    if (!(ww->m_state && WidgetState::Hover)) {
                        ww->toggleState(WidgetState::Hover, true);
                        EventMouseEntered e;
                        static_cast<EventMouse&>(e) = *lastMouseEvent;
                        ww->processTemporaryEvent(Event(e));
                    }
                } else {
                    if ((ww->m_state && WidgetState::Hover)) {
                        ww->toggleState(WidgetState::Hover, false);
                        EventMouseExited e;
                        static_cast<EventMouse&>(e) = *lastMouseEvent;
                        ww->processTemporaryEvent(Event(e));
                    }
                }
            }
        }
    } else {
        for (const HitTestMap::HitTestEntry& entry : hitTest.list) {
            // All parents are sorted after their children,
            // so hover events are processed in child-parent order,
            // which is the correct behavior.

            if (Rc<Widget> ww = entry.widget.lock()) {
                if ((ww->m_state && WidgetState::Hover)) {
                    ww->toggleState(WidgetState::Hover, false);
                    EventMouseExited e;
                    static_cast<EventMouse&>(e) = *lastMouseEvent;
                    ww->processTemporaryEvent(Event(e));
                }
            }
        }
    }
}

void InputQueue::mouseLeave() {
    processMouseState(nullptr);
}

std::optional<PointF> InputQueue::mousePosFor(Widget* widget) const {
    if (!lastMouseEvent)
        return std::nullopt;
    if (!widget->rect().contains(lastMouseEvent->point))
        return std::nullopt;
    return lastMouseEvent->point;
}

std::optional<PointF> InputQueue::mousePosForClient(Widget* widget) const {
    if (!lastMouseEvent)
        return std::nullopt;
    Rectangle client = widget->clientRect();
    if (!client.contains(lastMouseEvent->point))
        return std::nullopt;
    return lastMouseEvent->point - PointF(client.p1);
}

EventType Event::type() const {
    return static_cast<EventType>(index());
}

std::tuple<DragEvent, PointF, KeyModifiers> Event::dragged(Rectangle rect, bool& dragActive) const {
    auto mouseDown  = as<EventMouseButtonPressed>();
    auto mouseUp    = as<EventMouseButtonReleased>();
    auto mouseMoved = as<EventMouseMoved>();
    if (mouseDown && mouseDown->button != MouseButton::Left)
        mouseDown = std::nullopt;
    if (mouseUp && mouseUp->button != MouseButton::Left)
        mouseUp = std::nullopt;

    if (!dragActive && mouseDown && rect.contains(mouseDown->point)) {
        dragActive = true;
        return { DragEvent::Started, { 0.f, 0.f }, mouseDown->mods };
    } else if (mouseUp && dragActive) {
        dragActive = false;
        return { DragEvent::Dropped, mouseUp->point - *mouseUp->downPoint, mouseUp->mods };
    } else if (mouseMoved && dragActive) {
        if (mouseMoved->downPoint)
            return { DragEvent::Dragging, mouseMoved->point - *mouseMoved->downPoint, mouseMoved->mods };
        else
            return { DragEvent::Dropped, { 0.f, 0.f }, mouseMoved->mods };
    }
    return { DragEvent::None, PointF(), KeyModifiers::None };
}

std::tuple<DragEvent, PointF, KeyModifiers> Event::dragged(bool& dragActive) const {
    return dragged(anywhere, dragActive);
}

bool Event::focused() const {
    auto e = as<EventFocused>();
    if (e)
        return true;
    return false;
}

bool Event::blurred() const {
    auto e = as<EventBlurred>();
    if (e)
        return true;
    return false;
}

std::optional<char32_t> Event::characterTyped() const {
    auto e = as<EventCharacterTyped>();
    if (e)
        return e->character;
    return std::nullopt;
}

bool Event::keyReleased(KeyCode key, KeyModifiers mods) const {
    auto e = as<EventKeyReleased>();
    return e && e->key == key && ((e->mods & KeyModifiers::Regular) == mods);
}

bool Event::keyPressed(KeyCode key, KeyModifiers mods) const {
    auto e = as<EventKeyPressed>();
    return e && e->key == key && ((e->mods & KeyModifiers::Regular) == mods);
}

float Event::wheelScrolled(KeyModifiers mods) const {
    return wheelScrolled(WheelOrientation::Y, anywhere, mods);
}

float Event::wheelScrolled(Rectangle rect, KeyModifiers mods) const {
    return wheelScrolled(WheelOrientation::Y, rect, mods);
}

float Event::wheelScrolled(WheelOrientation orientation, KeyModifiers mods) const {
    return wheelScrolled(orientation, anywhere, mods);
}

float Event::wheelScrolled(WheelOrientation orientation, Rectangle rect, KeyModifiers mods) const {
    if (orientation == WheelOrientation::X) {
        auto e = as<EventMouseXWheel>();
        if (e && rect.contains(e->point) && ((e->mods & mods) == mods)) {
            return e->delta;
        }
    } else {
        auto e = as<EventMouseYWheel>();
        if (e && rect.contains(e->point) && ((e->mods & mods) == mods)) {
            return e->delta;
        }
    }
    return 0;
}

bool Event::tripleClicked() const {
    return tripleClicked(anywhere);
}

bool Event::doubleClicked() const {
    return doubleClicked(anywhere);
}

bool Event::tripleClicked(Rectangle rect) const {
    auto e = as<EventMouseTripleClicked>();
    return e && rect.contains(e->point);
}

bool Event::doubleClicked(Rectangle rect) const {
    auto e = as<EventMouseDoubleClicked>();
    return e && rect.contains(e->point);
}

bool Event::released(MouseButton btn, KeyModifiers mods) const {
    return released(anywhere, btn, mods);
}

bool Event::released(Rectangle rect, MouseButton btn, KeyModifiers mods) const {
    auto e = as<EventMouseButtonReleased>();
    return e && e->button == btn && rect.contains(e->point) && ((e->mods & mods) == mods);
}

bool Event::pressed(MouseButton btn, KeyModifiers mods) const {
    return pressed(anywhere, btn, mods);
}

bool Event::pressed(Rectangle rect, MouseButton btn, KeyModifiers mods) const {
    auto e = as<EventMouseButtonPressed>();
    return e && e->button == btn && rect.contains(e->point) && ((e->mods & mods) == mods);
}

void Event::stopPropagation() {
    *this = std::monostate{};
}

uint32_t Event::cookie() const {
    return as<EventBase>()->cookie;
}

bool Event::shouldBubble() const {
    return type() < EventType::Focused;
}

std::string Event::name() const {
    return eventTypeNames[index()];
}

void InputQueue::setLastInputEvent(EventInput e) {
    lastInputEvent = e;
    assignAndTrigger(keyModifiers, e.mods, trigKeyModifiers);
}

void InputQueue::setLastMouseEvent(EventMouse e) {
    lastMouseEvent = e;
    assignAndTrigger(mousePos, e.point, trigMousePos);
}

void InputQueue::allowDrop() {
    dropAllowed = true;
}

bool InputQueue::isDragging() const {
    return static_cast<bool>(dragSource.lock());
}

void InputQueue::setAutoFocus(std::weak_ptr<Widget> ptr) {
    autoFocus = std::move(ptr);
}

bool InputQueue::hasFocus() {
    return static_cast<bool>(focused.lock());
}

InputQueue::InputQueue() : registration{ this, uiScheduler } {}

void InputQueue::passThrough() {
    passThroughFlag = true;
}

void InputQueue::startMenu(std::shared_ptr<Widget> widget) {
    finishMenu();
    menuRoot = widget;
}

void InputQueue::finishMenu() {
    if (auto menu = menuRoot.lock()) {
        menu->visible = false;
    }
    menuRoot.reset();
}

} // namespace Brisk
