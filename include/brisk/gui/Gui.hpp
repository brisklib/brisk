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

#include <brisk/core/Binding.hpp>
#include <brisk/core/BasicTypes.hpp>
#include <brisk/core/Utilities.hpp>
#include <brisk/core/MetaClass.hpp>

BRISK_CLANG_PRAGMA(clang diagnostic push)
BRISK_CLANG_PRAGMA(clang diagnostic ignored "-Wc++2a-extensions")

#include <brisk/window/Types.hpp>
#include <brisk/window/Window.hpp>
#include <brisk/core/Compression.hpp>
#include <brisk/core/Utilities.hpp>
#include <brisk/core/Time.hpp>
#include <brisk/core/Settings.hpp>
#include <brisk/core/Threading.hpp>
#include <brisk/core/internal/Typename.hpp>
#include <brisk/graphics/Canvas.hpp>
#include <brisk/graphics/Color.hpp>
#include <set>
#include <fmt/ranges.h>
#include <spdlog/spdlog.h>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>
#include <type_traits>
#include <utility>
#include <brisk/core/internal/SmallVector.hpp>
#include "internal/Animation.hpp"
#include "Properties.hpp"
#include "Event.hpp"
#include "WidgetTree.hpp"

namespace Brisk {

void registerBuiltinFonts();

class Widget;

void boxPainter(Canvas& canvas, const Widget& widget, RectangleF rect);
void boxPainter(Canvas& canvas, const Widget& widget);

namespace Internal {
extern std::atomic_bool debugRelayoutAndRegenerate;
extern std::atomic_bool debugBoundaries;
extern std::atomic_bool debugDirtyRect;
} // namespace Internal

class Stylesheet;
struct Rules;

struct EventDelegate;

struct Painter {
    using PaintFunc    = function<void(Canvas&, const Widget&)>;

    Painter() noexcept = default;
    explicit Painter(PaintFunc painter);
    PaintFunc painter;

    void paint(Canvas& canvas, const Widget& w) const;

    explicit operator bool() const noexcept {
        return static_cast<bool>(painter);
    }

    bool operator==(const Painter&) const noexcept = default;
};

enum class BuilderKind {
    Regular,
    Delayed,
    Once,
};

struct Builder {
    using PushFunc = function<void(Widget*)>;

    explicit Builder(PushFunc builder, BuilderKind kind = BuilderKind::Delayed) noexcept;
    PushFunc builder;
    BuilderKind kind = BuilderKind::Delayed;

    void run(Widget* w);
};

constexpr bool isInheritable(PropFlags flags) noexcept {
    return flags && PropFlags::Inheritable;
}

constexpr bool isTransition(PropFlags flags) noexcept {
    return flags && PropFlags::Transition;
}

constexpr bool isResolvable(PropFlags flags) noexcept {
    return flags && PropFlags::Resolvable;
}

namespace Tag {
struct Depends {
    using Type = Value<Trigger<>>;

    static std::string_view name() noexcept {
        return "depends";
    }

    constexpr static PropFlags flags = PropFlags ::None;
};

} // namespace Tag

inline namespace Arg {
constexpr inline Argument<Tag::Depends> depends{};
}

struct SingleBuilder : Builder {
    using func = function<Rc<Widget>()>;

    explicit SingleBuilder(func builder) noexcept;
};

struct IndexedBuilder : Builder {
    using func = function<Rc<Widget>(size_t index)>;

    explicit IndexedBuilder(func builder) noexcept;
};

template <typename T>
struct ListBuilder : IndexedBuilder {
    explicit ListBuilder(std::vector<T> list,
                         function<Rc<Widget>(const std::type_identity_t<T>&)> fn) noexcept
        : IndexedBuilder([list = std::move(list), fn = std::move(fn)](size_t index)
                             BRISK_INLINE_LAMBDA -> Rc<Widget> {
                                 return index < list.size() ? fn(list[index]) : nullptr;
                             }) {}
};

struct Attributes {
    virtual ~Attributes() noexcept {}

    virtual void applyTo(Widget* target) const = 0;
};

struct ArgumentAttributes final : public Attributes {
    ArgumentAttributes(ArgumentsView<Widget> args) noexcept : args(args) {}

    void applyTo(Widget* target) const final {
        args.apply(target);
    }

    ArgumentsView<Widget> args;
};

inline ArgumentAttributes asAttributes(ArgumentsView<Widget> args) {
    return ArgumentAttributes{ args };
}

enum class WidgetState : uint8_t {
    None         = 0,
    Hover        = 1 << 0,
    Pressed      = 1 << 1,
    Focused      = 1 << 2,
    KeyFocused   = 1 << 3,
    Selected     = 1 << 4,
    Disabled     = 1 << 5,
    ForcePressed = 1 << 6,
    Last         = Disabled,
};

template <>
constexpr inline bool isBitFlags<WidgetState> = true;

struct MatchAny {
    template <std::derived_from<Widget> WidgetClass>
    constexpr bool operator()(const std::shared_ptr<WidgetClass>&) const noexcept {
        return true;
    }
};

struct MatchNth {
    const int requiredIndex;

    constexpr MatchNth(int requiredIndex) : requiredIndex(requiredIndex) {}

    mutable int index = 0;

    template <std::derived_from<Widget> WidgetClass>
    constexpr bool operator()(const std::shared_ptr<WidgetClass>&) const noexcept {
        return index++ == requiredIndex;
    }
};

struct MatchVisible {
    template <std::derived_from<Widget> WidgetClass>
    constexpr bool operator()(const std::shared_ptr<WidgetClass>& w) const noexcept {
        return w->isVisible();
    }
};

struct MatchId {
    const std::string_view id;

    template <std::derived_from<Widget> WidgetClass>
    constexpr bool operator()(const std::shared_ptr<WidgetClass>& w) const noexcept {
        return w->id.get() == id;
    }
};

struct MatchRole {
    const std::string_view role;

    template <std::derived_from<Widget> WidgetClass>
    constexpr bool operator()(const std::shared_ptr<WidgetClass>& w) const noexcept {
        return w->role.get() == role;
    }
};

struct MatchNone {
    template <std::derived_from<Widget> WidgetClass>
    constexpr bool operator()(const std::shared_ptr<WidgetClass>&) const noexcept {
        return false;
    }
};

struct EventDelegate {
    virtual void delegatedEvent(Widget* target, Event& event) = 0;
};

struct Construction {
    Construction() = delete;

    explicit Construction(std::string_view type) noexcept : type(type) {}

    std::string_view type;
};

#define WIDGET

struct WidgetActions {
    function<void(Widget*)> onParentSet;
};

namespace Internal {

struct Resolve {
    constexpr Resolve(Length value, float resolved = {}) noexcept : value(value), resolved(resolved) {}

    constexpr bool operator==(const Resolve& other) const noexcept {
        return value == other.value;
    }

    Length value;
    float resolved;
};

struct WidgetProps {};

class LayoutEngine;

struct WidgetArgumentAccept {
    void operator()(std::shared_ptr<Widget>);
    void operator()(Builder);
    void operator()(const Attributes&);
    void operator()(const Rules&);
    void operator()(WidgetGroup*);
    void operator()(WidgetActions);

    template <typename T, typename U, ArgumentOp op>
    void operator()(ArgVal<T, U, op>);
};

} // namespace Internal

template <typename T>
concept WidgetArgument = std::invocable<Internal::WidgetArgumentAccept, std::remove_cvref_t<T>>;

template <std::derived_from<Widget> T, WidgetArgument Arg>
BRISK_INLINE void applier(T* self, const Arg& arg)
    requires requires { self->apply(arg); }
{
    self->apply(arg);
}

template <typename Callable, typename R, typename... Args>
concept invocable_r = std::is_invocable_r_v<R, Callable, Args...>;

namespace Internal {
constexpr inline size_t numProperties = 102;

template <typename T, int subfield = -1>
struct PropFieldType {
    using Type = FieldType<T, subfield>;
};

template <typename T>
struct PropFieldType<T, -1> {
    using Type = T;
};

template <typename T, bool Transition, bool Resolvable>
struct PropFieldStorageType {
    using Type = T;
};

template <typename T>
struct PropFieldStorageType<T, true, false> {
    using Type = Internal::Transition<T>;
};

template <typename T>
struct PropFieldStorageType<T, false, true> {
    using Type = Internal::Resolve;
};

} // namespace Internal

template <typename T, int subfield = -1>
using PropFieldType = typename Internal::PropFieldType<T, subfield>::Type;

template <typename T, PropFlags flags>
using PropFieldStorageType = typename Internal::PropFieldStorageType<T, flags && PropFlags::Transition,
                                                                     flags && PropFlags::Resolvable>::Type;

namespace Internal {
template <typename Type>
using Fn0Type = Type (*)();
template <typename Type>
using Fn1Type = Type (*)(Widget*);

template <std::derived_from<Object> U>
void fixClone(U* ptr) noexcept {
    if constexpr (requires { typename U::Base; }) {
        fixClone(static_cast<typename U::Base*>(ptr));
    }
    ptr->propInit = ptr;
}

#define BRISK_CLONE_IMPLEMENTATION                                                                           \
    auto result = rcnew std::remove_cvref_t<decltype(*this)>(*this);                                         \
    Internal::fixClone(result.get());                                                                        \
    return result;

} // namespace Internal

namespace Tag {

template <typename Class, typename T, PropertyIndex index>
struct PropArg : PropertyTag {
    using Type = T;

    static std::string_view name() noexcept {
        return Property<Class, T, index>::traits().name;
    }
};

template <typename PropertyType>
struct PropArgT;

template <typename Class, typename T, PropertyIndex index>
struct PropArgT<Property<Class, T, index>> {
    using Type = PropArg<Class, T, index>;
};

} // namespace Tag

template <typename PropertyType>
using PropArgument = Argument<typename Tag::PropArgT<PropertyType>::Type>;

template <typename Class, typename T, PropertyIndex index, typename... Args>
inline void applier(std::type_identity_t<Class>* target,
                    const ArgVal<Tag::PropArg<Class, T, index>, BindableCallback<Args...>>& value) {
    BRISK_ASSERT(target);
    BRISK_ASSUME(target);
    Property<Class, T, index> prop{ target };
    prop.listen(value.value.callback, value.value.address, BindType::Default);
}

template <typename Class, typename T, PropertyIndex index, typename U>
inline void applier(std::type_identity_t<Class>* target,
                    const ArgVal<Tag::PropArg<Class, T, index>, U>& value)
    requires(!Internal::isTrigger<T>)
{
    Property<Class, T, index> prop{ target };
    prop.set(value.value);
}

using StyleVarType = std::variant<std::monostate, ColorW, EdgesL, float, int>;

namespace Internal {

template <typename WidgetClass, typename ValueType>
struct PropGui {
    ValueType(WidgetClass::* field);
    PropFlags flags;
    const char* name = nullptr;

    void set(WidgetClass* self, ValueType value) const {
        self->propSet(
            { this },
            [&]() -> bool {
                if (self->*field == value) {
                    return false;
                }
                self->*field = std::move(value);
                return true;
            },
            flags, toBindingAddress(&(self->*field)));
    }

    void set(WidgetClass* self, Inherit) const {
        self->propInherit({ this }, flags);
    }

    ValueOrConstRef<ValueType> get(const WidgetClass* self) const noexcept {
        return (self->*field);
    }

    ValueOrConstRef<ValueType> current(const WidgetClass* self) const noexcept {
        return (self->*field);
    }

    BindingAddress address(const WidgetClass* self) const noexcept {
        return toBindingAddress(&(self->*field));
    }
};

template <typename WidgetClass, typename ValueType>
PropGui(ValueType(WidgetClass::*), PropFlags, const char* = nullptr) -> PropGui<WidgetClass, ValueType>;

template <typename WidgetClass, typename ValueType>
struct PropGui<WidgetClass, Transition<ValueType>> {
    Transition<ValueType>(WidgetClass::* field);
    float(WidgetClass::* duration);
    PropFlags flags;
    const char* name = nullptr;

    void set(WidgetClass* self, ValueType value) const {
        self->propSet(
            { this },
            [&]() -> bool {
                if (!(self->*field).set(value, self->transitionAllowed() ? self->*duration : 0.f)) {
                    return false;
                }
                if ((self->*field).isActive()) {
                    self->requestAnimationFrame();
                }
                return true;
            },
            flags, toBindingAddress(&(self->*field)));
    }

    void set(WidgetClass* self, Inherit value) const {
        self->propInherit({ this }, flags);
    }

    ValueOrConstRef<ValueType> get(const WidgetClass* self) const noexcept {
        return (self->*field).stopValue;
    }

    ValueType current(const WidgetClass* self) const noexcept {
        return (self->*field).current;
    }

    BindingAddress address(const WidgetClass* self) const noexcept {
        return toBindingAddress(&(self->*field));
    }
};

template <typename WidgetClass, typename ValueType>
PropGui(Transition<ValueType>(WidgetClass::*), float(WidgetClass::*), PropFlags, const char* = nullptr)
    -> PropGui<WidgetClass, Transition<ValueType>>;

template <typename WidgetClass>
struct PropGui<WidgetClass, Resolve> {
    Resolve(WidgetClass::* field);
    PropFlags flags;
    const char* name = nullptr;

    void set(WidgetClass* self, Length value) const {
        self->propSet(
            { this },
            [&]() -> bool {
                if ((self->*field).value == value) {
                    return false;
                }
                (self->*field).value = std::move(value);
                return true;
            },
            flags, toBindingAddress(&(self->*field)));
    }

    void set(WidgetClass* self, Inherit) const {
        self->propInherit({ this }, flags);
    }

    Length get(const WidgetClass* self) const noexcept {
        return (self->*field).value;
    }

    float current(const WidgetClass* self) const noexcept {
        return (self->*field).resolved;
    }

    BindingAddress address(const WidgetClass* self) const noexcept {
        return toBindingAddress(&(self->*field));
    }
};

template <typename WidgetClass, template <typename T> typename TypeTemplate, typename SubType,
          typename CurrentSubType, PropertyIndex index0, PropertyIndex... indices>
struct PropGuiCompound {
    const char* name = nullptr;

    template <PropertyIndex Index>
    constexpr static decltype(auto) traits() noexcept {
        return Property<WidgetClass, SubType, Index>::traits(); // get<Index>(WidgetClass::properties());
    }

    // using SubType          = decltype(def0().get(nullptr));
    // using CurrentSubType   = decltype(def0().current(nullptr));
    using ValueType        = TypeTemplate<SubType>;
    using CurrentValueType = TypeTemplate<CurrentSubType>;

    static_assert(std::is_trivially_copyable_v<ValueType>);
    static_assert(std::is_trivially_copyable_v<CurrentValueType>);

    ValueType get(const WidgetClass* self) const noexcept {
        return ValueType{
            traits<index0>().get(self),
            traits<indices>().get(self)...,
        };
    }

    void set(WidgetClass* self, ValueType value) const {
        traits<index0>().set(self, value[0]);
        size_t i = 0;
        ((traits<indices>().set(self, value[++i])), ...);
    }

    void set(WidgetClass* self, Inherit) const {
        traits<index0>().set(self, Inherit{});
        ((traits<indices>().set(self, Inherit{})), ...);
    }

    CurrentValueType current(const WidgetClass* self) const noexcept {
        return CurrentValueType{
            traits<index0>().current(self),
            traits<indices>().current(self)...,
        };
    }

    BindingAddress address(const WidgetClass* self) const noexcept {
        return mergeAddresses(traits<index0>().address(self), traits<indices>().address(self)...);
    }
};
} // namespace Internal

class WIDGET Widget : public BindableObject<Widget, &uiScheduler> {
    BRISK_DYNAMIC_CLASS_ROOT(Widget)
public:
    using Ptr                 = std::shared_ptr<Widget>;
    using WidgetPtrs          = std::vector<Ptr>;
    using WidgetIterator      = typename WidgetPtrs::iterator;
    using WidgetConstIterator = typename WidgetPtrs::const_iterator;

    static Rc<Scheduler> dispatcher() {
        return uiScheduler;
    }

    Widget& operator=(const Widget&) = delete;
    Widget& operator=(Widget&&)      = delete;

    Ptr clone() const;

    constexpr static std::string_view widgetType = "widget";

    template <WidgetArgument... Args>
    explicit Widget(const Args&... args) : Widget{ Construction{ widgetType }, std::tuple{ args... } } {
        endConstruction();
    }

    explicit Widget(Construction construction, ArgumentsView<Widget> args);

    virtual ~Widget() noexcept;

    template <typename Tag, std::convertible_to<typename Tag::Type> Ty>
    void set(const ArgVal<Tag, Ty>& arg) {
        apply(arg);
    }

    virtual std::optional<std::string> textContent() const;

    ////////////////////////////////////////////////////////////////////////////////
    // Debug
    ////////////////////////////////////////////////////////////////////////////////

    std::string name() const;

    virtual void dump(int depth = 0) const;

    void updateState(WidgetState& state, const Event& event, Rectangle rect);

    ////////////////////////////////////////////////////////////////////////////////
    // Builders
    ////////////////////////////////////////////////////////////////////////////////

    void apply(Builder builder);

    void apply(const WidgetActions& action);

    void doRebuild();
    virtual void rebuild(bool force);

    template <typename T>
    void apply(ArgVal<Tag::Depends, Value<T>> value) {
        bindings->connect(trigRebuild(), std::move(value.value), BindType::Deferred, false);
    }

    Value<Trigger<>> trigRebuild();

    struct BuilderData {
        Builder builder;
        uint32_t position;
        uint32_t count;
    };

    ////////////////////////////////////////////////////////////////////////////////
    // Iterators & traversal
    ////////////////////////////////////////////////////////////////////////////////

    struct Iterator {
        const Widget* w;
        size_t i;

        void operator++();

        const Rc<Widget>& operator*() const;

        bool operator!=(std::nullptr_t) const;
    };

    struct IteratorEx {
        const Widget* w;
        size_t i;
        bool reverse;

        void operator++();

        const Rc<Widget>& operator*() const;

        bool operator!=(std::nullptr_t) const;
    };

    Iterator begin() const;
    std::nullptr_t end() const;

    IteratorEx rbegin() const;
    std::nullptr_t rend() const;

    IteratorEx begin(bool reverse) const;

    void bubble(function_ref<bool(Widget*)> fn, bool includePopup = false);

    bool hasParent(Widget* parent, bool includePopup = false) const;

    template <std::derived_from<Widget> Type = Widget, typename Fn>
    void enumerate(Fn&& fn, bool recursive = false, bool recursiveForMatching = true) {
        for (const Ptr& w : *this) {
            if (Type* t = dynamicCast<Type*>(w.get())) {
                fn(t);
                if (recursive && recursiveForMatching)
                    w->enumerate<Type>(fn, recursive, recursiveForMatching);
            } else {
                if (recursive)
                    w->enumerate<Type>(fn, recursive, recursiveForMatching);
            }
        }
    }

    template <std::derived_from<Widget> Type>
    std::shared_ptr<Type> findSibling(Order order, bool wrap = false) {
        BRISK_ASSERT(m_parent);
        bool foundThis = false;
        std::shared_ptr<Type> firstMatch;

        for (auto it = m_parent->begin(order == Order::Previous); it != m_parent->end(); ++it) {
            std::shared_ptr<Type> typed = dynamicPointerCast<Type>(*it);
            if (typed && !firstMatch) {
                firstMatch = typed;
            }
            if ((*it).get() == this) {
                foundThis = true;
            } else if (typed && foundThis) {
                return typed;
            }
        }
        if (wrap)
            return firstMatch;
        return nullptr;
    }

    template <typename Open, typename Close>
    void traverse(Open&& open, Close&& close) {
        struct State {
            Ptr widget;
            size_t index;
        };

        std::stack<State, SmallVector<State, 32>> stack;

        State current;
        current.widget = shared_from_this();
        current.index  = 0;
        bool process   = open(current.widget);
        if (!process)
            return;

        for (;;) {
            if (current.index >= current.widget->m_widgets.size()) {
                close(current.widget);
                if (stack.empty()) {
                    return;
                }
                current = std::move(stack.top());
                stack.pop();
                ++current.index;
            } else {
                State newCurrent;
                newCurrent.widget = current.widget->m_widgets[current.index];
                newCurrent.index  = 0;
                bool process      = open(newCurrent.widget);
                if (process && !newCurrent.widget->m_widgets.empty()) {
                    stack.push(std::move(current));
                    current = std::move(newCurrent);
                } else {
                    if (process) {
                        close(newCurrent.widget);
                    }
                    ++current.index;
                }
            }
        }
    }

    template <typename WidgetClass = Widget, typename Matcher>
    std::shared_ptr<WidgetClass> find(Matcher&& matcher) const
        requires std::is_invocable_r_v<bool, Matcher, std::shared_ptr<WidgetClass>>
    {
        for (const Ptr& w : *this) {
            std::shared_ptr<WidgetClass> ww = dynamicPointerCast<WidgetClass>(w);
            if (ww && matcher(ww))
                return ww;
        }
        return nullptr;
    }

    template <typename WidgetClass = Widget, typename Matcher, typename ParentMatcher>
    std::shared_ptr<WidgetClass> find(Matcher&& matcher, ParentMatcher&& parentMatcher) const
        requires std::is_invocable_r_v<bool, Matcher, std::shared_ptr<WidgetClass>> &&
                 std::is_invocable_r_v<bool, Matcher, std::shared_ptr<Widget>>
    {
        for (const Ptr& w : *this) {
            std::shared_ptr<WidgetClass> ww = dynamicPointerCast<WidgetClass>(w);
            if (ww && matcher(ww))
                return ww;
            if (parentMatcher(w)) {
                ww = w->find<WidgetClass>(std::forward<Matcher>(matcher),
                                          std::forward<ParentMatcher>(parentMatcher));
                if (ww)
                    return ww;
            }
        }
        return nullptr;
    }

    template <typename WidgetClass = Widget>
    std::shared_ptr<WidgetClass> find() const {
        return find<WidgetClass>(MatchAny{}, MatchAny{});
    }

    template <typename WidgetClass = Widget>
    std::shared_ptr<WidgetClass> findById(std::string_view id) const {
        return find<WidgetClass>(MatchId{ id }, MatchAny{});
    }

    std::optional<WidgetIterator> findIterator(Widget* widget, Widget** parent = nullptr);

    ////////////////////////////////////////////////////////////////////////////////
    // Geometry
    ////////////////////////////////////////////////////////////////////////////////

    bool isVisible() const noexcept;

    Rectangle rect() const noexcept;

    Rectangle clientRect() const noexcept;

    Rectangle subtreeRect() const noexcept;

    Rectangle clipRect() const noexcept;

    Rectangle hintRect() const noexcept;

    Rectangle adjustedRect() const noexcept;

    Rectangle adjustedHintRect() const noexcept;

    ////////////////////////////////////////////////////////////////////////////////
    // Style & layout
    ////////////////////////////////////////////////////////////////////////////////

    bool hasClass(std::string_view className) const;
    void addClass(std::string className);
    void removeClass(std::string_view className);
    void toggleClass(std::string_view className);

    const std::string& type() const noexcept;

    Font font() const;

    std::shared_ptr<const Stylesheet> currentStylesheet() const;

    template <StyleVarTag Tag, typename U>
    void apply(const ArgVal<Tag, U>& value) {
        set(Tag{}, static_cast<typename Tag::Type>(value.value));
    }

    template <typename T>
    std::optional<T> getStyleVar(uint64_t id) const;

    template <typename T>
    T getStyleVar(uint64_t id, T fallback) const;

    template <StyleVarTag Tag>
    void set(Tag, typename Tag::Type value) {
        if (assign(m_styleVars[Tag::id], value))
            requestRestyle();
    }

    void apply(const Rules& rules);

    void apply(const Attributes& arg);

    void requestUpdateLayout();
    void requestUpdateVisibility();
    virtual void onLayoutUpdated();
    Size contentSize() const noexcept;
    SizeF computeSize(AvailableSize size);
    bool hadOverflow() const noexcept;
    bool isLayoutDirty() const noexcept;

    EdgesF computedMargin() const noexcept;
    EdgesF computedPadding() const noexcept;
    EdgesF computedBorderWidth() const noexcept;

    ////////////////////////////////////////////////////////////////////////////////
    // Focus and hints
    ////////////////////////////////////////////////////////////////////////////////

    /// Sets focus to this widget
    void focus(bool byKeyboard = false);

    /// Clears focus
    void blur();

    bool hasFocus() const;

    void requestHint() const;

    bool isHintCurrent() const;

    WidgetState state() const noexcept;

    bool isHovered() const noexcept;

    bool isPressed() const noexcept;

    bool isFocused() const noexcept;

    bool isSelected() const noexcept;

    bool isKeyFocused() const noexcept;

    bool isDisabled() const noexcept;

    bool isEnabled() const noexcept;

    ////////////////////////////////////////////////////////////////////////////////
    // Tree & Children
    ////////////////////////////////////////////////////////////////////////////////

    const WidgetPtrs& widgets() const;

    WidgetTree* tree() const noexcept;
    void setTree(WidgetTree* tree);
    Nullable<InputQueue> inputQueue() const noexcept;
    virtual void attachedToTree();
    virtual void detachedFromTree();

    Widget* parent() const noexcept;

    void removeAt(size_t pos);
    void removeIf(function_ref<bool(Widget*)> predicate);
    void remove(Widget* widget);
    void clear();

    virtual void append(Rc<Widget> widget);
    void apply(Rc<Widget> widget);

    virtual void onParentChanged();

    virtual void attached();

    /// @brief Replaces oldWidget with newWidget.
    /// @param deep in subtree
    /// @returns false if oldWidget wasn't found
    bool replace(Ptr oldWidget, Ptr newWidget, bool deep);

    void apply(std::nullptr_t) = delete;

    void apply(WidgetGroup* group);

    virtual void onChildAdded(Widget* w);
    virtual void childrenChanged();

    std::optional<size_t> indexOf(const Widget* widget) const;

    ////////////////////////////////////////////////////////////////////////////////

    void paintTo(Canvas& canvas) const;

    void invalidate();

    Drawable drawable() const;

    std::optional<PointF> mousePos() const;

    void reveal();

    bool setScrollOffset(Point newOffset);
    bool setScrollOffset(Orientation orientation, int newOffset);
    Point scrollOffset() const;
    int scrollOffset(Orientation orientation) const;
    bool hasScrollBar(Orientation orientation) const noexcept;
    int scrollSize(Orientation orientation) const noexcept;
    Size scrollSize() const noexcept;

    bool isMenu() const noexcept;
    void setMenuRoot();

    bool transitionAllowed() const noexcept;

    /**
     * @brief Checks if the specified property is overridden.
     *
     * @param prop The identifier of the property to check.
     * @return true if the property is overridden, false otherwise.
     */
    bool isOverridden(PropertyId prop) const noexcept;

    /**
     * @brief Checks if the specified property is inherited.
     *
     * @param prop The identifier of the property to check.
     * @return true if the property is inherited, false otherwise.
     */
    bool isInherited(PropertyId prop) const noexcept;

    using enum PropFlags;

    friend struct Rules;

protected:
    friend struct InputQueue;
    friend class Stylesheet;
    friend class WidgetTree;

    template <typename WidgetClass, typename ValueType>
    friend struct Internal::PropGui;

    void propInherit(PropertyId prop, PropFlags flags);

    void propSet(PropertyId prop, function_ref<bool()> assign, PropFlags flags, BindingAddress address);

    WidgetTree* m_tree = nullptr;

    Rc<const Stylesheet> m_stylesheet;
    Painter m_painter;

    std::optional<PointF> m_mousePos;

    bool m_inConstruction : 1       = true;
    bool m_constructed : 1          = false;
    bool m_isPopup : 1              = false; // affected by closeNearestPopup
    bool m_processClicks : 1        = true;
    bool m_styleApplying : 1        = false;
    bool m_ignoreChildrenOffset : 1 = false;
    bool m_isMenuRoot : 1           = false; // affected by closeMenuChain

    // functions
    Trigger<> m_onClick;
    Trigger<> m_onDoubleClick;

    function<void(Widget*)> m_reapplyStyle;

    // strings
    std::string m_type;
    std::string m_id;
    std::string m_hint;
    std::string_view m_role;
    Classes m_classes;

    Rectangle m_rect{ 0, 0, 0, 0 };
    Rectangle m_clientRect{ 0, 0, 0, 0 };
    Rectangle m_subtreeRect{ 0, 0, 0, 0 };
    Rectangle m_clipRect{ 0, 0, 0, 0 };
    Rectangle m_hintRect{ 0, 0, 0, 0 };
    EdgesF m_computedMargin{ 0, 0, 0, 0 };
    EdgesF m_computedPadding{ 0, 0, 0, 0 };
    EdgesF m_computedBorderWidth{ 0, 0, 0, 0 };
    Size m_contentSize{ 0, 0 };
    Point m_hintTextOffset{ 0, 0 };
    PreparedText m_hintPrepared;

    Internal::Transition<ColorW> m_backgroundColor{ Palette::transparent };
    Internal::Transition<ColorW> m_borderColor{ Palette::transparent };
    Internal::Transition<ColorW> m_color{ Palette::white };
    Internal::Transition<ColorW> m_shadowColor{ Palette::black.multiplyAlpha(0.66f) };
    Internal::Transition<ColorW> m_scrollBarColor{ Palette::grey };
    float m_backgroundColorTransition      = 0;
    float m_borderColorTransition          = 0;
    float m_colorTransition                = 0;
    float m_shadowColorTransition          = 0;
    float m_scrollBarColorTransition       = 0;
    EasingFunction m_backgroundColorEasing = &easeLinear;
    EasingFunction m_borderColorEasing     = &easeLinear;
    EasingFunction m_colorEasing           = &easeLinear;
    EasingFunction m_shadowColorEasing     = &easeLinear;
    EasingFunction m_scrollBarColorEasing  = &easeLinear;

    // point/size
    PointL m_absolutePosition{ undef, undef }; // for popup only
    PointL m_anchor{ undef, undef };           // for popup only
    PointL m_translate{ 0, 0 };                // translation relative to own size
    PointF m_shadowOffset{ 0, 0 };

    // pointers
    Widget* m_parent               = nullptr;
    EventDelegate* m_delegate      = nullptr;

    // float
    mutable float m_regenerateTime = 0.0;
    mutable float m_relayoutTime   = 0.0;
    float m_hoverTime              = -1.0;
    OptFloat m_flexGrow            = undef;
    OptFloat m_flexShrink          = undef;
    OptFloat m_aspect              = undef;
    float m_opacity                = 1.f;
    float m_shadowSpread           = 0;

    // int
    Cursor m_cursor                = Cursor::NotSet;
    int m_tabGroupId               = -1;

    Internal::Resolve m_shadowSize{ 0_px };
    Internal::Resolve m_fontSize{ FontSize::Normal, dp(FontSize::Normal) };
    Internal::Resolve m_tabSize{ 40, 40 };
    Internal::Resolve m_letterSpacing{ 0_px, 0.f };
    Internal::Resolve m_wordSpacing{ 0_px, 0.f };
    Internal::Resolve m_scrollBarThickness{ 8_px, 8.f };
    Internal::Resolve m_scrollBarRadius{ 0_px, 0.f };
    Internal::Resolve m_borderRadiusTopLeft{ 0, 0.f };
    Internal::Resolve m_borderRadiusTopRight{ 0, 0.f };
    Internal::Resolve m_borderRadiusBottomLeft{ 0, 0.f };
    Internal::Resolve m_borderRadiusBottomRight{ 0, 0.f };

    CornersL getBorderRadius() const noexcept {
        return { m_borderRadiusTopLeft.value, m_borderRadiusTopRight.value, m_borderRadiusBottomLeft.value,
                 m_borderRadiusBottomRight.value };
    }

    CornersF getBorderRadiusResolved() const noexcept {
        return { m_borderRadiusTopLeft.resolved, m_borderRadiusTopRight.resolved,
                 m_borderRadiusBottomLeft.resolved, m_borderRadiusBottomRight.resolved };
    }

    Length m_borderWidthLeft   = 0;
    Length m_borderWidthTop    = 0;
    Length m_borderWidthRight  = 0;
    Length m_borderWidthBottom = 0;

    EdgesL getBorderWidth() const noexcept {
        return { m_borderWidthLeft, m_borderWidthTop, m_borderWidthRight, m_borderWidthBottom };
    }

    Length m_width  = undef;
    Length m_height = undef;

    SizeL getDimensions() const noexcept {
        return { m_width, m_height };
    }

    Length m_gapColumn = 0;
    Length m_gapRow    = 0;

    SizeL getGap() const noexcept {
        return { m_gapColumn, m_gapRow };
    }

    Length m_marginLeft   = 0;
    Length m_marginTop    = 0;
    Length m_marginRight  = 0;
    Length m_marginBottom = 0;

    EdgesL getMargin() const noexcept {
        return { m_marginLeft, m_marginTop, m_marginRight, m_marginBottom };
    }

    Length m_maxWidth  = undef;
    Length m_maxHeight = undef;

    SizeL getMaxDimensions() const noexcept {
        return { m_maxWidth, m_maxHeight };
    }

    Length m_minWidth  = undef;
    Length m_minHeight = undef;

    SizeL getMinDimensions() const noexcept {
        return { m_minWidth, m_minHeight };
    }

    Length m_paddingLeft   = 0;
    Length m_paddingTop    = 0;
    Length m_paddingRight  = 0;
    Length m_paddingBottom = 0;

    EdgesL getPadding() const noexcept {
        return { m_paddingLeft, m_paddingTop, m_paddingRight, m_paddingBottom };
    }

    // uint8_t
    mutable WidgetState m_state         = WidgetState::None;
    std::string m_fontFamily            = Font::DefaultPlusIconsEmoji;
    FontStyle m_fontStyle               = FontStyle::Normal;
    FontWeight m_fontWeight             = FontWeight::Regular;
    OpenTypeFeatureFlags m_fontFeatures = {};
    TextDecoration m_textDecoration     = TextDecoration::None;
    AlignSelf m_alignSelf               = AlignSelf::Auto;
    Justify m_justifyContent            = Justify::FlexStart;
    Length m_flexBasis                  = auto_;
    AlignItems m_alignItems             = AlignItems::Stretch;
    Layout m_layout                     = Layout::Horizontal;
    LayoutOrder m_layoutOrder           = LayoutOrder::Direct;
    Placement m_placement               = Placement::Normal;
    ZOrder m_zorder                     = ZOrder::Normal;
    WidgetClip m_clip                   = WidgetClip::Normal;
    AlignContent m_alignContent         = AlignContent::FlexStart;
    Wrap m_flexWrap                     = Wrap::NoWrap;
    BoxSizingPerAxis m_boxSizing        = BoxSizingPerAxis::BorderBox;
    AlignToViewport m_alignToViewport   = AlignToViewport::None;
    TextAlign m_textAlign               = TextAlign::Start;
    TextAlign m_textVerticalAlign       = TextAlign::Center;
    MouseInteraction m_mouseInteraction = MouseInteraction::Inherit;

    OverflowScroll m_overflowScrollX    = OverflowScroll::Disable;
    OverflowScroll m_overflowScrollY    = OverflowScroll::Disable;

    OverflowScrollBoth getOverflowScroll() const noexcept {
        return { m_overflowScrollX, m_overflowScrollY };
    }

    ContentOverflow m_contentOverflowX = ContentOverflow::Default;
    ContentOverflow m_contentOverflowY = ContentOverflow::Default;

    ContentOverflowBoth getContentOverflow() const noexcept {
        return { m_contentOverflowX, m_contentOverflowY };
    }

    bool m_tabStop              = false;
    bool m_tabGroup             = false;
    bool m_visible              = true;
    bool m_hidden               = false;
    bool m_autofocus            = false;
    bool m_mousePassThrough     = false;
    bool m_autoMouseCapture     = true;
    bool m_mouseAnywhere        = false;
    bool m_focusCapture         = false;
    bool m_stateTriggersRestyle = false;
    bool m_isHintExclusive      = false;
    bool m_isHintVisible        = false;
    bool m_autoHint             = true;
    bool m_squircleCorners      = false;

    std::array<bool, 2> m_scrollBarDrag{ false, false };
    int m_savedScrollOffset = 0;

    struct ScrollBarGeometry {
        Rectangle track;
        Rectangle thumb;
    };

    Range<int> scrollBarRange(Orientation orientation) const noexcept;
    ScrollBarGeometry scrollBarGeometry(Orientation orientation) const noexcept;

    void updateScrollAxes();

    std::set<PropertyId> m_overriddenProperties;
    std::set<PropertyId> m_inheritedProperties;

    std::map<uint64_t, StyleVarType> m_styleVars;

    enum class RestyleState {
        None,
        NeedRestyleForChildren,
        NeedRestyle,
    };
    RestyleState m_restyleState = RestyleState::NeedRestyle;

    struct StyleApplying {
        explicit StyleApplying(Widget* widget) : widget(widget) {
            widget->m_styleApplying = true;
        }

        ~StyleApplying() {
            widget->m_styleApplying = false;
        }

        Widget* widget;
    };

    Widget(const Widget&);
    void beginConstruction();
    void endConstruction();
    virtual void resetSelection();
    virtual void onConstructed();
    virtual void onFontChanged();

    void enableCustomMeasure() noexcept;

    virtual Ptr cloneThis() const;

    void requestAnimationFrame();
    void animationFrame();
    virtual void onAnimationFrame();

    virtual void revealChild(Widget*);
    void updateGeometry(HitTestMap::State& state);
    bool setChildrenOffset(Point newOffset);
    SizeF measuredDimensions() const noexcept;
    [[nodiscard]] virtual SizeF measure(AvailableSize size) const;

    void parentChanged();

    ///////////////////////////////////////////////////////////////////////////////
    void toggleState(WidgetState mask, bool on);
    void setState(WidgetState newState);
    void requestRestyle();
    void requestStateRestyle();
    ///////////////////////////////////////////////////////////////////////////////

    void doPaint(Canvas& canvas) const;
    void doRefresh();
    virtual void paint(Canvas& canvas) const;
    virtual void postPaint(Canvas& canvas) const;
    virtual void paintScrollBar(Canvas& canvas, Orientation orientation,
                                const ScrollBarGeometry& geometry) const;
    void paintBackground(Canvas& canvas, Rectangle rect) const;
    void paintHint(Canvas& canvas) const;
    void paintFocusFrame(Canvas& canvas) const;
    void paintChildren(Canvas& canvas) const;
    void paintScrollBars(Canvas& canvas) const;
    Rectangle fullPaintRect() const;

    ///////////////////////////////////////////////////////////////////////////////

    Size viewportSize() const noexcept;
    void setRect(Rectangle rect);

    size_t rebuildOne(Builder builder);

    virtual Ptr getContextWidget();

    void insertChild(WidgetConstIterator it, Ptr w);
    void addChild(Ptr w);

    void stateChanged(WidgetState oldState, WidgetState newState);

    virtual void onVisible();
    virtual void onHidden();
    virtual void onRefresh();
    virtual void onStateChanged(WidgetState oldState, WidgetState newState);

    virtual void onEvent(Event& event);
    void processEvent(Event& event);
    void processTemporaryEvent(Event event);
    void bubbleEvent(Event& event, WidgetState enable = WidgetState::None,
                     WidgetState disable = WidgetState::None, bool includePopup = false);

    void requestRebuild();

    /// @brief Closes nearest parent with m_isPopup set to true
    void closeNearestPopup();
    /// @brief Closes nearest parent with m_isMenuRoot set to true
    void closeMenuChain();

    /// @brief
    virtual void close(Widget* sender);

    void resolveProperties(PropFlags flags);
    void restyleIfRequested();
    void requestUpdates(PropFlags flags);

private:
    Point m_childrenOffset{ 0, 0 };

    bool m_rebuildRequested : 1 { false };
    bool m_previouslyVisible : 1 { false };
    bool m_isVisible : 1 { false };
    bool m_embeddable : 1 { false };
    bool m_styleApplied : 1 { false };
    bool m_autofocusReceived : 1 { false };
    bool m_animationRequested : 1 { false };
    bool m_hasLayout : 1 { false };
    bool m_previouslyHasLayout : 1 { false };
    bool m_pendingAnimationRequest : 1 { false };

    Trigger<> m_rebuildTrigger{};

    WidgetPtrs m_widgets;
    std::vector<BuilderData> m_builders;
    std::set<WidgetGroup*> m_groups;
    std::vector<function<void(Widget*)>> m_onParentSet;

    friend struct WidgetGroup;

    explicit Widget(Construction construction);
    void childAdded(Widget* w);
    int32_t applyLayoutRecursively(RectangleF rectangle);
    void computeClipRect();
    void prepareHint();
    void computeHintRect();

    friend class Internal::LayoutEngine;
    ClonablePtr<Internal::LayoutEngine> m_layoutEngine;

    void removeFromGroup(WidgetGroup* group);

    void replaceChild(WidgetIterator it, Ptr newWidget);
    void removeChild(WidgetConstIterator it);
    void childRemoved(Ptr child);

    void processVisibility(bool isVisible);
    void processTreeVisibility(bool isVisible);
    void reposition(Point relativeOffset);
    void refreshTree();

    void doRestyle();
    void doRestyle(std::shared_ptr<const Stylesheet> stylesheet, bool root);

    float resolveFontHeight() const;
    void updateLayout(Rectangle rectangle, bool viewportChanged);
    void markTreeDirty();

    void layoutSet();
    void layoutResetRecursively();

    void setDisabled(bool);
    void setEnabled(bool);
    void setSelected(bool);

    using This                      = Widget;

    constexpr static size_t noIndex = static_cast<size_t>(-1);

public:
    static const tuplet::tuple<
        /* 0 */ Internal::PropField<Widget, Trigger<>>,
        /* 1 */ Internal::PropField<Widget, Trigger<>>,
        /* 2 */ Internal::PropGetterSetter<Widget, WidgetState, bool>,
        /* 3 */ Internal::PropGetterSetter<Widget, WidgetState, bool>,
        /* 4 */ Internal::PropGui<Widget, PointL>,
        /* 5 */ Internal::PropGui<Widget, Align>,
        /* 6 */ Internal::PropGui<Widget, Align>,
        /* 7 */ Internal::PropGui<Widget, Align>,
        /* 8 */ Internal::PropGui<Widget, PointL>,
        /* 9 */ Internal::PropGui<Widget, OptFloat>,
        /* 10 */ Internal::PropGui<Widget, EasingFunction>,
        /* 11 */ Internal::PropGui<Widget, float>,
        /* 12 */ Internal::PropGui<Widget, Internal::Transition<ColorW>>,
        /* 13 */ Internal::PropGui<Widget, EasingFunction>,
        /* 14 */ Internal::PropGui<Widget, float>,
        /* 15 */ Internal::PropGui<Widget, Internal::Transition<ColorW>>,
        /* 16 */ Internal::PropGui<Widget, WidgetClip>,
        /* 17 */ Internal::PropGui<Widget, EasingFunction>,
        /* 18 */ Internal::PropGui<Widget, float>,
        /* 19 */ Internal::PropGui<Widget, Internal::Transition<ColorW>>,
        /* 20 */ Internal::PropGui<Widget, PointF>,
        /* 21 */ Internal::PropGui<Widget, Cursor>,
        /* 22 */ Internal::PropGui<Widget, Length>,
        /* 23 */ Internal::PropGui<Widget, OptFloat>,
        /* 24 */ Internal::PropGui<Widget, OptFloat>,
        /* 25 */ Internal::PropGui<Widget, Wrap>,
        /* 26 */ Internal::PropGui<Widget, std::string>,
        /* 27 */ Internal::PropGui<Widget, Internal::Resolve>,
        /* 28 */ Internal::PropGui<Widget, FontStyle>,
        /* 29 */ Internal::PropGui<Widget, FontWeight>,
        /* 30 */ Internal::PropGui<Widget, bool>,
        /* 31 */ Internal::PropGui<Widget, Justify>,
        /* 32 */ Internal::PropGui<Widget, LayoutOrder>,
        /* 33 */ Internal::PropGui<Widget, Layout>,
        /* 34 */ Internal::PropGui<Widget, Internal::Resolve>,
        /* 35 */ Internal::PropGui<Widget, float>,
        /* 36 */ Internal::PropGui<Widget, Placement>,
        /* 37 */ Internal::PropGui<Widget, Internal::Resolve>,
        /* 38 */ Internal::PropGui<Widget, Internal::Transition<ColorW>>,
        /* 39 */ Internal::PropGui<Widget, float>,
        /* 40 */ Internal::PropGui<Widget, EasingFunction>,
        /* 41 */ Internal::PropGui<Widget, Internal::Resolve>,
        /* 42 */ Internal::PropGui<Widget, TextAlign>,
        /* 43 */ Internal::PropGui<Widget, TextAlign>,
        /* 44 */ Internal::PropGui<Widget, TextDecoration>,
        /* 45 */ Internal::PropGui<Widget, PointL>,
        /* 46 */ Internal::PropGui<Widget, bool>,
        /* 47 */ Internal::PropGui<Widget, Internal::Resolve>,
        /* 48 */ Internal::PropGui<Widget, AlignToViewport>,
        /* 49 */ Internal::PropGui<Widget, BoxSizingPerAxis>,
        /* 50 */ Internal::PropGui<Widget, ZOrder>,
        /* 51 */ Internal::PropGui<Widget, bool>,
        /* 52 */ Internal::PropGui<Widget, std::string>,
        /* 53 */ Internal::PropGui<Widget, std::string_view>,
        /* 54 */ Internal::PropGui<Widget, SmallVector<std::string, 1>>,
        /* 55 */ Internal::PropGui<Widget, MouseInteraction>,
        /* 56 */ Internal::PropGui<Widget, bool>,
        /* 57 */ Internal::PropGui<Widget, bool>,
        /* 58 */ Internal::PropGui<Widget, bool>,
        /* 59 */ Internal::PropGui<Widget, bool>,
        /* 60 */ Internal::PropGui<Widget, bool>,
        /* 61 */ Internal::PropGui<Widget, bool>,
        /* 62 */ Internal::PropGui<Widget, bool>,
        /* 63 */ Internal::PropGui<Widget, bool>,
        /* 64 */ Internal::PropGui<Widget, bool>,
        /* 65 */ Internal::PropGui<Widget, bool>,
        /* 66 */ Internal::PropGui<Widget, EventDelegate*>,
        /* 67 */ Internal::PropGui<Widget, std::string>,
        /* 68 */ Internal::PropGui<Widget, std::shared_ptr<const Stylesheet>>,
        /* 69 */ Internal::PropGui<Widget, Painter>,
        /* 70 */ Internal::PropGui<Widget, bool>,
        /* 71 */ Internal::PropGui<Widget, OpenTypeFeatureFlags>,
        /* 72 */ Internal::PropGui<Widget, Internal::Transition<ColorW>>,
        /* 73 */ Internal::PropGui<Widget, Internal::Resolve>,
        /* 74 */ Internal::PropGui<Widget, Internal::Resolve>,
        /* 75 */ Internal::PropGui<Widget, float>,
        /* 76 */ Internal::PropGui<Widget, Internal::Resolve>,
        /* 77 */ Internal::PropGui<Widget, Internal::Resolve>,
        /* 78 */ Internal::PropGui<Widget, Internal::Resolve>,
        /* 79 */ Internal::PropGui<Widget, Internal::Resolve>,
        /* 80 */ Internal::PropGui<Widget, Length>,
        /* 81 */ Internal::PropGui<Widget, Length>,
        /* 82 */ Internal::PropGui<Widget, Length>,
        /* 83 */ Internal::PropGui<Widget, Length>,
        /* 84 */ Internal::PropGui<Widget, Length>,
        /* 85 */ Internal::PropGui<Widget, Length>,
        /* 86 */ Internal::PropGui<Widget, Length>,
        /* 87 */ Internal::PropGui<Widget, Length>,
        /* 88 */ Internal::PropGui<Widget, Length>,
        /* 89 */ Internal::PropGui<Widget, Length>,
        /* 90 */ Internal::PropGui<Widget, Length>,
        /* 91 */ Internal::PropGui<Widget, Length>,
        /* 92 */ Internal::PropGui<Widget, Length>,
        /* 93 */ Internal::PropGui<Widget, Length>,
        /* 94 */ Internal::PropGui<Widget, Length>,
        /* 95 */ Internal::PropGui<Widget, Length>,
        /* 96 */ Internal::PropGui<Widget, Length>,
        /* 97 */ Internal::PropGui<Widget, Length>,
        /* 98 */ Internal::PropGui<Widget, Length>,
        /* 99 */ Internal::PropGui<Widget, Length>,
        /* 100 */ Internal::PropGui<Widget, OverflowScroll>,
        /* 101 */ Internal::PropGui<Widget, OverflowScroll>,
        /* 102 */ Internal::PropGui<Widget, ContentOverflow>,
        /* 103 */ Internal::PropGui<Widget, ContentOverflow>,
        /* 104 */ Internal::PropGui<Widget, float>,
        /* 105 */ Internal::PropGui<Widget, EasingFunction>,
        /* 106 */ Internal::PropGuiCompound<Widget, CornersOf, Length, float, 76, 77, 78, 79>,
        /* 107 */ Internal::PropGuiCompound<Widget, EdgesOf, Length, Length, 80, 81, 82, 83>,
        /* 108 */ Internal::PropGuiCompound<Widget, SizeOf, Length, Length, 84, 85>,
        /* 109 */ Internal::PropGuiCompound<Widget, SizeOf, Length, Length, 86, 87>,
        /* 110 */ Internal::PropGuiCompound<Widget, EdgesOf, Length, Length, 88, 89, 90, 91>,
        /* 111 */ Internal::PropGuiCompound<Widget, SizeOf, Length, Length, 92, 93>,
        /* 112 */ Internal::PropGuiCompound<Widget, SizeOf, Length, Length, 94, 95>,
        /* 113 */ Internal::PropGuiCompound<Widget, EdgesOf, Length, Length, 96, 97, 98, 99>,
        /* 114 */ Internal::PropGuiCompound<Widget, SizeOf, OverflowScroll, OverflowScroll, 100, 101>,
        /* 115 */ Internal::PropGuiCompound<Widget, SizeOf, ContentOverflow, ContentOverflow, 102, 103>>&
    properties() noexcept;

public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PROPERTIES
    ////////////////////////////////////////////////////////////////////////////////////////////////////

    BRISK_PROPERTIES_BEGIN
    Property<Widget, Trigger<>, 0> onClick;
    Property<Widget, Trigger<>, 1> onDoubleClick;
    Property<Widget, bool, 2> enabled;
    Property<Widget, bool, 3> selected;
    Property<Widget, PointL, 4> absolutePosition;
    Property<Widget, AlignContent, 5> alignContent;
    Property<Widget, AlignItems, 6> alignItems;
    Property<Widget, AlignSelf, 7> alignSelf;
    Property<Widget, PointL, 8> anchor;
    Property<Widget, OptFloat, 9> aspect;
    Property<Widget, EasingFunction, 10> backgroundColorEasing;
    Property<Widget, float, 11> backgroundColorTransition;
    Property<Widget, ColorW, 12> backgroundColor;
    Property<Widget, EasingFunction, 13> borderColorEasing;
    Property<Widget, float, 14> borderColorTransition;
    Property<Widget, ColorW, 15> borderColor;
    Property<Widget, WidgetClip, 16> clip;
    Property<Widget, EasingFunction, 17> colorEasing;
    Property<Widget, float, 18> colorTransition;
    Property<Widget, ColorW, 19> color;
    Property<Widget, PointF, 20> shadowOffset;
    Property<Widget, Cursor, 21> cursor;
    Property<Widget, Length, 22> flexBasis;
    Property<Widget, OptFloat, 23> flexGrow;
    Property<Widget, OptFloat, 24> flexShrink;
    Property<Widget, Wrap, 25> flexWrap;
    Property<Widget, std::string, 26> fontFamily;
    Property<Widget, Length, 27> fontSize;
    Property<Widget, FontStyle, 28> fontStyle;
    Property<Widget, FontWeight, 29> fontWeight;
    Property<Widget, bool, 30> hidden;
    Property<Widget, Justify, 31> justifyContent;
    Property<Widget, LayoutOrder, 32> layoutOrder;
    Property<Widget, Layout, 33> layout;
    Property<Widget, Length, 34> letterSpacing;
    Property<Widget, float, 35> opacity;
    Property<Widget, Placement, 36> placement;
    Property<Widget, Length, 37> shadowSize;
    Property<Widget, ColorW, 38> shadowColor;
    Property<Widget, float, 39> shadowColorTransition;
    Property<Widget, EasingFunction, 40> shadowColorEasing;
    Property<Widget, Length, 41> tabSize;
    Property<Widget, TextAlign, 42> textAlign;
    Property<Widget, TextAlign, 43> textVerticalAlign;
    Property<Widget, TextDecoration, 44> textDecoration;
    Property<Widget, PointL, 45> translate;
    Property<Widget, bool, 46> visible;
    Property<Widget, Length, 47> wordSpacing;
    Property<Widget, AlignToViewport, 48> alignToViewport;
    Property<Widget, BoxSizingPerAxis, 49> boxSizing;
    Property<Widget, ZOrder, 50> zorder;
    Property<Widget, bool, 51> stateTriggersRestyle;
    Property<Widget, std::string, 52> id;
    Property<Widget, std::string_view, 53> role;
    Property<Widget, Classes, 54> classes;
    Property<Widget, MouseInteraction, 55> mouseInteraction;
    Property<Widget, bool, 56> mousePassThrough;
    Property<Widget, bool, 57> autoMouseCapture;
    Property<Widget, bool, 58> mouseAnywhere;
    Property<Widget, bool, 59> focusCapture;
    Property<Widget, bool, 60> isHintVisible;
    Property<Widget, bool, 61> tabStop;
    Property<Widget, bool, 62> tabGroup;
    Property<Widget, bool, 63> autofocus;
    Property<Widget, bool, 64> autoHint;
    Property<Widget, bool, 65> squircleCorners;
    Property<Widget, EventDelegate*, 66> delegate;
    Property<Widget, std::string, 67> hint;
    Property<Widget, std::shared_ptr<const Stylesheet>, 68> stylesheet;
    Property<Widget, Painter, 69> painter;
    Property<Widget, bool, 70> isHintExclusive;
    Property<Widget, OpenTypeFeatureFlags, 71> fontFeatures;
    Property<Widget, ColorW, 72> scrollBarColor;
    Property<Widget, Length, 73> scrollBarThickness;
    Property<Widget, Length, 74> scrollBarRadius;
    Property<Widget, float, 75> shadowSpread;
    Property<Widget, Length, 76> borderRadiusTopLeft;
    Property<Widget, Length, 77> borderRadiusTopRight;
    Property<Widget, Length, 78> borderRadiusBottomLeft;
    Property<Widget, Length, 79> borderRadiusBottomRight;
    Property<Widget, Length, 80> borderWidthLeft;
    Property<Widget, Length, 81> borderWidthTop;
    Property<Widget, Length, 82> borderWidthRight;
    Property<Widget, Length, 83> borderWidthBottom;
    Property<Widget, Length, 84> width;
    Property<Widget, Length, 85> height;
    Property<Widget, Length, 86> gapColumn;
    Property<Widget, Length, 87> gapRow;
    Property<Widget, Length, 88> marginLeft;
    Property<Widget, Length, 89> marginTop;
    Property<Widget, Length, 90> marginRight;
    Property<Widget, Length, 91> marginBottom;
    Property<Widget, Length, 92> maxWidth;
    Property<Widget, Length, 93> maxHeight;
    Property<Widget, Length, 94> minWidth;
    Property<Widget, Length, 95> minHeight;
    Property<Widget, Length, 96> paddingLeft;
    Property<Widget, Length, 97> paddingTop;
    Property<Widget, Length, 98> paddingRight;
    Property<Widget, Length, 99> paddingBottom;
    Property<Widget, OverflowScroll, 100> overflowScrollX;
    Property<Widget, OverflowScroll, 101> overflowScrollY;
    Property<Widget, ContentOverflow, 102> contentOverflowX;
    Property<Widget, ContentOverflow, 103> contentOverflowY;
    Property<Widget, float, 104> scrollBarColorTransition;
    Property<Widget, EasingFunction, 105> scrollBarColorEasing;
    Property<Widget, CornersL, 106> borderRadius;
    Property<Widget, EdgesL, 107> borderWidth;
    Property<Widget, SizeL, 108> dimensions;
    Property<Widget, SizeL, 109> gap;
    Property<Widget, EdgesL, 110> margin;
    Property<Widget, SizeL, 111> maxDimensions;
    Property<Widget, SizeL, 112> minDimensions;
    Property<Widget, EdgesL, 113> padding;
    Property<Widget, OverflowScrollBoth, 114> overflowScroll;
    Property<Widget, ContentOverflowBoth, 115> contentOverflow;

    BRISK_PROPERTIES_END
};

constinit inline size_t widgetSize = sizeof(Widget);

template <std::derived_from<Widget> WidgetType>
inline WidgetActions storeWidget(std::shared_ptr<WidgetType>* ptr) {
    return WidgetActions{
        .onParentSet =
            [ptr](Widget* w) {
                *ptr = dynamicPointerCast<WidgetType>(w->shared_from_this());
            },
    };
}

template <std::derived_from<Widget> WidgetType>
inline WidgetActions storeWidget(std::weak_ptr<WidgetType>* ptr) {
    return WidgetActions{
        [ptr](Widget* w) {
            *ptr = dynamicPointerCast<WidgetType>(w->shared_from_this());
        },
    };
}

inline namespace Arg {

extern const PropArgument<decltype(Widget::absolutePosition)> absolutePosition;
extern const PropArgument<decltype(Widget::alignContent)> alignContent;
extern const PropArgument<decltype(Widget::alignItems)> alignItems;
extern const PropArgument<decltype(Widget::alignSelf)> alignSelf;
extern const PropArgument<decltype(Widget::anchor)> anchor;
extern const PropArgument<decltype(Widget::aspect)> aspect;
extern const PropArgument<decltype(Widget::backgroundColorEasing)> backgroundColorEasing;
extern const PropArgument<decltype(Widget::backgroundColorTransition)> backgroundColorTransition;
extern const PropArgument<decltype(Widget::backgroundColor)> backgroundColor;
extern const PropArgument<decltype(Widget::borderColorEasing)> borderColorEasing;
extern const PropArgument<decltype(Widget::borderColorTransition)> borderColorTransition;
extern const PropArgument<decltype(Widget::borderColor)> borderColor;
extern const PropArgument<decltype(Widget::borderRadius)> borderRadius;
extern const PropArgument<decltype(Widget::borderWidth)> borderWidth;
extern const PropArgument<decltype(Widget::clip)> clip;
extern const PropArgument<decltype(Widget::colorEasing)> colorEasing;
extern const PropArgument<decltype(Widget::colorTransition)> colorTransition;
extern const PropArgument<decltype(Widget::color)> color;
extern const PropArgument<decltype(Widget::cursor)> cursor;
extern const PropArgument<decltype(Widget::dimensions)> dimensions;
extern const PropArgument<decltype(Widget::flexBasis)> flexBasis;
extern const PropArgument<decltype(Widget::flexGrow)> flexGrow;
extern const PropArgument<decltype(Widget::flexShrink)> flexShrink;
extern const PropArgument<decltype(Widget::flexWrap)> flexWrap;
extern const PropArgument<decltype(Widget::fontFamily)> fontFamily;
extern const PropArgument<decltype(Widget::fontSize)> fontSize;
extern const PropArgument<decltype(Widget::fontStyle)> fontStyle;
extern const PropArgument<decltype(Widget::fontWeight)> fontWeight;
extern const PropArgument<decltype(Widget::gap)> gap;
extern const PropArgument<decltype(Widget::hidden)> hidden;
extern const PropArgument<decltype(Widget::justifyContent)> justifyContent;
extern const PropArgument<decltype(Widget::layoutOrder)> layoutOrder;
extern const PropArgument<decltype(Widget::layout)> layout;
extern const PropArgument<decltype(Widget::letterSpacing)> letterSpacing;
extern const PropArgument<decltype(Widget::margin)> margin;
extern const PropArgument<decltype(Widget::maxDimensions)> maxDimensions;
extern const PropArgument<decltype(Widget::minDimensions)> minDimensions;
extern const PropArgument<decltype(Widget::opacity)> opacity;
extern const PropArgument<decltype(Widget::overflowScrollX)> overflowScrollX;
extern const PropArgument<decltype(Widget::overflowScrollY)> overflowScrollY;
extern const PropArgument<decltype(Widget::overflowScroll)> overflowScroll;
extern const PropArgument<decltype(Widget::contentOverflowX)> contentOverflowX;
extern const PropArgument<decltype(Widget::contentOverflowY)> contentOverflowY;
extern const PropArgument<decltype(Widget::contentOverflow)> contentOverflow;
extern const PropArgument<decltype(Widget::padding)> padding;
extern const PropArgument<decltype(Widget::placement)> placement;
extern const PropArgument<decltype(Widget::shadowSize)> shadowSize;
extern const PropArgument<decltype(Widget::shadowOffset)> shadowOffset;
extern const PropArgument<decltype(Widget::shadowColor)> shadowColor;
extern const PropArgument<decltype(Widget::shadowColorTransition)> shadowColorTransition;
extern const PropArgument<decltype(Widget::shadowColorEasing)> shadowColorEasing;
extern const PropArgument<decltype(Widget::tabSize)> tabSize;
extern const PropArgument<decltype(Widget::textAlign)> textAlign;
extern const PropArgument<decltype(Widget::textVerticalAlign)> textVerticalAlign;
extern const PropArgument<decltype(Widget::textDecoration)> textDecoration;
extern const PropArgument<decltype(Widget::translate)> translate;
extern const PropArgument<decltype(Widget::visible)> visible;
extern const PropArgument<decltype(Widget::wordSpacing)> wordSpacing;
extern const PropArgument<decltype(Widget::alignToViewport)> alignToViewport;
extern const PropArgument<decltype(Widget::stateTriggersRestyle)> stateTriggersRestyle;
extern const PropArgument<decltype(Widget::id)> id;
extern const PropArgument<decltype(Widget::role)> role;
extern const PropArgument<decltype(Widget::classes)> classes;
extern const PropArgument<decltype(Widget::mouseInteraction)> mouseInteraction;
extern const PropArgument<decltype(Widget::mousePassThrough)> mousePassThrough;
extern const PropArgument<decltype(Widget::autoMouseCapture)> autoMouseCapture;
extern const PropArgument<decltype(Widget::mouseAnywhere)> mouseAnywhere;
extern const PropArgument<decltype(Widget::focusCapture)> focusCapture;
extern const PropArgument<decltype(Widget::isHintVisible)> isHintVisible;
extern const PropArgument<decltype(Widget::tabStop)> tabStop;
extern const PropArgument<decltype(Widget::tabGroup)> tabGroup;
extern const PropArgument<decltype(Widget::autofocus)> autofocus;
extern const PropArgument<decltype(Widget::autoHint)> autoHint;
extern const PropArgument<decltype(Widget::squircleCorners)> squircleCorners;
extern const PropArgument<decltype(Widget::delegate)> delegate;
extern const PropArgument<decltype(Widget::hint)> hint;
extern const PropArgument<decltype(Widget::zorder)> zorder;
extern const PropArgument<decltype(Widget::stylesheet)> stylesheet;
extern const PropArgument<decltype(Widget::painter)> painter;
extern const PropArgument<decltype(Widget::isHintExclusive)> isHintExclusive;

extern const PropArgument<decltype(Widget::borderRadiusTopLeft)> borderRadiusTopLeft;
extern const PropArgument<decltype(Widget::borderRadiusTopRight)> borderRadiusTopRight;
extern const PropArgument<decltype(Widget::borderRadiusBottomLeft)> borderRadiusBottomLeft;
extern const PropArgument<decltype(Widget::borderRadiusBottomRight)> borderRadiusBottomRight;

extern const PropArgument<decltype(Widget::borderWidthLeft)> borderWidthLeft;
extern const PropArgument<decltype(Widget::borderWidthTop)> borderWidthTop;
extern const PropArgument<decltype(Widget::borderWidthRight)> borderWidthRight;
extern const PropArgument<decltype(Widget::borderWidthBottom)> borderWidthBottom;

extern const PropArgument<decltype(Widget::marginLeft)> marginLeft;
extern const PropArgument<decltype(Widget::marginTop)> marginTop;
extern const PropArgument<decltype(Widget::marginRight)> marginRight;
extern const PropArgument<decltype(Widget::marginBottom)> marginBottom;

extern const PropArgument<decltype(Widget::paddingLeft)> paddingLeft;
extern const PropArgument<decltype(Widget::paddingTop)> paddingTop;
extern const PropArgument<decltype(Widget::paddingRight)> paddingRight;
extern const PropArgument<decltype(Widget::paddingBottom)> paddingBottom;

extern const PropArgument<decltype(Widget::width)> width;
extern const PropArgument<decltype(Widget::height)> height;
extern const PropArgument<decltype(Widget::maxWidth)> maxWidth;
extern const PropArgument<decltype(Widget::maxHeight)> maxHeight;
extern const PropArgument<decltype(Widget::minWidth)> minWidth;
extern const PropArgument<decltype(Widget::minHeight)> minHeight;

extern const PropArgument<decltype(Widget::gapColumn)> gapColumn;
extern const PropArgument<decltype(Widget::gapRow)> gapRow;

extern const PropArgument<decltype(Widget::fontFeatures)> fontFeatures;

extern const PropArgument<decltype(Widget::scrollBarColor)> scrollBarColor;
extern const PropArgument<decltype(Widget::scrollBarThickness)> scrollBarThickness;
extern const PropArgument<decltype(Widget::scrollBarRadius)> scrollBarRadius;
extern const PropArgument<decltype(Widget::shadowSpread)> shadowSpread;

constexpr inline PropArgument<decltype(Widget::onClick)> onClick{};
constexpr inline PropArgument<decltype(Widget::onDoubleClick)> onDoubleClick{};

constexpr inline PropArgument<decltype(Widget::enabled)> enabled{};
constexpr inline PropArgument<decltype(Widget::selected)> selected{};

} // namespace Arg

namespace Tag {
template <std::derived_from<Widget> WidgetType, Internal::FixedString Name>
struct WithRole {
    using Type = Rc<WidgetType>;
};
} // namespace Tag

template <std::derived_from<Widget> WidgetType, Internal::FixedString Name>
struct Argument<Tag::WithRole<WidgetType, Name>> {
    using ValueType = typename Tag::WithRole<WidgetType, Name>::Type;

    constexpr ArgVal<Tag::WithRole<WidgetType, Name>> operator=(WidgetType* value) const {
        value->role.set(Name.string());
        return { Rc<WidgetType>(value) };
    }

    ArgVal<Tag::WithRole<WidgetType, Name>> operator=(Rc<WidgetType> value) const {
        value->role.set(Name.string());
        return { std::move(value) };
    }

    static WidgetType* matchesType(Widget* widget) {
        if (WidgetType* typed = dynamicCast<WidgetType*>(widget)) {
            typed->role.set(Name.string());
            return typed;
        }
        return nullptr;
    }

    static WidgetType* matchesRole(Widget* widget) {
        if (WidgetType* typed = dynamicCast<WidgetType*>(widget);
            typed && typed->role.get() == Name.string()) {
            return typed;
        }
        return nullptr;
    }

    static Rc<WidgetType> get(const Widget* parent) {
        return parent->template find<WidgetType>(MatchRole{ Name.string() });
    }

    static std::string_view role() {
        return Name.string();
    }

    static void create(Widget* parent) {
        static_assert(std::constructible_from<WidgetType>,
                      "WidgetType must be constructible from empty argument list");
        if (!get(parent)) {
            parent->apply(rcnew WidgetType{ Arg::role = Name.string() });
        }
    }
};

template <std::derived_from<Widget> WidgetType, Internal::FixedString Name>
using WidgetRole = Argument<Tag::WithRole<WidgetType, Name>>;

template <std::derived_from<Widget> WidgetType, Internal::FixedString Name>
void applier(Widget* target, ArgVal<Tag::WithRole<WidgetType, Name>> value) {
    target->apply(std::move(value.value));
}

int shufflePalette(int x);

} // namespace Brisk

template <>
struct fmt::formatter<Brisk::WidgetState> : fmt::formatter<std::string> {
    template <typename FormatContext>
    auto format(Brisk::WidgetState val, FormatContext& ctx) const {
        std::vector<std::string_view> list;
        using enum Brisk::WidgetState;
        if (val && Hover)
            list.push_back("Hover");
        if (val && Selected)
            list.push_back("Selected");
        if (val && Pressed)
            list.push_back("Pressed");
        if (val && Focused)
            list.push_back("Focused");
        if (val && KeyFocused)
            list.push_back("KeyFocused");
        if (val && Disabled)
            list.push_back("Disabled");
        return fmt::format_to(ctx.out(), "{}", fmt::join(list, " | "));
    }
};

BRISK_CLANG_PRAGMA(clang diagnostic pop)
