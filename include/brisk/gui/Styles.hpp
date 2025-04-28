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
#include "GUI.hpp"
#include <bit>
#include <brisk/core/internal/cityhash.hpp>

namespace Brisk {

template <PropertyTag Tag>
struct TagWithState : Tag::PropertyTag {
    using Type       = typename Tag::Type;

    using ExtraTypes = typename Tag::ExtraTypes;
};

template <PropertyTag Tag, typename Type>
struct ArgVal<TagWithState<Tag>, Type> {
    using ValueType = Type;
    ValueType value;
    WidgetState state;
};

template <PropertyTag Tag>
struct Argument<TagWithState<Tag>> : Tag {
    using ValueType = typename Tag::Type;

    constexpr ArgVal<TagWithState<Tag>> operator=(ValueType value) const {
        return { std::move(value), state };
    }

    template <MatchesExtraTypes<Tag> U>
    constexpr ArgVal<TagWithState<Tag>, U> operator=(U value) const {
        return { std::move(value), state };
    }

    WidgetState state;
};

template <PropertyTag Tag>
Argument<TagWithState<Tag>> operator|(const Argument<Tag>& arg, WidgetState state) {
    return { .state = state };
}

template <PropertyTag Tag>
Argument<TagWithState<Tag>> operator|(const Argument<TagWithState<Tag>>& arg, WidgetState state) {
    return { .state = arg.state | state };
}

namespace Internal {

template <typename T>
using StyleFunction = function<T(Widget*)>;

using StyleValuePtr = RC<void>;

enum class RuleOp : uint8_t {
    Value,
    Function,
    Inherit,
};

struct StyleProperty {
    template <typename Type>
    static bool get(Type& value, RuleOp op, const RC<void>& rule, Widget* widget) {
        switch (op) {
        default:
            return false;
        case RuleOp::Value:
            value = *reinterpret_cast<const Type*>(rule.get());
            return true;
        case RuleOp::Function:
            if (!widget)
                return false;
            StyleFunction<Type> fn = *reinterpret_cast<const StyleFunction<Type>*>(rule.get());
            value                  = fn(widget);
            return true;
        }
    }

    template <typename Tag>
    StyleProperty(TypeID<Tag>)
        requires PropertyTag<Tag> || StyleVarTag<Tag>
    {
        using Type = typename Tag::Type;
        name       = Tag::name();
        apply      = [](RuleOp op, const StyleValuePtr& rule, Widget* widget, WidgetState state) {
            WidgetState widgetState = widget->state();
            if (widgetState && WidgetState::ForcePressed) {
                widgetState |= WidgetState::Pressed;
            }
            if ((widgetState & state) == state) {
                if constexpr (PropertyTag<Tag>) {
                    if constexpr (MatchesExtraTypes<Inherit, Tag>) {
                        if (op == RuleOp::Inherit) {
                            applier(widget, ArgVal<Tag, Inherit>{ inherit });
                            return;
                        }
                    }
                }
                Type value;
                get(value, op, rule, widget);
                applier(widget, ArgVal<Tag>{ value });
            }
        };
        toString = [](RuleOp op, const StyleValuePtr& rule) -> std::string {
            if (op == RuleOp::Inherit) {
                return "(inherit)";
            } else if (op == RuleOp::Function) {
                return "(dynamic)";
            }
            if constexpr (fmt::is_formattable<Type>::value) {
                Type value;
                get(value, op, rule, nullptr);
                return fmt::to_string(value);
            } else {
                return "(unknown)";
            }
        };
        equals = [](RuleOp op1, const StyleValuePtr& rule1, RuleOp op2, const StyleValuePtr& rule2) -> bool {
            if (op1 == RuleOp::Inherit && op2 == RuleOp::Inherit)
                return true;
            if (op1 == RuleOp::Inherit || op2 == RuleOp::Inherit)
                return false;
            if (op1 == RuleOp::Function || op2 == RuleOp::Function)
                return false;
            Type value1;
            Type value2;
            get(value1, op1, rule1, nullptr);
            get(value2, op2, rule2, nullptr);
            return value1 == value2;
        };
    }

    using fn_apply = void (*)(RuleOp, const StyleValuePtr&, Widget*, WidgetState);
    std::string_view name;
    fn_apply apply;
    using fn_toString = std::string (*)(RuleOp, const StyleValuePtr&);
    fn_toString toString;
    using fn_equals = bool (*)(RuleOp, const StyleValuePtr&, RuleOp, const StyleValuePtr&);
    fn_equals equals;
};

template <typename Tag>
const StyleProperty* styleProperty() {
    static const StyleProperty& instance{ TypeID<Tag>{} };
    return &instance;
}

} // namespace Internal

struct Rules;

struct Rule {
    template <typename Tag, typename U>
    Rule(ArgVal<Tag, U> value) {
        m_property = Internal::styleProperty<Tag>();
        if constexpr (std::is_same_v<U, Inherit>) {
            m_op = Internal::RuleOp::Inherit;
        } else {
            m_op       = Internal::RuleOp::Value;
            using Type = typename Tag::Type;
            m_storage  = std::make_shared<Type>(static_cast<Type>(std::move(value.value)));
        }
        m_state = WidgetState::None;
    }

    template <typename Tag, typename U>
    Rule(ArgVal<TagWithState<Tag>, U> value) : Rule(ArgVal<Tag, U>{ std::move(value.value) }) {
        m_state = value.state;
    }

    template <typename Tag, typename Fn>
    Rule(ArgVal<Tag, Fn> value)
        requires std::is_invocable_r_v<typename Tag::Type, Fn> ||
                 std::is_invocable_r_v<typename Tag::Type, Fn, Widget*>
    {
        m_property = Internal::styleProperty<Tag>();
        using Type = typename Tag::Type;
        m_op       = Internal::RuleOp::Function;
        Internal::StyleFunction<Type> fn;
        if constexpr (std::is_invocable_r_v<typename Tag::Type, Fn>) {
            fn = [fn = std::move(value.value)](Widget*) -> Type {
                return fn();
            };
        } else {
            fn = function<Type(Widget*)>(std::move(value.value));
        }
        m_storage = std::make_shared<Internal::StyleFunction<Type>>(std::move(fn));
        m_state   = WidgetState::None;
    }

    template <typename Tag, typename Fn>
    Rule(ArgVal<TagWithState<Tag>, Fn> value)
        requires std::is_invocable_r_v<typename Tag::Type, Fn> ||
                 std::is_invocable_r_v<typename Tag::Type, Fn, Widget*>
        : Rule(ArgVal<Tag, Fn>{ std::move(value.value) }) {
        m_state = value.state;
    }

    void applyTo(Widget* widget) const {
        m_property->apply(m_op, m_storage, widget, m_state);
    }

    std::string toString() const {
        if (m_state == WidgetState::None)
            return fmt::format("{}: {}", m_property->name, m_property->toString(m_op, m_storage));
        return fmt::format("{} | {}: {}", m_property->name, m_state, m_property->toString(m_op, m_storage));
    }

    // uniquely identifies the property
    const void* id() const noexcept {
        return m_property;
    }

    std::string_view name() const noexcept {
        return m_property->name;
    }

    WidgetState state() const noexcept {
        return m_state;
    }

    bool operator==(const Rule& other) const noexcept {
        return m_property == other.m_property &&
               m_property->equals(m_op, m_storage, other.m_op, other.m_storage) && m_state == other.m_state &&
               m_op == other.m_op;
    }

private:
    friend struct Rules;
    const Internal::StyleProperty* m_property;
    Internal::StyleValuePtr m_storage;
    WidgetState m_state;
    Internal::RuleOp m_op;
};

template <typename Enum>
std::underlying_type_t<Enum> to_underlying(Enum e)
    requires std::is_enum_v<Enum>
{
    return static_cast<std::underlying_type_t<Enum>>(e);
}

struct RuleCmpLess {
    bool operator()(const Rule& x, const Rule& y) {
        if (x.name() < y.name())
            return true;
        if (x.name() > y.name())
            return false;
        int xbits = std::popcount(to_underlying(x.state()));
        int ybits = std::popcount(to_underlying(y.state()));
        if (xbits < ybits)
            return true;
        if (xbits > ybits)
            return false;
        return x.state() < y.state();
    }
};

struct RuleCmpEq {
    bool operator()(const Rule& x, const Rule& y) {
        return x.id() == y.id() && x.state() == y.state();
    }
};

enum class MatchFlags {
    None   = 0,
    IsRoot = 1,
};
template <>
constexpr inline bool isBitFlags<MatchFlags> = true;

template <Argument arg>
constexpr inline Internal::Fn1Type<typename decltype(arg)::ValueType> styleVar = [](Widget* w) ->
    typename decltype(arg)::ValueType {
        return w->getStyleVar<typename decltype(arg)::ValueType>(decltype(arg)::id,
                                                                 typename decltype(arg)::ValueType{});
    };

template <typename Fn>
inline auto adjustColor(Fn&& fn, float lightnessOffset, float chromaMultiplier = 1.f) {
    return [fn = std::move(fn), lightnessOffset, chromaMultiplier](Widget* w) {
        return fn(w).adjust(lightnessOffset, chromaMultiplier);
    };
}

template <typename Fn>
inline auto transparency(Fn&& fn, float alpha) {
    return [fn = std::move(fn), alpha](Widget* w) {
        return fn(w).multiplyAlpha(alpha);
    };
}

template <typename Fn>
inline auto scaleValue(Fn&& fn, float scale) {
    return [fn = std::move(fn), scale](Widget* w) {
        return fn(w) * scale;
    };
}

/**
 * @brief Computes the contrast ratio between two colors.
 *
 * This function calculates the contrast ratio between a foreground and a background color,
 * based on their lightness values. The contrast ratio is used to determine the readability
 * and accessibility of text on a background.
 *
 * The formula used is: (L1 + 0.05) / (L2 + 0.05), where L1 is the lightness of the lighter color
 * and L2 is the lightness of the darker color.
 *
 * @param foreground The foreground color.
 * @param background The background color.
 * @return The contrast ratio between the two colors.
 */
inline float contrastRatio(ColorF foreground, ColorF background) {
    float L1 = foreground.lightness();
    float L2 = background.lightness();
    if (!linearColor) {
        L1 = Internal::srgbGammaToLinear(L1);
        L2 = Internal::srgbGammaToLinear(L2);
    }
    if (L1 < L2)
        std::swap(L1, L2);
    return (L1 + 0.05f) / (L2 + 0.05f);
}

/**
 * @brief Creates a function that returns the appropriate text color based on contrast.
 *
 * This function returns a lambda that, when given a `Widget` pointer, computes the contrast
 * ratio between the given colors and the result of a provided function `fn`. The lambda
 * returns the primary color if it has a higher contrast ratio to the computed color,
 * and the secondary color otherwise.
 *
 * @tparam Fn The type of the function provided.
 * @param fn The function that returns a `ColorW` based on a `Widget`.
 * @param primary The primary text color (default is white).
 * @param secondary The secondary text color (default is black).
 * @return A lambda that takes a `Widget*` and returns a `ColorW` (either primary or secondary color).
 */
template <typename Fn>
inline auto textColorFor(Fn&& fn, ColorW primary = Palette::white, ColorW secondary = Palette::black) {
    return [fn = std::move(fn), primary, secondary](Widget* w) {
        ColorW c = fn(w);
        float c1 = contrastRatio(primary, c);
        float c2 = contrastRatio(secondary, c);
        if (c1 > c2)
            return primary;
        return secondary;
    };
}

namespace Selectors {

template <typename Sel>
concept Selector = requires(const Sel sel, Widget* w, MatchFlags f) {
    { sel.matches(w, f) } noexcept -> std::same_as<bool>;
};

// *
struct Universal {
    bool matches(Widget*, MatchFlags) const noexcept {
        return true;
    }
};

// :root
struct Root {
    bool matches(Widget*, MatchFlags flags) const noexcept {
        return flags && MatchFlags::IsRoot;
    }
};

// type
struct State {
    explicit State(WidgetState state) noexcept : state(state) {}

    WidgetState state;

    bool matches(Widget* widget, MatchFlags) const noexcept {
        return (widget->state() & state) == state;
    }
};

// type
struct Type {
    explicit Type(std::string_view type) noexcept : type(type) {}

    std::string type;

    bool matches(Widget* widget, MatchFlags) const noexcept {
        return widget->type() == type;
    }
};

// menu
struct IsMenu {
    bool matches(Widget* w, MatchFlags flags) const noexcept {
        return w->isMenu();
    }
};

// role
struct Role {
    explicit Role(std::string_view role) noexcept : role(role) {}

    std::string role;

    bool matches(Widget* widget, MatchFlags) const noexcept {
        return widget->role.get() == role;
    }
};

// #id
struct Id {
    explicit Id(std::string_view id) noexcept : id(id) {}

    std::string id;

    bool matches(Widget* widget, MatchFlags) const noexcept {
        return widget->id.get() == id;
    }
};

// .class
struct Class {
    explicit Class(std::string_view className) noexcept : className(className) {}

    std::string className;

    bool matches(Widget* widget, MatchFlags) const noexcept {
        if (std::find(widget->classes.get().begin(), widget->classes.get().end(), className) ==
            widget->classes.get().end())
            return false;

        return true;
    }
};

// Parent > *
template <Selector Sel>
struct Parent {
    Parent(Sel&& selector) noexcept : selector(std::move(selector)) {}

    Sel selector;

    bool matches(Widget* widget, MatchFlags flags) const noexcept {
        return widget->parent() && selector.matches(widget->parent(), MatchFlags::None);
    }
};

template <Selector... Selectors>
struct All {
    All(Selectors&&... selectors) noexcept : selectors(std::move(selectors)...) {}

    std::tuple<Selectors...> selectors;

    bool matches(Widget* widget, MatchFlags flags) const noexcept {
        return matchesInternal(std::index_sequence_for<Selectors...>{}, widget, flags);
    }

private:
    template <size_t... Idx>
    bool matchesInternal(std::index_sequence<Idx...>, Widget* widget, MatchFlags flags) const noexcept {
        return (std::get<Idx>(selectors).matches(widget, flags) && ...);
    }
};

template <>
struct All<> {
    // illformed
};

template <Selector... Selectors>
struct Any {
    Any(Selectors&&... selectors) noexcept : selectors(std::move(selectors)...) {}

    std::tuple<Selectors...> selectors;

    bool matches(Widget* widget, MatchFlags flags) const noexcept {
        return matchesInternal(std::index_sequence_for<Selectors...>{}, widget, flags);
    }

private:
    template <size_t... Idx>
    bool matchesInternal(std::index_sequence<Idx...>, Widget* widget, MatchFlags flags) const noexcept {
        return (std::get<Idx>(selectors).matches(widget, flags) || ...);
    }
};

template <>
struct Any<> {
    // illformed
};

template <Selector Sel>
struct Not {
    Not(Sel&& sel) noexcept : sel(std::move(sel)) {}

    Sel sel;

    bool matches(Widget* widget, MatchFlags flags) const noexcept {
        return !sel.matches(widget, flags);
    }
};

struct Nth {
    Nth(int index, int modulo = INT_MAX, bool reverse = false) noexcept
        : index(index), modulo(modulo), reverse(reverse) {}

    int index;
    int modulo;
    bool reverse;

    bool matches(Widget* widget, MatchFlags) const noexcept {
        if (!widget->parent())
            return false;
        std::optional<size_t> childIndex = widget->parent()->indexOf(widget);
        if (!childIndex)
            return false;
        if (reverse)
            *childIndex = widget->parent()->widgets().size() - 1 - *childIndex;
        if (modulo != INT_MAX)
            *childIndex %= modulo;
        return *childIndex == index;
    }
};

struct NthLast : public Nth {
    NthLast(int index, int modulo = INT_MAX) noexcept : Nth(index, modulo, true) {}
};

struct First : public Nth {
    First() noexcept : Nth(0) {}
};

struct Last : public Nth {
    Last() noexcept : Nth(0, INT_MAX, true) {}
};

template <Selector... Selectors>
inline All<std::remove_cvref_t<Selectors>...> all(Selectors&&... selectors) {
    return { std::forward<Selectors>(selectors)... };
}

template <Selector Selector1, Selector Selector>
inline All<std::remove_cvref_t<Selector1>, std::remove_cvref_t<Selector>> operator&&(Selector1&& x,
                                                                                     Selector&& y) {
    return { std::forward<Selector1>(x), std::forward<Selector>(y) };
}

template <Selector Selector1, Selector Selector>
inline All<Parent<std::remove_cvref_t<Selector1>>, std::remove_cvref_t<Selector>> operator>(Selector1&& x,
                                                                                            Selector&& y) {
    return { { std::forward<Selector1>(x) }, std::forward<Selector>(y) };
}

template <Selector... Selectors>
inline Any<std::remove_cvref_t<Selectors>...> any(Selectors&&... selectors) {
    return { std::forward<Selectors>(selectors)... };
}

template <Selector Selector1, Selector Selector>
inline Any<std::remove_cvref_t<Selector1>, std::remove_cvref_t<Selector>> operator||(Selector1&& x,
                                                                                     Selector&& y) {
    return { std::forward<Selector1>(x), std::forward<Selector>(y) };
}

template <Selector Selector1>
inline Not<std::remove_cvref_t<Selector1>> operator!(Selector1&& x) {
    return { std::forward<Selector1>(x) };
}

} // namespace Selectors

struct Selector {
    template <Selectors::Selector Sel>
    Selector(Sel&& sel) : sel(std::make_shared<std::remove_cvref_t<Sel>>(std::move(sel))) {
        match = [](const void* p, Widget* w, MatchFlags flags) {
            return reinterpret_cast<const std::remove_cvref_t<Sel>*>(p)->matches(w, flags);
        };
    }

    bool matches(Widget* widget, MatchFlags flags) const noexcept {
        return match(sel.get(), widget, flags);
    }

private:
    std::shared_ptr<void> sel;
    using fn_match = bool (*)(const void*, Widget*, MatchFlags);
    fn_match match;
};

struct Rules {
    Rules() = default;
    Rules(std::initializer_list<Rule> rules);
    Rules(std::vector<Rule> rules, bool doSort = false);
    void sort();
    // both must be sorted
    // `other` overrides rules with same id in `this`
    Rules& merge(const Rules& other);

    void applyTo(Widget* widget) const;
    std::vector<Rule> rules;

    template <PropertyTag Tag>
    std::optional<typename Tag::Type> get(Tag) const {
        const Internal::StyleProperty* prop = Internal::styleProperty<Tag>();
        for (const Rule& rule : rules) {
            if (rule.m_property == prop) {
                if (rule.m_op == Internal::RuleOp::Function) {
                    return std::nullopt;
                }
                typename Tag::Type value;
                prop->get(value, rule.m_op, rule.m_storage, nullptr);
                return value;
            }
        }
        return std::nullopt;
    }

    bool operator==(const Rules&) const noexcept = default;
    bool operator!=(const Rules&) const noexcept = default;
};

struct Style {
    Selector selector;
    Rules rules;
};

class Stylesheet : public std::vector<Style> {
public:
    template <std::same_as<Style>... Args>
    Stylesheet(Args... args) : Stylesheet(std::vector<RC<const Stylesheet>>{}, std::move(args)...) {}

    template <std::same_as<Style>... Args>
    Stylesheet(RC<const Stylesheet> inheritFrom, Args... args)
        : std::vector<Style>(std::initializer_list<Style>{ args... }), inherited{ std::move(inheritFrom) } {}

    template <std::same_as<Style>... Args>
    Stylesheet(std::vector<RC<const Stylesheet>> inheritFrom, Args... args)
        : std::vector<Style>(std::initializer_list<Style>{ args... }), inherited(std::move(inheritFrom)) {}

    std::vector<RC<const Stylesheet>> inherited;

    void stylize(Widget* widget, bool isRoot) const;

private:
    void stylizeInternal(Rules& rules, Widget* widget, bool isRoot) const;
};

template <typename T, uint64_t Id>
struct StyleVariableTag : Tag::StyleVarTag {
    using Type                   = T;
    constexpr static uint64_t id = Id;

    static std::string_view name() noexcept {
        return "styleVar";
    }
};

constexpr inline Argument<StyleVariableTag<ColorW, "windowColor"_hash>> windowColor{};
constexpr inline Argument<StyleVariableTag<ColorW, "selectedColor"_hash>> selectedColor{};
constexpr inline Argument<StyleVariableTag<float, "animationSpeed"_hash>> animationSpeed{};
constexpr inline Argument<StyleVariableTag<ColorW, "focusFrameColor"_hash>> focusFrameColor{};
constexpr inline Argument<StyleVariableTag<ColorW, "hintBackgroundColor"_hash>> hintBackgroundColor{};
constexpr inline Argument<StyleVariableTag<ColorW, "hintTextColor"_hash>> hintTextColor{};
constexpr inline Argument<StyleVariableTag<ColorW, "hintShadowColor"_hash>> hintShadowColor{};

} // namespace Brisk

template <>
struct fmt::formatter<Brisk::Rule> : fmt::formatter<std::string> {
    template <typename FormatContext>
    auto format(const Brisk::Rule& val, FormatContext& ctx) const {
        return fmt::format_to(ctx.out(), "{}", val.toString());
    }
};

template <>
struct fmt::formatter<Brisk::Rules> : fmt::formatter<std::string> {
    template <typename FormatContext>
    auto format(const Brisk::Rules& val, FormatContext& ctx) const {
        return fmt::format_to(ctx.out(), "{}", fmt::join(val.rules, "; "));
    }
};
