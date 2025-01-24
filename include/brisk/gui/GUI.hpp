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

#include <brisk/core/Binding.hpp>
#include <brisk/core/BasicTypes.hpp>
#include <brisk/core/Utilities.hpp>

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
#include <brisk/graphics/RawCanvas.hpp>
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
} // namespace Internal

using BindingFunc = function<void(Widget*)>;

class Stylesheet;
struct Rules;

using OnClick     = WithLifetime<Callback<>>;
using OnItemClick = Callback<size_t>;

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

    explicit Builder(PushFunc builder, BuilderKind kind = BuilderKind::Delayed);
    PushFunc builder;
    BuilderKind kind = BuilderKind::Delayed;

    void run(Widget* w);
};

constexpr bool isInheritable(PropFlags flags) {
    return flags && PropFlags::Inheritable;
}

constexpr bool isTransition(PropFlags flags) {
    return flags && PropFlags::Transition;
}

constexpr bool isResolvable(PropFlags flags) {
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
    using func = function<RC<Widget>()>;

    explicit SingleBuilder(func builder);
};

struct IndexedBuilder : Builder {
    using func = function<RC<Widget>(size_t index)>;

    explicit IndexedBuilder(func builder);
};

template <typename T>
struct ListBuilder : IndexedBuilder {
    explicit ListBuilder(std::vector<T> list, function<RC<Widget>(const std::type_identity_t<T>&)> fn)
        : IndexedBuilder([list = std::move(list), fn = std::move(fn)](size_t index)
                             BRISK_INLINE_LAMBDA -> RC<Widget> {
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
    None       = 0,
    Hover      = 1 << 0,
    Pressed    = 1 << 1,
    Focused    = 1 << 2,
    KeyFocused = 1 << 3,
    Selected   = 1 << 4,
    Disabled   = 1 << 5,
    Last       = Disabled,
};

BRISK_FLAGS(WidgetState)

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

namespace Internal {
template <typename T>
struct ResolvedType {
    using Type = T;
};

template <>
struct ResolvedType<Length> {
    using Type = float;
};

template <>
struct ResolvedType<EdgesL> {
    using Type = EdgesF;
};

template <>
struct ResolvedType<CornersL> {
    using Type = CornersF;
};
} // namespace Internal

template <typename T>
using ResolvedType = typename Internal::ResolvedType<T>::Type;

namespace Internal {

template <typename InputT>
struct Resolve {
    using ResolvedT = typename ResolvedType<InputT>::Type;

    constexpr Resolve(InputT value, ResolvedT resolved = {}) noexcept : value(value), resolved(resolved) {}

    constexpr bool operator==(const Resolve& other) const noexcept {
        return value == other.value;
    }

    InputT value;
    ResolvedT resolved;
};

struct WidgetProps {};

class LayoutEngine;

struct WidgetArgumentAccept {
    void operator()(std::shared_ptr<Widget>);
    void operator()(Builder);
    void operator()(const Attributes&);
    void operator()(const Rules&);
    void operator()(WidgetGroup*);

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
constexpr inline size_t numProperties = 104;
extern const std::string_view propNames[numProperties];

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
    using Type = Internal::Resolve<T>;
};

} // namespace Internal

template <typename T, int subfield = -1>
using PropFieldType = typename Internal::PropFieldType<T, subfield>::Type;

template <typename T, PropFlags flags>
using PropFieldStorageType = typename Internal::PropFieldStorageType<T, flags && PropFlags::Transition,
                                                                     flags && PropFlags::Resolvable>::Type;

template <size_t index_, typename Type_, PropFlags flags_,
          PropFieldStorageType<Type_, flags_> Widget::* field, typename... Properties>
struct GUIPropertyCompound {
    using Type      = Type_;
    using ValueType = Type;

    static_assert(index_ == static_cast<size_t>(-1) || index_ < Internal::numProperties);

    constexpr static PropFlags flags = (Properties::flags | ...) | PropFlags::Compound;

    static std::string_view name() noexcept {
        return Internal::propNames[index_];
    }

    void internalSet(ValueType value);
    void internalSetInherit();

    void operator=(Type value) {
        this->internalSet(std::move(value));
    }

    void operator=(Inherit inherit)
        requires(isInheritable(flags_))
    {
        this->internalSetInherit();
    }

    OptConstRef<Type> get() const noexcept;
    OptConstRef<ResolvedType<Type>> resolved() const noexcept
        requires(isResolvable(flags_));

    void set(Type value) {
        this->internalSet(std::move(value));
    }

    void set(Inherit)
        requires(isInheritable(flags_))
    {
        this->internalSetInherit();
    }

    BindingAddress address() const noexcept;

    Widget* this_pointer;
};

template <size_t index_, typename T, PropFlags flags_, PropFieldStorageType<T, flags_> Widget::* field_,
          int subfield_ = -1>
struct GUIProperty {
public:
    using Type      = T;
    using ValueType = PropFieldType<T, subfield_>;

    static_assert(index_ == static_cast<size_t>(-1) || index_ < Internal::numProperties);

    static ValueType sub(const Type& value) {
        if constexpr (subfield == -1)
            return value;
        else
            return accessField<subfield>(value);
    }

    static std::string_view name() noexcept {
        return Internal::propNames[index_];
    }

    constexpr static size_t index    = index_;

    constexpr static auto field      = field_;
    constexpr static int subfield    = subfield_;

    constexpr static PropFlags flags = flags_;

    operator ValueType() const noexcept {
        return this->get();
    }

    void operator=(ValueType value) {
        this->internalSet(std::move(value));
    }

    void operator=(Inherit inherit)
        requires(isInheritable(flags_))
    {
        this->internalSetInherit();
    }

    OptConstRef<ValueType> get() const noexcept;

    OptConstRef<ResolvedType<ValueType>> resolved() const noexcept
        requires(isResolvable(flags_));
    OptConstRef<ValueType> current() const noexcept
        requires(isTransition(flags_));

    void internalSet(ValueType value);
    void internalSetInherit();

    void set(ValueType value) {
        internalSet(std::move(value));
    }

    void set(Inherit inherit)
        requires(isInheritable(flags_))
    {
        this->internalSetInherit();
    }

    BindingAddress address() const noexcept;

    void operator=(Value<ValueType> value) {
        this->set(std::move(value));
    }

    void set(Value<ValueType> value) {
        bindings->connectBidir(Value<ValueType>{ this }, std::move(value));
    }

    template <invocable_r<ValueType> Fn>
    void operator=(Fn&& fn) {
        this->internalSet(std::forward<Fn>(fn)());
    }

    template <invocable_r<ValueType> Fn>
    void set(Fn&& fn) {
        this->internalSet(std::forward<Fn>(fn)());
    }

    template <invocable_r<ValueType, Widget*> Fn>
    void operator=(Fn&& fn) {
        this->internalSet(std::forward<Fn>(fn)(this_pointer));
    }

    template <invocable_r<ValueType, Widget*> Fn>
    void set(Fn&& fn) {
        this->internalSet(std::forward<Fn>(fn)(this_pointer));
    }

    Widget* this_pointer;
};

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

template <typename PropertyType>
struct PropArg : PropertyTag {
    using Type = typename PropertyType::ValueType;

    struct ExtraTypes {
        static void accept(Value<Type>);
        static void accept(function<Type()>);
        static void accept(function<Type(Widget*)>);
    };
};

template <size_t index, typename T, PropFlags flags, PropFieldStorageType<T, flags> Widget::* field,
          int subfield>
BRISK_IF_GNU(requires(!requires {
    { T::isTrigger };
}))
struct PropArg<GUIProperty<index, T, flags, field, subfield>> : PropertyTag {
    using Type = typename GUIProperty<index, T, flags, field, subfield>::ValueType;

    static std::string_view name() noexcept {
        return GUIProperty<index, T, flags, field, subfield>::name();
    }

    struct ExtraTypes {
        static void accept(Inherit)
            requires(isInheritable(flags));
        static void accept(Value<Type>);
        static void accept(function<Type()>);
        static void accept(function<Type(Widget*)>);
    };
};

template <size_t index, typename... Args, PropFlags flags, Trigger<Args...> Widget::* field, int subfield>
struct PropArg<GUIProperty<index, Trigger<Args...>, flags, field, subfield>> : PropertyTag {
    using Type = Value<Trigger<Args...>>;
};

template <size_t index_, typename Type_, PropFlags flags_,
          PropFieldStorageType<Type_, flags_> Widget::* field, typename... Properties>
struct PropArg<GUIPropertyCompound<index_, Type_, flags_, field, Properties...>> : PropertyTag {

    static std::string_view name() noexcept {
        return GUIPropertyCompound<index_, Type_, flags_, field, Properties...>::name();
    }

    using Type = Type_;

    struct ExtraTypes {
        static void accept(function<Type()>);
        static void accept(function<Type(Widget*)>);
    };
};

} // namespace Tag

template <std::derived_from<Widget> Target, typename PropertyType, typename... Args>
inline void applier(Target* target,
                    const ArgVal<Tag::PropArg<PropertyType>, Value<Trigger<Args...>>>& value) {
    BRISK_ASSERT(target);
    PropertyType prop{ target };
    bindings->connect(value.value, Internal::asValue(prop), BindType::Immediate, false);
}

template <std::derived_from<Widget> Target, typename PropertyType, typename U>
inline void applier(Target* target, const ArgVal<Tag::PropArg<PropertyType>, U>& value)
    requires(!Internal::isTrigger<typename PropertyType::Type>)
{
    PropertyType prop{ target };
    prop.set(value.value);
}

using StyleVarType = std::variant<std::monostate, ColorF, EdgesL, float, int>;

class WIDGET Widget : public BindingObject<Widget, &uiThread> {
public:
    using Ptr                 = std::shared_ptr<Widget>;
    using WidgetPtrs          = std::vector<Ptr>;
    using WidgetIterator      = typename WidgetPtrs::iterator;
    using WidgetConstIterator = typename WidgetPtrs::const_iterator;

    static RC<Scheduler> dispatcher() {
        return uiThread;
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

    virtual optional<std::string> textContent() const;

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

        const RC<Widget>& operator*() const;

        bool operator!=(std::nullptr_t) const;
    };

    struct IteratorEx {
        const Widget* w;
        size_t i;
        bool reverse;

        void operator++();

        const RC<Widget>& operator*() const;

        bool operator!=(std::nullptr_t) const;
    };

    Iterator begin() const;
    std::nullptr_t end() const;

    IteratorEx rbegin() const;
    std::nullptr_t rend() const;

    IteratorEx begin(bool reverse) const;

    template <typename Fn>
    void bubble(Fn&& fn, bool includePopup = false) {
        RC<Widget> current = this->shared_from_this();
        while (current) {
            if (!fn(current.get()))
                return;
            if (current->m_zorder != ZOrder::Normal && !includePopup)
                return;
            current = current->m_parent ? current->m_parent->shared_from_this() : nullptr;
        }
    }

    template <std::derived_from<Widget> Type = Widget, typename Fn>
    void enumerate(Fn&& fn, bool recursive = false, bool recursiveForMatching = true) {
        for (const Ptr& w : *this) {
            if (Type* t = dynamic_cast<Type*>(w.get())) {
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
            std::shared_ptr<Type> typed = std::dynamic_pointer_cast<Type>(*it);
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
            std::shared_ptr<WidgetClass> ww = std::dynamic_pointer_cast<WidgetClass>(w);
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
            std::shared_ptr<WidgetClass> ww = std::dynamic_pointer_cast<WidgetClass>(w);
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

    optional<WidgetIterator> findIterator(Widget* widget, Widget** parent = nullptr);

    ////////////////////////////////////////////////////////////////////////////////
    // Geometry
    ////////////////////////////////////////////////////////////////////////////////

    bool isVisible() const noexcept;

    Rectangle rect() const noexcept;

    Rectangle clientRect() const noexcept;

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
    std::optional<T> getStyleVar(unsigned id) const;

    template <typename T>
    T getStyleVar(unsigned id, T fallback) const;

    template <StyleVarTag Tag>
    void set(Tag, typename Tag::Type value) {
        if (Tag::id >= m_styleVars.size()) {
            m_styleVars.resize(Tag::id + 1, std::monostate{});
        }
        if (assign(m_styleVars[Tag::id], value))
            requestRestyle();
    }

    void apply(const Rules& rules);

    void apply(const Attributes& arg);

    void requestUpdateLayout();
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

    ////////////////////////////////////////////////////////////////////////////////
    // Tree & Children
    ////////////////////////////////////////////////////////////////////////////////

    const WidgetPtrs& widgets() const;

    WidgetTree* tree() const noexcept;
    void setTree(WidgetTree* tree);

    Widget* parent() const noexcept;

    void removeAt(size_t pos);
    void removeIf(function<bool(Widget*)> predicate);
    void remove(Widget* widget);
    void clear();

    virtual void append(RC<Widget> widget);
    void apply(RC<Widget> widget);

    virtual void onParentChanged();

    virtual void attached();

    /// @brief Replaces oldWidget with newWidget.
    /// @param deep in subtree
    /// @returns false if oldWidget wasn't found
    bool replace(Ptr oldWidget, Ptr newWidget, bool deep);

    void apply(std::nullptr_t) = delete;

    void apply(WidgetGroup* group);

    virtual void onChildAdded(Widget* w);
    virtual void childrenAdded();

    std::optional<size_t> indexOf(const Widget* widget) const;

    ////////////////////////////////////////////////////////////////////////////////

    void paintTo(Canvas& canvas) const;

    Drawable drawable(RectangleF scissors) const;

    optional<PointF> mousePos() const;

    void reveal();

    bool setScrollOffset(Point newOffset);
    bool setScrollOffset(Orientation orientation, int newOffset);
    Point scrollOffset() const;
    int scrollOffset(Orientation orientation) const;
    bool hasScrollBar(Orientation orientation) const noexcept;
    int scrollSize(Orientation orientation) const noexcept;
    Size scrollSize() const noexcept;

    using enum PropFlags;

    friend struct Rules;

protected:
    friend struct InputQueue;
    friend class Stylesheet;
    friend class WidgetTree;

    WidgetTree* m_tree = nullptr;

    RC<const Stylesheet> m_stylesheet;
    Painter m_painter;

    optional<PointF> m_mousePos;

    bool m_inConstruction : 1       = true;
    bool m_constructed : 1          = false;
    bool m_isPopup : 1              = false; // affected by closeNearestPopup
    bool m_processClicks : 1        = true;
    bool m_styleApplying : 1        = false;
    bool m_ignoreChildrenOffset : 1 = false;

    // functions
    Trigger<> m_onClick;
    Trigger<> m_onDoubleClick;

    mutable bool m_hintShown = false;
    function<void(Widget*)> m_reapplyStyle;

    // strings
    std::string m_description;
    std::string m_type;
    std::string m_id;
    std::string m_hint;
    std::string_view m_role;
    Classes m_classes;

    Rectangle m_rect{ 0, 0, 0, 0 };
    Rectangle m_clientRect{ 0, 0, 0, 0 };
    EdgesF m_computedMargin{ 0, 0, 0, 0 };
    EdgesF m_computedPadding{ 0, 0, 0, 0 };
    EdgesF m_computedBorderWidth{ 0, 0, 0, 0 };
    Size m_contentSize{ 0, 0 };

    EdgesL m_margin{ 0, 0, 0, 0 };
    EdgesL m_padding{ 0, 0, 0, 0 };
    EdgesL m_borderWidth{ 0, 0, 0, 0 };

    Internal::Transition<ColorF> m_backgroundColor{ Palette::transparent };
    Internal::Transition<ColorF> m_borderColor{ Palette::transparent };
    Internal::Transition<ColorF> m_color{ Palette::white };
    Internal::Transition<ColorF> m_shadowColor{ Palette::black.multiplyAlpha(0.4f) };
    Internal::Transition<ColorF> m_scrollBarColor{ Palette::grey };
    float m_backgroundColorTransition      = 0;
    float m_borderColorTransition          = 0;
    float m_colorTransition                = 0;
    float m_shadowColorTransition          = 0;
    EasingFunction m_backgroundColorEasing = &easeLinear;
    EasingFunction m_borderColorEasing     = &easeLinear;
    EasingFunction m_colorEasing           = &easeLinear;
    EasingFunction m_shadowColorEasing     = &easeLinear;

    // point/size
    PointL m_absolutePosition{ undef, undef }; // for popup only
    PointL m_anchor{ undef, undef };           // for popup only
    SizeL m_minDimensions{ undef, undef };
    SizeL m_maxDimensions{ undef, undef };
    SizeL m_dimensions{ undef, undef };
    PointL m_translate{ 0, 0 }; // translation relative to own size
    SizeL m_gap                    = { 0, 0 };

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

    // int
    int m_corners                  = +CornerFlags::All;
    Cursor m_cursor                = Cursor::NotSet;
    int m_tabGroupId               = -1;

    Internal::Resolve<CornersL> m_borderRadius{ CornersL(0_px), CornersF(0.f) };
    Internal::Resolve<Length> m_shadowSize{ 0_px };
    Internal::Resolve<Length> m_fontSize{ FontSize::Normal, dp(FontSize::Normal) };
    Internal::Resolve<Length> m_tabSize{ 40, 40 };
    Internal::Resolve<Length> m_letterSpacing{ 0_px, 0.f };
    Internal::Resolve<Length> m_wordSpacing{ 0_px, 0.f };
    Internal::Resolve<Length> m_scrollBarThickness{ 8_px, 8.f };
    Internal::Resolve<Length> m_scrollBarRadius{ 0_px, 0.f };

    // uint8_t
    mutable WidgetState m_state         = WidgetState::None;
    FontFamily m_fontFamily             = Fonts::DefaultPlusIconsEmoji;
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
    WidgetClip m_clip                   = WidgetClip::All;
    Overflow m_overflow                 = Overflow::None;
    AlignContent m_alignContent         = AlignContent::FlexStart;
    Wrap m_flexWrap                     = Wrap::NoWrap;
    BoxSizingPerAxis m_boxSizing        = BoxSizingPerAxis::BorderBox;
    AlignToViewport m_alignToViewport   = AlignToViewport::None;
    TextAlign m_textAlign               = TextAlign::Start;
    TextAlign m_textVerticalAlign       = TextAlign::Center;
    MouseInteraction m_mouseInteraction = MouseInteraction::Inherit;

    bool m_tabStop                      = false;
    bool m_tabGroup                     = false;
    bool m_visible                      = true;
    bool m_hidden                       = false;
    bool m_autofocus                    = false;
    bool m_mousePassThrough             = false;
    bool m_autoMouseCapture             = true;
    bool m_mouseAnywhere                = false;
    bool m_focusCapture                 = false;
    bool m_stateTriggersRestyle         = false;
    bool m_isHintExclusive              = false;

    std::array<bool, 2> m_scrollBarDrag{ false, false };
    int m_savedScrollOffset = 0;

    struct ScrollBarGeometry {
        Rectangle track;
        Rectangle thumb;
    };

    Range<int> scrollBarRange(Orientation orientation) const noexcept;
    ScrollBarGeometry scrollBarGeometry(Orientation orientation) const noexcept;

    void updateScrollAxes();

    std::bitset<Internal::propStateBits * Internal::numProperties> m_propStates;
    Internal::PropState getPropState(size_t index) const noexcept;
    void setPropState(size_t index, Internal::PropState state) noexcept;

    std::vector<StyleVarType> m_styleVars;

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
    void updateGeometry();
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
    virtual void paint(Canvas& canvas) const;
    virtual void postPaint(Canvas& canvas) const;
    virtual void paintScrollBar(Canvas& canvas, Orientation orientation,
                                const ScrollBarGeometry& geometry) const;
    void paintBackground(Canvas& canvas, Rectangle rect) const;
    void paintHint(Canvas& canvas) const;
    void paintFocusFrame(Canvas& canvas) const;
    void paintChildren(Canvas& canvas) const;
    void paintScrollBars(Canvas& canvas) const;

    ///////////////////////////////////////////////////////////////////////////////

    Size viewportSize() const noexcept;
    void setRect(Rectangle rect);

    void rebuildOne(Builder builder);

    virtual Ptr getContextWidget();

    void insertChild(WidgetConstIterator it, Ptr w);
    void addChild(Ptr w);

    void stateChanged(WidgetState oldState, WidgetState newState);

    bool transitionAllowed();

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

    /// @brief
    virtual void close(Widget* sender);

    void resolveProperties(PropFlags flags);
    void restyleIfRequested();
    void requestUpdates(PropFlags flags);

    template <typename T, PropFlags flags, PropFieldStorageType<T, flags> Widget::* field, int subfield>
    OptConstRef<PropFieldType<T, subfield>> getter() const noexcept;

    template <typename T, PropFlags flags, PropFieldStorageType<T, flags> Widget::* field, int subfield>
    OptConstRef<ResolvedType<PropFieldType<T, subfield>>> getterResolved() const noexcept;

    template <typename T, PropFlags flags, PropFieldStorageType<T, flags> Widget::* field, int subfield>
    OptConstRef<PropFieldType<T, subfield>> getterCurrent() const noexcept;

    template <typename T>
    float Widget::* transitionField(Internal::Transition<T> Widget::* field) const noexcept;

    template <typename T, size_t index, PropFlags flags, PropFieldStorageType<T, flags> Widget::* field,
              int subfield>
    void setter(PropFieldType<T, subfield> value);

    template <typename T, size_t index, PropFlags flags, PropFieldStorageType<T, flags> Widget::* field,
              int subfield>
    void setter(Inherit);

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

    Trigger<> m_rebuildTrigger{};

    WidgetPtrs m_widgets;
    std::vector<BuilderData> m_builders;
    std::set<WidgetGroup*> m_groups;

    friend struct WidgetGroup;

    explicit Widget(Construction construction);
    void childAdded(Widget* w);
    int32_t applyLayoutRecursively(RectangleF rectangle);

    friend class Internal::LayoutEngine;
    ClonablePtr<Internal::LayoutEngine> m_layoutEngine;

    void removeFromGroup(WidgetGroup* group);

    void replaceChild(WidgetIterator it, Ptr newWidget);
    void removeChild(WidgetConstIterator it);
    void childRemoved(Ptr child);

    void processVisibility(bool isVisible);
    void reposition(Point relativeOffset);
    void refreshTree();
    void paintTree(RawCanvas& canvas);

    void doRestyle();
    void doRestyle(std::shared_ptr<const Stylesheet> stylesheet, bool root);

    float resolveFontHeight() const;
    void updateGeometryAndProcessEvents();
    void updateLayout(Rectangle rectangle);

    void layoutSet();
    void layoutResetRecursively();

    void setDisabled(bool);

    template <size_t index, typename T, PropFlags flags, PropFieldStorageType<T, flags> Widget::* field,
              int subfield>
    friend struct GUIProperty;

    using This                      = Widget;

    constexpr static size_t noIndex = static_cast<size_t>(-1);

public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PROPERTIES
    ////////////////////////////////////////////////////////////////////////////////////////////////////

    BRISK_PROPERTIES_BEGIN
    GUIProperty<0, PointL, AffectLayout, &This::m_absolutePosition> absolutePosition;
    GUIProperty<1, AlignContent, AffectLayout, &This::m_alignContent> alignContent;
    GUIProperty<2, AlignItems, AffectLayout, &This::m_alignItems> alignItems;
    GUIProperty<3, AlignSelf, AffectLayout, &This::m_alignSelf> alignSelf;
    GUIProperty<4, PointL, AffectLayout, &This::m_anchor> anchor;
    GUIProperty<5, OptFloat, AffectLayout, &This::m_aspect> aspect;
    GUIProperty<6, EasingFunction, None, &This::m_backgroundColorEasing> backgroundColorEasing;
    GUIProperty<7, float, None, &This::m_backgroundColorTransition> backgroundColorTransition;
    GUIProperty<8, ColorF, Transition, &This::m_backgroundColor> backgroundColor;
    GUIProperty<9, EasingFunction, None, &This::m_borderColorEasing> borderColorEasing;
    GUIProperty<10, float, None, &This::m_borderColorTransition> borderColorTransition;
    GUIProperty<11, ColorF, Transition, &This::m_borderColor> borderColor;
    GUIProperty<12, CornersL, Resolvable | Inheritable, &This::m_borderRadius, 0> borderRadiusTopLeft;
    GUIProperty<13, CornersL, Resolvable | Inheritable, &This::m_borderRadius, 1> borderRadiusTopRight;
    GUIProperty<14, CornersL, Resolvable | Inheritable, &This::m_borderRadius, 2> borderRadiusBottomLeft;
    GUIProperty<15, CornersL, Resolvable | Inheritable, &This::m_borderRadius, 3> borderRadiusBottomRight;
    GUIProperty<16, EdgesL, AffectLayout, &This::m_borderWidth, 0> borderWidthLeft;
    GUIProperty<17, EdgesL, AffectLayout, &This::m_borderWidth, 1> borderWidthTop;
    GUIProperty<18, EdgesL, AffectLayout, &This::m_borderWidth, 2> borderWidthRight;
    GUIProperty<19, EdgesL, AffectLayout, &This::m_borderWidth, 3> borderWidthBottom;
    GUIProperty<20, WidgetClip, None, &This::m_clip> clip;
    GUIProperty<21, EasingFunction, None, &This::m_colorEasing> colorEasing;
    GUIProperty<22, float, None, &This::m_colorTransition> colorTransition;
    GUIProperty<23, ColorF, Transition | Inheritable, &This::m_color> color;
    GUIProperty<24, int, None, &This::m_corners> corners;
    GUIProperty<25, Cursor, None, &This::m_cursor> cursor;
    GUIProperty<26, SizeL, AffectLayout, &This::m_dimensions, 0> width;
    GUIProperty<27, SizeL, AffectLayout, &This::m_dimensions, 1> height;
    GUIProperty<28, Length, AffectLayout, &This::m_flexBasis> flexBasis;
    GUIProperty<29, OptFloat, AffectLayout, &This::m_flexGrow> flexGrow;
    GUIProperty<30, OptFloat, AffectLayout, &This::m_flexShrink> flexShrink;
    GUIProperty<31, Wrap, AffectLayout, &This::m_flexWrap> flexWrap;
    GUIProperty<32, FontFamily, AffectLayout | AffectFont | Inheritable, &This::m_fontFamily> fontFamily;
    GUIProperty<33, Length,
                AffectLayout | Resolvable | AffectResolve | AffectFont | Inheritable | RelativeToParent,
                &This::m_fontSize>
        fontSize;
    GUIProperty<34, FontStyle, AffectLayout | AffectFont | Inheritable, &This::m_fontStyle> fontStyle;
    GUIProperty<35, FontWeight, AffectLayout | AffectFont | Inheritable, &This::m_fontWeight> fontWeight;
    GUIProperty<36, SizeL, AffectLayout, &This::m_gap, 0> gapColumn;
    GUIProperty<37, SizeL, AffectLayout, &This::m_gap, 1> gapRow;
    GUIProperty<38, bool, None, &This::m_hidden> hidden;
    GUIProperty<39, Justify, AffectLayout, &This::m_justifyContent> justifyContent;
    GUIProperty<40, LayoutOrder, AffectLayout, &This::m_layoutOrder> layoutOrder;
    GUIProperty<41, Layout, AffectLayout, &This::m_layout> layout;
    GUIProperty<42, Length, AffectLayout | Resolvable | AffectFont | Inheritable, &This::m_letterSpacing>
        letterSpacing;
    GUIProperty<43, EdgesL, AffectLayout, &This::m_margin, 0> marginLeft;
    GUIProperty<44, EdgesL, AffectLayout, &This::m_margin, 1> marginTop;
    GUIProperty<45, EdgesL, AffectLayout, &This::m_margin, 2> marginRight;
    GUIProperty<46, EdgesL, AffectLayout, &This::m_margin, 3> marginBottom;
    GUIProperty<47, SizeL, AffectLayout, &This::m_maxDimensions, 0> maxWidth;
    GUIProperty<48, SizeL, AffectLayout, &This::m_maxDimensions, 1> maxHeight;
    GUIProperty<49, SizeL, AffectLayout, &This::m_minDimensions, 0> minWidth;
    GUIProperty<50, SizeL, AffectLayout, &This::m_minDimensions, 1> minHeight;
    GUIProperty<51, float, None, &This::m_opacity> opacity;
    GUIProperty<52, Overflow, AffectLayout, &This::m_overflow> overflow;
    GUIProperty<53, EdgesL, AffectLayout, &This::m_padding, 0> paddingLeft;
    GUIProperty<54, EdgesL, AffectLayout, &This::m_padding, 1> paddingTop;
    GUIProperty<55, EdgesL, AffectLayout, &This::m_padding, 2> paddingRight;
    GUIProperty<56, EdgesL, AffectLayout, &This::m_padding, 3> paddingBottom;
    GUIProperty<57, Placement, AffectLayout, &This::m_placement> placement;
    GUIProperty<58, Length, Resolvable | Inheritable, &This::m_shadowSize> shadowSize;
    GUIProperty<59, ColorF, Transition, &This::m_shadowColor> shadowColor;
    GUIProperty<60, float, None, &This::m_shadowColorTransition> shadowColorTransition;
    GUIProperty<61, EasingFunction, None, &This::m_shadowColorEasing> shadowColorEasing;
    GUIProperty<62, Length, AffectLayout | Resolvable | AffectFont | Inheritable, &This::m_tabSize> tabSize;
    GUIProperty<63, TextAlign, Inheritable, &This::m_textAlign> textAlign;
    GUIProperty<64, TextAlign, Inheritable, &This::m_textVerticalAlign> textVerticalAlign;
    GUIProperty<65, TextDecoration, AffectFont | Inheritable, &This::m_textDecoration> textDecoration;
    GUIProperty<66, PointL, AffectLayout, &This::m_translate> translate;
    GUIProperty<67, bool, AffectLayout, &This::m_visible> visible;
    GUIProperty<68, Length, AffectLayout | Resolvable | AffectFont | Inheritable, &This::m_wordSpacing>
        wordSpacing;
    GUIProperty<69, AlignToViewport, AffectLayout, &This::m_alignToViewport> alignToViewport;
    GUIProperty<70, BoxSizingPerAxis, AffectLayout, &This::m_boxSizing> boxSizing;
    GUIProperty<71, ZOrder, AffectLayout, &This::m_zorder> zorder;

    GUIProperty<72, bool, AffectStyle, &This::m_stateTriggersRestyle> stateTriggersRestyle;
    GUIProperty<73, std::string, AffectStyle, &This::m_id> id;
    GUIProperty<74, std::string_view, AffectStyle, &This::m_role> role;
    GUIProperty<75, Classes, AffectStyle, &This::m_classes> classes;
    GUIProperty<76, MouseInteraction, None, &This::m_mouseInteraction> mouseInteraction;
    GUIProperty<77, bool, None, &This::m_mousePassThrough> mousePassThrough;
    GUIProperty<78, bool, None, &This::m_autoMouseCapture> autoMouseCapture;
    GUIProperty<79, bool, None, &This::m_mouseAnywhere> mouseAnywhere;
    GUIProperty<80, bool, None, &This::m_focusCapture> focusCapture;
    GUIProperty<81, std::string, None, &This::m_description> description;
    GUIProperty<82, bool, None, &This::m_tabStop> tabStop;
    GUIProperty<83, bool, None, &This::m_tabGroup> tabGroup;
    GUIProperty<84, bool, None, &This::m_autofocus> autofocus;
    GUIProperty<85, Trigger<>, None, &This::m_onClick> onClick;
    GUIProperty<86, Trigger<>, None, &This::m_onDoubleClick> onDoubleClick;
    GUIProperty<87, EventDelegate*, None, &This::m_delegate> delegate;
    GUIProperty<88, std::string, None, &This::m_hint> hint;
    GUIProperty<89, std::shared_ptr<const Stylesheet>, AffectStyle, &This::m_stylesheet> stylesheet;
    GUIProperty<90, Painter, None, &This::m_painter> painter;
    GUIProperty<91, bool, None, &This::m_isHintExclusive> isHintExclusive;

    GUIPropertyCompound<92, CornersL, Resolvable | Inheritable, &This::m_borderRadius,
                        decltype(borderRadiusTopLeft), decltype(borderRadiusTopRight),
                        decltype(borderRadiusBottomLeft), decltype(borderRadiusBottomRight)>
        borderRadius;
    GUIPropertyCompound<93, EdgesL, AffectLayout, &This::m_borderWidth, decltype(borderWidthLeft),
                        decltype(borderWidthTop), decltype(borderWidthRight), decltype(borderWidthBottom)>
        borderWidth;
    GUIPropertyCompound<94, SizeL, AffectLayout, &This::m_dimensions, decltype(width), decltype(height)>
        dimensions;
    GUIPropertyCompound<95, SizeL, AffectLayout, &This::m_gap, decltype(gapColumn), decltype(gapRow)> gap;
    GUIPropertyCompound<96, EdgesL, AffectLayout, &This::m_margin, decltype(marginLeft), decltype(marginTop),
                        decltype(marginRight), decltype(marginBottom)>
        margin;
    GUIPropertyCompound<97, SizeL, AffectLayout, &This::m_maxDimensions, decltype(maxWidth),
                        decltype(maxHeight)>
        maxDimensions;
    GUIPropertyCompound<98, SizeL, AffectLayout, &This::m_minDimensions, decltype(minWidth),
                        decltype(minHeight)>
        minDimensions;
    GUIPropertyCompound<99, EdgesL, AffectLayout, &This::m_padding, decltype(paddingLeft),
                        decltype(paddingTop), decltype(paddingRight), decltype(paddingBottom)>
        padding;
    GUIProperty<100, OpenTypeFeatureFlags, AffectLayout | AffectFont | Inheritable, &This::m_fontFeatures>
        fontFeatures;

    GUIProperty<101, ColorF, Transition | Inheritable, &This::m_scrollBarColor> scrollBarColor;
    GUIProperty<102, Length, Resolvable, &This::m_scrollBarThickness> scrollBarThickness;
    GUIProperty<103, Length, Resolvable, &This::m_scrollBarRadius> scrollBarRadius;

    Property<This, bool, &This::m_state, &This::isDisabled, &This::setDisabled> disabled;
    BRISK_PROPERTIES_END
};

constinit inline size_t widgetSize = sizeof(Widget);

inline namespace Arg {

extern const Argument<Tag::PropArg<decltype(Widget::absolutePosition)>> absolutePosition;
extern const Argument<Tag::PropArg<decltype(Widget::alignContent)>> alignContent;
extern const Argument<Tag::PropArg<decltype(Widget::alignItems)>> alignItems;
extern const Argument<Tag::PropArg<decltype(Widget::alignSelf)>> alignSelf;
extern const Argument<Tag::PropArg<decltype(Widget::anchor)>> anchor;
extern const Argument<Tag::PropArg<decltype(Widget::aspect)>> aspect;
extern const Argument<Tag::PropArg<decltype(Widget::backgroundColorEasing)>> backgroundColorEasing;
extern const Argument<Tag::PropArg<decltype(Widget::backgroundColorTransition)>> backgroundColorTransition;
extern const Argument<Tag::PropArg<decltype(Widget::backgroundColor)>> backgroundColor;
extern const Argument<Tag::PropArg<decltype(Widget::borderColorEasing)>> borderColorEasing;
extern const Argument<Tag::PropArg<decltype(Widget::borderColorTransition)>> borderColorTransition;
extern const Argument<Tag::PropArg<decltype(Widget::borderColor)>> borderColor;
extern const Argument<Tag::PropArg<decltype(Widget::borderRadius)>> borderRadius;
extern const Argument<Tag::PropArg<decltype(Widget::borderWidth)>> borderWidth;
extern const Argument<Tag::PropArg<decltype(Widget::clip)>> clip;
extern const Argument<Tag::PropArg<decltype(Widget::colorEasing)>> colorEasing;
extern const Argument<Tag::PropArg<decltype(Widget::colorTransition)>> colorTransition;
extern const Argument<Tag::PropArg<decltype(Widget::color)>> color;
extern const Argument<Tag::PropArg<decltype(Widget::corners)>> corners;
extern const Argument<Tag::PropArg<decltype(Widget::cursor)>> cursor;
extern const Argument<Tag::PropArg<decltype(Widget::dimensions)>> dimensions;
extern const Argument<Tag::PropArg<decltype(Widget::flexBasis)>> flexBasis;
extern const Argument<Tag::PropArg<decltype(Widget::flexGrow)>> flexGrow;
extern const Argument<Tag::PropArg<decltype(Widget::flexShrink)>> flexShrink;
extern const Argument<Tag::PropArg<decltype(Widget::flexWrap)>> flexWrap;
extern const Argument<Tag::PropArg<decltype(Widget::fontFamily)>> fontFamily;
extern const Argument<Tag::PropArg<decltype(Widget::fontSize)>> fontSize;
extern const Argument<Tag::PropArg<decltype(Widget::fontStyle)>> fontStyle;
extern const Argument<Tag::PropArg<decltype(Widget::fontWeight)>> fontWeight;
extern const Argument<Tag::PropArg<decltype(Widget::gap)>> gap;
extern const Argument<Tag::PropArg<decltype(Widget::hidden)>> hidden;
extern const Argument<Tag::PropArg<decltype(Widget::justifyContent)>> justifyContent;
extern const Argument<Tag::PropArg<decltype(Widget::layoutOrder)>> layoutOrder;
extern const Argument<Tag::PropArg<decltype(Widget::layout)>> layout;
extern const Argument<Tag::PropArg<decltype(Widget::letterSpacing)>> letterSpacing;
extern const Argument<Tag::PropArg<decltype(Widget::margin)>> margin;
extern const Argument<Tag::PropArg<decltype(Widget::maxDimensions)>> maxDimensions;
extern const Argument<Tag::PropArg<decltype(Widget::minDimensions)>> minDimensions;
extern const Argument<Tag::PropArg<decltype(Widget::opacity)>> opacity;
extern const Argument<Tag::PropArg<decltype(Widget::overflow)>> overflow;
extern const Argument<Tag::PropArg<decltype(Widget::padding)>> padding;
extern const Argument<Tag::PropArg<decltype(Widget::placement)>> placement;
extern const Argument<Tag::PropArg<decltype(Widget::shadowSize)>> shadowSize;
extern const Argument<Tag::PropArg<decltype(Widget::shadowColor)>> shadowColor;
extern const Argument<Tag::PropArg<decltype(Widget::shadowColorTransition)>> shadowColorTransition;
extern const Argument<Tag::PropArg<decltype(Widget::shadowColorEasing)>> shadowColorEasing;
extern const Argument<Tag::PropArg<decltype(Widget::tabSize)>> tabSize;
extern const Argument<Tag::PropArg<decltype(Widget::textAlign)>> textAlign;
extern const Argument<Tag::PropArg<decltype(Widget::textVerticalAlign)>> textVerticalAlign;
extern const Argument<Tag::PropArg<decltype(Widget::textDecoration)>> textDecoration;
extern const Argument<Tag::PropArg<decltype(Widget::translate)>> translate;
extern const Argument<Tag::PropArg<decltype(Widget::visible)>> visible;
extern const Argument<Tag::PropArg<decltype(Widget::wordSpacing)>> wordSpacing;
extern const Argument<Tag::PropArg<decltype(Widget::alignToViewport)>> alignToViewport;
extern const Argument<Tag::PropArg<decltype(Widget::stateTriggersRestyle)>> stateTriggersRestyle;
extern const Argument<Tag::PropArg<decltype(Widget::id)>> id;
extern const Argument<Tag::PropArg<decltype(Widget::role)>> role;
extern const Argument<Tag::PropArg<decltype(Widget::classes)>> classes;
extern const Argument<Tag::PropArg<decltype(Widget::mouseInteraction)>> mouseInteraction;
extern const Argument<Tag::PropArg<decltype(Widget::mousePassThrough)>> mousePassThrough;
extern const Argument<Tag::PropArg<decltype(Widget::autoMouseCapture)>> autoMouseCapture;
extern const Argument<Tag::PropArg<decltype(Widget::mouseAnywhere)>> mouseAnywhere;
extern const Argument<Tag::PropArg<decltype(Widget::focusCapture)>> focusCapture;
extern const Argument<Tag::PropArg<decltype(Widget::description)>> description;
extern const Argument<Tag::PropArg<decltype(Widget::tabStop)>> tabStop;
extern const Argument<Tag::PropArg<decltype(Widget::tabGroup)>> tabGroup;
extern const Argument<Tag::PropArg<decltype(Widget::autofocus)>> autofocus;
extern const Argument<Tag::PropArg<decltype(Widget::onClick)>> onClick;
extern const Argument<Tag::PropArg<decltype(Widget::onDoubleClick)>> onDoubleClick;
extern const Argument<Tag::PropArg<decltype(Widget::delegate)>> delegate;
extern const Argument<Tag::PropArg<decltype(Widget::hint)>> hint;
extern const Argument<Tag::PropArg<decltype(Widget::zorder)>> zorder;
extern const Argument<Tag::PropArg<decltype(Widget::stylesheet)>> stylesheet;
extern const Argument<Tag::PropArg<decltype(Widget::painter)>> painter;
extern const Argument<Tag::PropArg<decltype(Widget::isHintExclusive)>> isHintExclusive;

extern const Argument<Tag::PropArg<decltype(Widget::borderRadiusTopLeft)>> borderRadiusTopLeft;
extern const Argument<Tag::PropArg<decltype(Widget::borderRadiusTopRight)>> borderRadiusTopRight;
extern const Argument<Tag::PropArg<decltype(Widget::borderRadiusBottomLeft)>> borderRadiusBottomLeft;
extern const Argument<Tag::PropArg<decltype(Widget::borderRadiusBottomRight)>> borderRadiusBottomRight;

extern const Argument<Tag::PropArg<decltype(Widget::borderWidthLeft)>> borderWidthLeft;
extern const Argument<Tag::PropArg<decltype(Widget::borderWidthTop)>> borderWidthTop;
extern const Argument<Tag::PropArg<decltype(Widget::borderWidthRight)>> borderWidthRight;
extern const Argument<Tag::PropArg<decltype(Widget::borderWidthBottom)>> borderWidthBottom;

extern const Argument<Tag::PropArg<decltype(Widget::marginLeft)>> marginLeft;
extern const Argument<Tag::PropArg<decltype(Widget::marginTop)>> marginTop;
extern const Argument<Tag::PropArg<decltype(Widget::marginRight)>> marginRight;
extern const Argument<Tag::PropArg<decltype(Widget::marginBottom)>> marginBottom;

extern const Argument<Tag::PropArg<decltype(Widget::paddingLeft)>> paddingLeft;
extern const Argument<Tag::PropArg<decltype(Widget::paddingTop)>> paddingTop;
extern const Argument<Tag::PropArg<decltype(Widget::paddingRight)>> paddingRight;
extern const Argument<Tag::PropArg<decltype(Widget::paddingBottom)>> paddingBottom;

extern const Argument<Tag::PropArg<decltype(Widget::width)>> width;
extern const Argument<Tag::PropArg<decltype(Widget::height)>> height;
extern const Argument<Tag::PropArg<decltype(Widget::maxWidth)>> maxWidth;
extern const Argument<Tag::PropArg<decltype(Widget::maxHeight)>> maxHeight;
extern const Argument<Tag::PropArg<decltype(Widget::minWidth)>> minWidth;
extern const Argument<Tag::PropArg<decltype(Widget::minHeight)>> minHeight;

extern const Argument<Tag::PropArg<decltype(Widget::gapColumn)>> gapColumn;
extern const Argument<Tag::PropArg<decltype(Widget::gapRow)>> gapRow;

extern const Argument<Tag::PropArg<decltype(Widget::disabled)>> disabled;

extern const Argument<Tag::PropArg<decltype(Widget::fontFeatures)>> fontFeatures;

extern const Argument<Tag::PropArg<decltype(Widget::scrollBarColor)>> scrollBarColor;
extern const Argument<Tag::PropArg<decltype(Widget::scrollBarThickness)>> scrollBarThickness;
extern const Argument<Tag::PropArg<decltype(Widget::scrollBarRadius)>> scrollBarRadius;

} // namespace Arg

namespace Tag {
template <std::derived_from<Widget> WidgetType, Internal::FixedString Name>
struct WithRole {
    using Type = RC<WidgetType>;
};
} // namespace Tag

template <std::derived_from<Widget> WidgetType, Internal::FixedString Name>
struct Argument<Tag::WithRole<WidgetType, Name>> {
    using ValueType = typename Tag::WithRole<WidgetType, Name>::Type;

    constexpr ArgVal<Tag::WithRole<WidgetType, Name>> operator=(WidgetType* value) const {
        value->role.set(Name.string());
        return { RC<WidgetType>(value) };
    }

    ArgVal<Tag::WithRole<WidgetType, Name>> operator=(RC<WidgetType> value) const {
        value->role.set(Name.string());
        return { std::move(value) };
    }

    static WidgetType* matchesType(Widget* widget) {
        if (WidgetType* typed = dynamic_cast<WidgetType*>(widget)) {
            typed->role.set(Name.string());
            return typed;
        }
        return nullptr;
    }

    static WidgetType* matchesRole(Widget* widget) {
        if (WidgetType* typed = dynamic_cast<WidgetType*>(widget);
            typed && typed->role.get() == Name.string()) {
            return typed;
        }
        return nullptr;
    }

    static RC<WidgetType> get(const Widget* parent) {
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
