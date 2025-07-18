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

#include <brisk/core/Brisk.h>

BRISK_CLANG_PRAGMA(clang diagnostic push)
BRISK_CLANG_PRAGMA(clang diagnostic ignored "-Wc++2a-extensions")

#include <set>
#include <fmt/ranges.h>
#include <spdlog/spdlog.h>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>
#include <type_traits>
#include <utility>
#include <brisk/core/Binding.hpp>
#include <brisk/core/BasicTypes.hpp>
#include <brisk/core/Utilities.hpp>
#include <brisk/core/MetaClass.hpp>
#include <brisk/core/internal/cityhash.hpp>
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
    using PaintFunc              = function<void(Canvas&, const Widget&)>;

    constexpr Painter() noexcept = default;
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

using Resolve = Animated<Length, float>;

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

} // namespace Internal

namespace Internal {

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

    template <typename U>
    static consteval bool accepts() noexcept {
        return Property<Class, T, index>::template accepts<U>();
    }

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

namespace Internal {
void invalidPropertyApplication(Widget* target, std::string_view name);
} // namespace Internal

template <std::derived_from<Widget> Class, typename T, PropertyIndex index, typename... Args>
inline void applier(Widget* target,
                    const ArgVal<Tag::PropArg<Class, T, index>, BindableCallback<Args...>>& value) {
    BRISK_ASSERT(target);
    BRISK_ASSUME(target);
    if (isOf<Class>(target)) {
        Property<Class, T, index> prop{ static_cast<Class*>(target) };
        prop.listen(value.value.callback, value.value.address, BindType::Default);
    } else {
        Internal::invalidPropertyApplication(target, Tag::PropArg<Class, T, index>::name());
    }
}

template <std::derived_from<Widget> Class, typename T, PropertyIndex index, typename U>
inline void applier(Widget* target, const ArgVal<Tag::PropArg<Class, T, index>, U>& value)
    requires(!Internal::isTrigger<T>)
{
    BRISK_ASSERT(target);
    BRISK_ASSUME(target);
    if (isOf<Class>(target)) {
        Property<Class, T, index> prop{ static_cast<Class*>(target) };
        prop.set(value.value);
    } else {
        Internal::invalidPropertyApplication(target, Tag::PropArg<Class, T, index>::name());
    }
}

using StyleVarType = std::variant<std::monostate, ColorW, EdgesL, float, int>;

namespace Internal {

template <typename T>
struct IsConstexprCompatible {
    constexpr static bool value = true;
};

template <typename T, typename Tr, typename A>
struct IsConstexprCompatible<std::basic_string<T, Tr, A>> {
    constexpr static bool value = false;
};

template <typename WidgetClass, typename ValueType>
struct GuiProp {
    ValueType(WidgetClass::* field);
    std::conditional_t<IsConstexprCompatible<ValueType>::value, ValueType, ValueType (*)()> initialValue;
    PropFlags flags;
    const char* name = nullptr;

    PropertyId id() const noexcept {
        return { this };
    }

    ValueType getInitialValue() const {
        if constexpr (IsConstexprCompatible<ValueType>::value) {
            return initialValue;
        } else {
            if (initialValue) {
                return initialValue();
            } else {
                return ValueType{};
            }
        }
    }

    void initialize(WidgetClass* self) const {
        (self->*field) = getInitialValue();
    }

    void reset(WidgetClass* self) const {
        set(self, getInitialValue());
    }

    void set(WidgetClass* self, ValueType value, bool inherit = false, bool override = true) const {
        if (!self->propChanging(this, inherit, override))
            return;
        if (self->*field == value) {
            return;
        }
        bindings->assign(self->*field, std::move(value));
        self->propChanged(this);
    }

    void set(WidgetClass* self, Inherit, bool override = true) const {
        if constexpr (std::is_same_v<WidgetClass, Widget>) {
            if (Widget* parent = self->parent()) {
                set(self, parent->*field, true, override);
            } else {
                set(self, getInitialValue(), true, override);
            }
        }
    }

    void set(WidgetClass* self, Initial) const {
        reset(self);
    }

    template <invocable_r<ValueType, WidgetClass*> Fn>
    void set(WidgetClass* self, Fn&& fn) const {
        set(self, fn(self));
    }

    template <invocable_r<ValueType> Fn>
    void set(WidgetClass* self, Fn&& fn) const {
        set(self, fn());
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
GuiProp(ValueType(WidgetClass::*), std::type_identity_t<ValueType>, PropFlags, const char* = nullptr)
    -> GuiProp<WidgetClass, ValueType>;

template <typename WidgetClass, typename ValueType, typename AnimatedType>
struct GuiProp<WidgetClass, Animated<ValueType, AnimatedType>> {
    Animated<ValueType, AnimatedType>(WidgetClass::* field);
    ValueType initialValue;
    PropFlags flags;
    const char* name = nullptr;

    PropertyId id() const noexcept {
        return { this };
    }

    void initialize(WidgetClass* self) const {
        (self->*field).value   = initialValue;
        AnimatedType resolved  = self->propResolve(this, initialValue);
        (self->*field).current = resolved;
    }

    void reset(WidgetClass* self) const {
        set(self, initialValue);
    }

    void set(WidgetClass* self, ValueType value, bool inherit = false, bool override = true) const {
        if (!self->propChanging(this, inherit, override)) {
            return;
        }
        if ((self->*field).value == value) {
            return;
        }
        PropertyAnimations& anim = self->animations();
        AnimatedType resolved    = self->propResolve(this, value);
        bindings->assign((self->*field).value, value);
        std::function<void()> changed;
        using enum PropFlags;
        if (flags && (AffectLayout | AffectStyle | AffectFont | AffectVisibility | AffectHint)) {
            changed = [self, this]() {
                self->propChanged(this);
            };
        }
        if (self->transitionAllowed() &&
            anim.startTransition((self->*field).current, resolved, id(), std::move(changed))) {
            self->requestAnimationFrame();
        } else {
            (self->*field).current = resolved;
        }
        self->propChanged(this);
    }

    void set(WidgetClass* self, Inherit, bool override = true) const {
        if constexpr (std::is_same_v<WidgetClass, Widget>) {
            if (Widget* parent = self->parent()) {
                set(self, (parent->*field).value, true, override);
            } else {
                set(self, initialValue, true, override);
            }
        }
    }

    void set(WidgetClass* self, Initial) const {
        reset(self);
    }

    template <invocable_r<ValueType, WidgetClass*> Fn>
    void set(WidgetClass* self, Fn&& fn) const {
        set(self, fn(self));
    }

    template <invocable_r<ValueType> Fn>
    void set(WidgetClass* self, Fn&& fn) const {
        set(self, fn());
    }

    ValueOrConstRef<ValueType> get(const WidgetClass* self) const noexcept {
        return (self->*field).value;
    }

    AnimatedType current(const WidgetClass* self) const noexcept {
        return (self->*field).current;
    }

    BindingAddress address(const WidgetClass* self) const noexcept {
        return toBindingAddress(&(self->*field).value);
    }
};

template <typename WidgetClass, typename ValueType, typename AnimatedType>
GuiProp(Animated<ValueType, AnimatedType>(WidgetClass::*), std::type_identity_t<ValueType>, PropFlags,
        const char* = nullptr) -> GuiProp<WidgetClass, Animated<ValueType, AnimatedType>>;

template <typename WidgetClass, template <typename T> typename TypeTemplate, typename SubType,
          typename CurrentSubType, PropertyIndex index0, PropertyIndex... indices>
struct GuiPropCompound {
    const char* name = nullptr;

    template <PropertyIndex Index>
    constexpr static decltype(auto) traits() noexcept {
        return Property<WidgetClass, SubType, Index>::traits();
    }

    using ValueType        = TypeTemplate<SubType>;
    using CurrentValueType = TypeTemplate<CurrentSubType>;

    static_assert(std::is_trivially_copyable_v<ValueType>);
    static_assert(std::is_trivially_copyable_v<CurrentValueType>);

    void reset(WidgetClass* self) const {
        traits<index0>().reset(self);
        (traits<indices>().reset(self), ...);
    }

    ValueType get(const WidgetClass* self) const noexcept {
        return ValueType{
            traits<index0>().get(self),
            traits<indices>().get(self)...,
        };
    }

    void set(WidgetClass* self, ValueType value) const {
        traits<index0>().set(self, value[0]);
        size_t i = 0;
        (traits<indices>().set(self, value[++i]), ...);
    }

    void set(WidgetClass* self, Inherit) const {
        traits<index0>().set(self, Inherit{});
        (traits<indices>().set(self, Inherit{}), ...);
    }

    void set(WidgetClass* self, Initial) const {
        reset(self);
    }

    template <invocable_r<ValueType, WidgetClass*> Fn>
    void set(WidgetClass* self, Fn&& fn) const {
        set(self, fn(self));
    }

    template <invocable_r<ValueType> Fn>
    void set(WidgetClass* self, Fn&& fn) const {
        set(self, fn());
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

    PropertyAnimations& animations() noexcept {
        return m_animations;
    }

    const PropertyAnimations& animations() const noexcept {
        return m_animations;
    }

    using enum PropFlags;

    friend struct Rules;

protected:
    friend struct InputQueue;
    friend class Stylesheet;
    friend class WidgetTree;

    template <typename WidgetClass, typename ValueType>
    friend struct Internal::GuiProp;

    bool propChanging(PropertyId id, bool inherited, bool override);

    template <typename WidgetType, typename ValueType>
    ValueType propResolve(const Internal::GuiProp<WidgetType, Animated<ValueType>>* prop,
                          ValueType value) const {
        return value;
    }

    float propResolve(const Internal::GuiProp<Widget, Internal::Resolve>* prop, Length value) const;

    template <typename Traits>
    bool propChanging(const Traits* prop, bool inherited, bool override) {
        return propChanging(prop->id(), inherited, override);
    }

    template <typename Prop>
    void propChangedPropagate(const Prop*) {}

    template <typename ValueType>
    void propChangedPropagate(const Internal::GuiProp<Widget, ValueType>* prop) {
        const auto val = prop->get(this);
        for (const Rc<Widget>& w : *this) {
            if (w.get()->isInherited(prop->id())) {
                prop->set(w.get(), val, true, false);
            }
        }
    }

    template <typename Traits>
    void propChanged(const Traits* prop) {
        requestUpdates(prop->flags);
        if constexpr (std::is_same_v<Traits, Internal::GuiProp<Widget, Internal::Resolve>>) {
            if (prop->id() == fontSize) {
                resolveProperties();
                for (const Ptr& w : *this) {
                    w->resolveProperty(prop);
                }
            }
        }
        propChangedPropagate(prop);
    }

    WidgetTree* m_tree            = nullptr;

    uint32_t m_invalidatedCounter = 0;

    PropertyAnimations m_animations;

    Rc<const Stylesheet> m_stylesheet;
    Painter m_painter;

    std::optional<PointF> m_mousePos;

    static bool m_styleApplying;

    bool m_inConstruction : 1       = true;
    bool m_constructed : 1          = false;
    bool m_isPopup : 1              = false; // affected by closeNearestPopup
    bool m_processClicks : 1        = true;
    bool m_ignoreChildrenOffset : 1 = false;
    bool m_isMenuRoot : 1           = false; // affected by closeMenuChain
    bool m_destroying : 1           = false;

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

    Animated<ColorW> m_backgroundColor;
    Animated<ColorW> m_borderColor;
    Animated<ColorW> m_color;
    Animated<ColorW> m_shadowColor;
    Animated<ColorW> m_scrollBarColor;

    // point/size
    PointL m_absolutePosition; // for popup only
    PointL m_anchor;           // for popup only
    PointL m_translate;        // translation relative to own size
    Animated<PointF> m_shadowOffset;

    // pointers
    Widget* m_parent = nullptr;
    EventDelegate* m_delegate;

    // float
    mutable float m_regenerateTime = 0.0;
    mutable float m_relayoutTime   = 0.0;
    float m_hoverTime              = -1.0;
    OptFloat m_flexGrow;
    OptFloat m_flexShrink;
    OptFloat m_aspect;
    Animated<float> m_opacity;
    Animated<float> m_shadowSpread;

    // int
    Cursor m_cursor;
    int m_tabGroupId = -1;

    Internal::Resolve m_shadowSize;
    Internal::Resolve m_fontSize;
    Internal::Resolve m_tabSize;
    Internal::Resolve m_letterSpacing;
    Internal::Resolve m_wordSpacing;
    Internal::Resolve m_scrollBarThickness;
    Internal::Resolve m_scrollBarRadius;
    Internal::Resolve m_borderRadiusTopLeft;
    Internal::Resolve m_borderRadiusTopRight;
    Internal::Resolve m_borderRadiusBottomLeft;
    Internal::Resolve m_borderRadiusBottomRight;

    CornersL getBorderRadius() const noexcept {
        return { m_borderRadiusTopLeft.value, m_borderRadiusTopRight.value, m_borderRadiusBottomLeft.value,
                 m_borderRadiusBottomRight.value };
    }

    CornersF getBorderRadiusResolved() const noexcept {
        return { m_borderRadiusTopLeft.current, m_borderRadiusTopRight.current,
                 m_borderRadiusBottomLeft.current, m_borderRadiusBottomRight.current };
    }

    Length m_borderWidthLeft;
    Length m_borderWidthTop;
    Length m_borderWidthRight;
    Length m_borderWidthBottom;

    EdgesL getBorderWidth() const noexcept {
        return { m_borderWidthLeft, m_borderWidthTop, m_borderWidthRight, m_borderWidthBottom };
    }

    Length m_width;
    Length m_height;

    SizeL getDimensions() const noexcept {
        return { m_width, m_height };
    }

    Length m_gapColumn;
    Length m_gapRow;

    SizeL getGap() const noexcept {
        return { m_gapColumn, m_gapRow };
    }

    Length m_marginLeft;
    Length m_marginTop;
    Length m_marginRight;
    Length m_marginBottom;

    EdgesL getMargin() const noexcept {
        return { m_marginLeft, m_marginTop, m_marginRight, m_marginBottom };
    }

    Length m_maxWidth;
    Length m_maxHeight;

    SizeL getMaxDimensions() const noexcept {
        return { m_maxWidth, m_maxHeight };
    }

    Length m_minWidth;
    Length m_minHeight;

    SizeL getMinDimensions() const noexcept {
        return { m_minWidth, m_minHeight };
    }

    Length m_paddingLeft;
    Length m_paddingTop;
    Length m_paddingRight;
    Length m_paddingBottom;

    EdgesL getPadding() const noexcept {
        return { m_paddingLeft, m_paddingTop, m_paddingRight, m_paddingBottom };
    }

    // uint8_t
    mutable WidgetState m_state = WidgetState::None;
    std::string m_fontFamily;
    FontStyle m_fontStyle;
    FontWeight m_fontWeight;
    OpenTypeFeatureFlags m_fontFeatures;
    TextDecoration m_textDecoration;
    AlignSelf m_alignSelf;
    Justify m_justifyContent;
    Length m_flexBasis;
    AlignItems m_alignItems;
    Layout m_layout;
    LayoutOrder m_layoutOrder;
    Placement m_placement;
    ZOrder m_zorder;
    WidgetClip m_clip;
    AlignContent m_alignContent;
    Wrap m_flexWrap;
    BoxSizingPerAxis m_boxSizing;
    AlignToViewport m_alignToViewport;
    TextAlign m_textAlign;
    TextAlign m_textVerticalAlign;
    MouseInteraction m_mouseInteraction;

    OverflowScroll m_overflowScrollX;
    OverflowScroll m_overflowScrollY;

    OverflowScrollBoth getOverflowScroll() const noexcept {
        return { m_overflowScrollX, m_overflowScrollY };
    }

    ContentOverflow m_contentOverflowX;
    ContentOverflow m_contentOverflowY;

    ContentOverflowBoth getContentOverflow() const noexcept {
        return { m_contentOverflowX, m_contentOverflowY };
    }

    bool m_tabStop;
    bool m_tabGroup;
    bool m_visible;
    bool m_hidden;
    bool m_autofocus;
    bool m_mousePassThrough;
    bool m_autoMouseCapture;
    bool m_mouseAnywhere;
    bool m_focusCapture;
    bool m_stateTriggersRestyle;
    bool m_isHintExclusive;
    bool m_isHintVisible;
    bool m_autoHint;
    bool m_squircleCorners;

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
        StyleApplying() {
            std::swap(styleApplyingSaved, m_styleApplying);
        }

        ~StyleApplying() {
            std::swap(styleApplyingSaved, m_styleApplying);
        }

        bool styleApplyingSaved = true;
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

    void resolveAndInherit();
    void resolveProperties();
    void inheritProperties();
    bool resolveProperty(const Internal::GuiProp<Widget, Internal::Resolve>* prop);
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
        /* 4 */ Internal::GuiProp<Widget, PointL>,
        /* 5 */ Internal::GuiProp<Widget, Align>,
        /* 6 */ Internal::GuiProp<Widget, Align>,
        /* 7 */ Internal::GuiProp<Widget, Align>,
        /* 8 */ Internal::GuiProp<Widget, PointL>,
        /* 9 */ Internal::GuiProp<Widget, OptFloat>,
        /* 10 */ int,
        /* 11 */ int,
        /* 12 */ Internal::GuiProp<Widget, Animated<ColorW>>,
        /* 13 */ int,
        /* 14 */ int,
        /* 15 */ Internal::GuiProp<Widget, Animated<ColorW>>,
        /* 16 */ Internal::GuiProp<Widget, WidgetClip>,
        /* 17 */ int,
        /* 18 */ int,
        /* 19 */ Internal::GuiProp<Widget, Animated<ColorW>>,
        /* 20 */ Internal::GuiProp<Widget, Animated<PointF>>,
        /* 21 */ Internal::GuiProp<Widget, Cursor>,
        /* 22 */ Internal::GuiProp<Widget, Length>,
        /* 23 */ Internal::GuiProp<Widget, OptFloat>,
        /* 24 */ Internal::GuiProp<Widget, OptFloat>,
        /* 25 */ Internal::GuiProp<Widget, Wrap>,
        /* 26 */ Internal::GuiProp<Widget, std::string>,
        /* 27 */ Internal::GuiProp<Widget, Internal::Resolve>,
        /* 28 */ Internal::GuiProp<Widget, FontStyle>,
        /* 29 */ Internal::GuiProp<Widget, FontWeight>,
        /* 30 */ Internal::GuiProp<Widget, bool>,
        /* 31 */ Internal::GuiProp<Widget, Justify>,
        /* 32 */ Internal::GuiProp<Widget, LayoutOrder>,
        /* 33 */ Internal::GuiProp<Widget, Layout>,
        /* 34 */ Internal::GuiProp<Widget, Internal::Resolve>,
        /* 35 */ Internal::GuiProp<Widget, Animated<float>>,
        /* 36 */ Internal::GuiProp<Widget, Placement>,
        /* 37 */ Internal::GuiProp<Widget, Internal::Resolve>,
        /* 38 */ Internal::GuiProp<Widget, Animated<ColorW>>,
        /* 39 */ int,
        /* 40 */ int,
        /* 41 */ Internal::GuiProp<Widget, Internal::Resolve>,
        /* 42 */ Internal::GuiProp<Widget, TextAlign>,
        /* 43 */ Internal::GuiProp<Widget, TextAlign>,
        /* 44 */ Internal::GuiProp<Widget, TextDecoration>,
        /* 45 */ Internal::GuiProp<Widget, PointL>,
        /* 46 */ Internal::GuiProp<Widget, bool>,
        /* 47 */ Internal::GuiProp<Widget, Internal::Resolve>,
        /* 48 */ Internal::GuiProp<Widget, AlignToViewport>,
        /* 49 */ Internal::GuiProp<Widget, BoxSizingPerAxis>,
        /* 50 */ Internal::GuiProp<Widget, ZOrder>,
        /* 51 */ Internal::GuiProp<Widget, bool>,
        /* 52 */ Internal::GuiProp<Widget, std::string>,
        /* 53 */ Internal::GuiProp<Widget, std::string_view>,
        /* 54 */ Internal::PropFieldNotify<Widget, SmallVector<std::string, 1>>,
        /* 55 */ Internal::GuiProp<Widget, MouseInteraction>,
        /* 56 */ Internal::GuiProp<Widget, bool>,
        /* 57 */ Internal::GuiProp<Widget, bool>,
        /* 58 */ Internal::GuiProp<Widget, bool>,
        /* 59 */ Internal::GuiProp<Widget, bool>,
        /* 60 */ Internal::GuiProp<Widget, bool>,
        /* 61 */ Internal::GuiProp<Widget, bool>,
        /* 62 */ Internal::GuiProp<Widget, bool>,
        /* 63 */ Internal::GuiProp<Widget, bool>,
        /* 64 */ Internal::GuiProp<Widget, bool>,
        /* 65 */ Internal::GuiProp<Widget, bool>,
        /* 66 */ Internal::GuiProp<Widget, EventDelegate*>,
        /* 67 */ Internal::GuiProp<Widget, std::string>,
        /* 68 */ Internal::PropFieldNotify<Widget, std::shared_ptr<const Stylesheet>>,
        /* 69 */ Internal::PropFieldNotify<Widget, Painter>,
        /* 70 */ Internal::GuiProp<Widget, bool>,
        /* 71 */ Internal::GuiProp<Widget, OpenTypeFeatureFlags>,
        /* 72 */ Internal::GuiProp<Widget, Animated<ColorW>>,
        /* 73 */ Internal::GuiProp<Widget, Internal::Resolve>,
        /* 74 */ Internal::GuiProp<Widget, Internal::Resolve>,
        /* 75 */ Internal::GuiProp<Widget, Animated<float>>,
        /* 76 */ Internal::GuiProp<Widget, Internal::Resolve>,
        /* 77 */ Internal::GuiProp<Widget, Internal::Resolve>,
        /* 78 */ Internal::GuiProp<Widget, Internal::Resolve>,
        /* 79 */ Internal::GuiProp<Widget, Internal::Resolve>,
        /* 80 */ Internal::GuiProp<Widget, Length>,
        /* 81 */ Internal::GuiProp<Widget, Length>,
        /* 82 */ Internal::GuiProp<Widget, Length>,
        /* 83 */ Internal::GuiProp<Widget, Length>,
        /* 84 */ Internal::GuiProp<Widget, Length>,
        /* 85 */ Internal::GuiProp<Widget, Length>,
        /* 86 */ Internal::GuiProp<Widget, Length>,
        /* 87 */ Internal::GuiProp<Widget, Length>,
        /* 88 */ Internal::GuiProp<Widget, Length>,
        /* 89 */ Internal::GuiProp<Widget, Length>,
        /* 90 */ Internal::GuiProp<Widget, Length>,
        /* 91 */ Internal::GuiProp<Widget, Length>,
        /* 92 */ Internal::GuiProp<Widget, Length>,
        /* 93 */ Internal::GuiProp<Widget, Length>,
        /* 94 */ Internal::GuiProp<Widget, Length>,
        /* 95 */ Internal::GuiProp<Widget, Length>,
        /* 96 */ Internal::GuiProp<Widget, Length>,
        /* 97 */ Internal::GuiProp<Widget, Length>,
        /* 98 */ Internal::GuiProp<Widget, Length>,
        /* 99 */ Internal::GuiProp<Widget, Length>,
        /* 100 */ Internal::GuiProp<Widget, OverflowScroll>,
        /* 101 */ Internal::GuiProp<Widget, OverflowScroll>,
        /* 102 */ Internal::GuiProp<Widget, ContentOverflow>,
        /* 103 */ Internal::GuiProp<Widget, ContentOverflow>,
        /* 104 */ int,
        /* 105 */ int,
        /* 106 */ Internal::GuiPropCompound<Widget, CornersOf, Length, float, 76, 77, 78, 79>,
        /* 107 */ Internal::GuiPropCompound<Widget, EdgesOf, Length, Length, 80, 81, 82, 83>,
        /* 108 */ Internal::GuiPropCompound<Widget, SizeOf, Length, Length, 84, 85>,
        /* 109 */ Internal::GuiPropCompound<Widget, SizeOf, Length, Length, 86, 87>,
        /* 110 */ Internal::GuiPropCompound<Widget, EdgesOf, Length, Length, 88, 89, 90, 91>,
        /* 111 */ Internal::GuiPropCompound<Widget, SizeOf, Length, Length, 92, 93>,
        /* 112 */ Internal::GuiPropCompound<Widget, SizeOf, Length, Length, 94, 95>,
        /* 113 */ Internal::GuiPropCompound<Widget, EdgesOf, Length, Length, 96, 97, 98, 99>,
        /* 114 */ Internal::GuiPropCompound<Widget, SizeOf, OverflowScroll, OverflowScroll, 100, 101>,
        /* 115 */ Internal::GuiPropCompound<Widget, SizeOf, ContentOverflow, ContentOverflow, 102, 103>>&
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
    Property<Widget, ColorW, 12> backgroundColor;
    Property<Widget, ColorW, 15> borderColor;
    Property<Widget, WidgetClip, 16> clip;
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
extern const PropArgument<decltype(Widget::backgroundColor)> backgroundColor;
extern const PropArgument<decltype(Widget::borderColor)> borderColor;
extern const PropArgument<decltype(Widget::borderRadius)> borderRadius;
extern const PropArgument<decltype(Widget::borderWidth)> borderWidth;
extern const PropArgument<decltype(Widget::clip)> clip;
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

template <typename Class, typename T, PropertyIndex index>
struct TransitionDuration : SynthPropertyTag {
    using Type = Seconds;

    static std::string_view name() noexcept {
        static std::string concatenated =
            std::string(Property<Class, T, index>::traits().name) + "-transition-duration";
        return concatenated;
    }
};

template <typename Class, typename T, PropertyIndex index>
struct TransitionEasing : SynthPropertyTag {
    using Type = EasingFunction;

    static std::string_view name() noexcept {
        static std::string concatenated =
            std::string(Property<Class, T, index>::traits().name) + "-transition-easing";
        return concatenated;
    }
};

template <typename Class, typename T, PropertyIndex index>
struct TransitionDelay : SynthPropertyTag {
    using Type = Seconds;

    static std::string_view name() noexcept {
        static std::string concatenated =
            std::string(Property<Class, T, index>::traits().name) + "-transition-delay";
        return concatenated;
    }
};
} // namespace Tag

template <typename Class, typename T, PropertyIndex index>
inline Argument<Tag::TransitionDuration<Class, T, index>> transitionDuration(
    const Argument<Tag::PropArg<Class, T, index>>& argument) {
    return {};
}

template <typename Class, typename T, PropertyIndex index>
inline Argument<Tag::TransitionEasing<Class, T, index>> transitionEasing(
    const Argument<Tag::PropArg<Class, T, index>>& argument) {
    return {};
}

template <typename Class, typename T, PropertyIndex index>
inline Argument<Tag::TransitionDelay<Class, T, index>> transitionDelay(
    const Argument<Tag::PropArg<Class, T, index>>& argument) {
    return {};
}

template <typename Class, typename T, PropertyIndex index>
void applier(Widget* target, ArgVal<Tag::TransitionDuration<Class, T, index>> value) {
    PropertyAnimations& animations = target->animations();
    TransitionParams params        = animations.getTransitionParams(Property<Class, T, index>::id());
    params.duration                = value.value;
    animations.setTransitionParams(Property<Class, T, index>::id(), params);
}

template <typename Class, typename T, PropertyIndex index>
void applier(Widget* target, ArgVal<Tag::TransitionEasing<Class, T, index>> value) {
    PropertyAnimations& animations = target->animations();
    TransitionParams params        = animations.getTransitionParams(Property<Class, T, index>::id());
    params.easing                  = value.value;
    animations.setTransitionParams(Property<Class, T, index>::id(), params);
}

template <typename Class, typename T, PropertyIndex index>
void applier(Widget* target, ArgVal<Tag::TransitionDelay<Class, T, index>> value) {
    PropertyAnimations& animations = target->animations();
    TransitionParams params        = animations.getTransitionParams(Property<Class, T, index>::id());
    params.delay                   = value.value;
    animations.setTransitionParams(Property<Class, T, index>::id(), params);
}

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

namespace Internal {
inline void invalidPropertyApplication(Widget* target, std::string_view name) {
    BRISK_LOG_WARN("Property {} is not applicable to widget type {}", name,
                   target->dynamicMetaClass()->className);
}
} // namespace Internal

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
