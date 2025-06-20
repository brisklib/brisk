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
#include <brisk/core/Memory.hpp>
#include <brisk/core/Rc.hpp>
#include <brisk/core/BasicTypes.hpp>
#include <brisk/core/Utilities.hpp>
#include <brisk/core/internal/SmallVector.hpp>
#include <brisk/core/Log.hpp>
#include <brisk/core/Threading.hpp>
#include <brisk/core/internal/FunctionRef.hpp>
#include <brisk/core/internal/tuplet/Tuplet.hpp>

namespace Brisk {

template <typename... Args>
using Callback = function<void(Args...)>;

template <typename... Args>
struct Callbacks : public std::vector<Callback<Args...>> {
    Callbacks& operator+=(Callback<Args...> cb) {
        BRISK_ASSERT(cb);
        std::vector<Callback<Args...>>::push_back(std::move(cb));
        return *this;
    }

    void operator()(Args... args) const {
        for (const Callback<Args...>& cb : *this) {
            cb(args...);
        }
    }
};

#if 0
#define LOG_BINDING LOG_DEBUG
#else
#define LOG_BINDING LOG_NOP
#endif

struct BindingAddress {
    const void* address;
    size_t size;

    const uint8_t* min() const noexcept {
        return reinterpret_cast<const uint8_t*>(address);
    }

    const uint8_t* max() const noexcept {
        return reinterpret_cast<const uint8_t*>(address) + size;
    }

    Range<const uint8_t*> range() const noexcept {
        return { min(), max() };
    }

    constexpr bool operator==(const BindingAddress&) const noexcept = default;

    friend BindingAddress mergeAddresses(std::convertible_to<BindingAddress> auto... addresses) {
        size_t sum         = (addresses->size + ...);
        const uint8_t* min = std::min({ addresses->min()... });
        const uint8_t* max = std::max({ addresses->max()... });
        if (max - min == sum) {
            return { min, sum };
        }
        return { nullptr, 0 };
    }
};

/**
 * @brief Converts a pointer of type T to a BindingAddress.
 *
 * This function creates a BindingAddress that encompasses the memory range
 * of the specified pointer, allowing for easier management of variable
 * bindings in a property-like system.
 *
 * @tparam T The type of the value being pointed to.
 * @param value A pointer to the value.
 * @return A BindingAddress representing the memory range of the value.
 */
template <typename T>
constexpr BindingAddress toBindingAddress(const T* value) noexcept {
    if constexpr (std::is_same_v<T, void>) {
        return BindingAddress{ value, 1 };
    } else {
        return BindingAddress{ static_cast<const void*>(value), sizeof(T) };
    }
}

/**
 * @brief A special object for static binding.
 */
inline const Empty staticBinding{};

/**
 * @brief The BindingAddress for the static binding object.
 */
inline const constinit BindingAddress staticBindingAddress = toBindingAddress(&staticBinding);

/**
 * @brief A collection of BindingAddresses.
 */
using BindingAddresses                                     = SmallVector<BindingAddress, 1>;

/**
 * @brief Generic Value structure for property management.
 *
 * The Value class allows for both read and write access to properties,
 * enabling a more dynamic approach to property handling.
 *
 * @tparam T The type of the value contained within the Value.
 */
template <typename T>
struct Value;

namespace Internal {
template <typename... Args>
struct TriggerArgs {
    using Type = std::tuple<Args...>;
};

template <>
struct TriggerArgs<> {
    using Type = Empty;
};

template <typename Arg>
struct TriggerArgs<Arg> {
    using Type = Arg;
};
} // namespace Internal

template <typename... Args>
struct Trigger {
    using Type                      = typename Internal::TriggerArgs<Args...>::Type;

    constexpr static bool isTrigger = true;

    std::optional<Type> arg;

    operator Type() const noexcept {
        BRISK_ASSERT(arg.has_value());
        return *arg;
    }

    constexpr Trigger() noexcept = default;

    constexpr bool operator==(const Trigger& other) const noexcept {
        return false;
    }

    int trigger(Args... args);
};

namespace Internal {

template <typename T>
constexpr inline bool isTrigger = false;

template <typename... Args>
constexpr inline bool isTrigger<Trigger<Args...>> = true;

template <typename T>
struct ValueArgumentImpl {
    using Type = T;
};

template <typename... Args>
struct ValueArgumentImpl<Trigger<Args...>> {
    using Type = typename Trigger<Args...>::Type;
};

} // namespace Internal

template <typename T>
using ValueArgument = typename Internal::ValueArgumentImpl<T>::Type;

/**
 * @brief Concept that checks if a type behaves like a property.
 *
 * @tparam T The type to check.
 */
template <typename T>
concept PropertyLike = requires(T t, const T ct, typename T::ValueType v) {
    requires std::copy_constructible<typename T::ValueType>;
    { ct.get() } -> std::convertible_to<typename T::ValueType>;
    t.set(v);
    { ct.address() } -> std::convertible_to<BindingAddress>;
    { t.this_pointer = nullptr };
};

template <typename T>
struct Value;

namespace Internal {

template <PropertyLike Prop, typename Type = typename Prop::ValueType>
Value<Type> asValue(const Prop& prop) noexcept {
    return Value<Type>(
        [prop]() noexcept -> Type {
            return prop.get();
        },
        nullptr, // read-only
        prop.address());
}

template <PropertyLike Prop, typename Type = typename Prop::ValueType>
Value<Type> asValue(Prop& prop) noexcept {
    return Value<Type>(
        [prop]() noexcept -> Type {
            return prop.get();
        },
        [prop](Type value) mutable {
            prop.set(std::move(value));
        },
        prop.address());
}

template <typename T>
constexpr inline bool isValue = false;

template <typename T>
constexpr inline bool isValue<Value<T>> = true;

} // namespace Internal

template <typename T>
concept AtomicCompatible = std::is_trivially_copyable_v<T> && std::is_copy_assignable_v<T>;

template <typename... T, std::invocable<T...> Fn>
Value<std::invoke_result_t<Fn, T...>> transform(Fn&& fn, const Value<T>&... values) noexcept;

/**
 * @brief Value class that manages a value with getter and setter functionality.
 *
 * This class encapsulates the value and provides mechanisms to manipulate it,
 * including transformation functions.
 *
 * @tparam T The type of the value being managed.
 */
template <typename T>
struct Value {
    using GetFn                     = function<T()>;
    using SetFn                     = function<void(T)>;
    using NotifyFn                  = function<void()>;

    constexpr static bool isTrigger = Internal::isTrigger<T>;

    using ValueType                 = T;

    using AtomicType                = std::conditional_t<AtomicCompatible<T>, std::atomic<T>, T>;

    /**
     * @brief Default constructor.
     */
    constexpr Value() noexcept : m_get{}, m_set{}, m_srcAddresses{}, m_destAddress{} {}

    /**
     * @brief Constructs a Value from a PropertyLike type.
     *
     * @tparam U The property type.
     * @param property Pointer to the property.
     */
    template <PropertyLike U>
    [[nodiscard]] explicit Value(U* property) noexcept : Value(Internal::asValue(*property)) {}

    template <PropertyLike U>
    [[nodiscard]] explicit Value(const U* property) noexcept : Value(Internal::asValue(*property)) {}

    [[nodiscard]] explicit Value(T* value) noexcept : Value(variable(value)) {}

    [[nodiscard]] explicit Value(T* value, NotifyFn notify) noexcept
        : Value(variable(value, std::move(notify))) {}

    template <typename NotifyClass>
    [[nodiscard]] explicit Value(T* value, NotifyClass* self, void (NotifyClass::*notify)()) noexcept
        : Value(variable(value, [self, notify]() {
              (self->*notify)();
          })) {}

    [[nodiscard]] explicit Value(AtomicType* value) noexcept
        requires AtomicCompatible<T>
        : Value(variable(value)) {}

    [[nodiscard]] explicit Value(AtomicType* value, NotifyFn notify) noexcept
        requires AtomicCompatible<T>
        : Value(variable(value, std::move(notify))) {}

    template <typename NotifyClass>
    [[nodiscard]] explicit Value(AtomicType* value, NotifyClass* self, void (NotifyClass::*notify)()) noexcept
        requires AtomicCompatible<T>
        : Value(variable(value, [self, notify]() {
              (self->*notify)();
          })) {}

    template <typename U>
    Value<U> explicitConversion() && noexcept {
        if constexpr (requires(T t, U u) {
                          static_cast<T>(u);
                          static_cast<U>(t);
                      }) {
            return std::move(*this).transform(
                [](T value) -> U {
                    return static_cast<U>(value);
                },
                [](U value) -> T {
                    return static_cast<T>(value);
                });
        } else {
            return std::move(*this).transform([](T value) -> U {
                return static_cast<U>(value);
            });
        }
    }

    template <typename U>
    Value(Value<U> other) noexcept
        requires std::is_convertible_v<U, T>
        : Value(std::move(other).template implicitConversion<T>()) {}

    template <typename U>
    explicit Value(Value<U> other) noexcept
        requires(!std::is_convertible_v<U, T>) && requires(U u) { static_cast<T>(u); }
        : Value(std::move(other).template explicitConversion<T>()) {}

    /**
     * @brief Checks whether the value is empty
     */
    bool empty() const noexcept {
        return !m_get && !m_set;
    }

    /**
     * @brief Creates a Value that holds a constant value.
     *
     * @param constant The constant value to hold.
     * @return A Value representing the constant.
     */
    [[nodiscard]] static Value constant(T constant) noexcept {
        return Value{
            [constant]() -> T {
                return constant;
            },
            nullptr, // no-op
            {},
            {},
        };
    }

    /**
     * @brief Returns a read-only version of this Value.
     *
     * @return A read-only Value instance.
     */
    [[nodiscard]] Value readOnly() && noexcept {
        return Value{
            std::move(m_get),
            nullptr, // no-op
            std::move(m_srcAddresses),
            std::move(m_destAddress),
        };
    }

    /**
     * @brief Returns read-only version of Value
     */
    [[nodiscard]] Value readOnly() const& noexcept {
        Value(*this).readOnly();
    }

    /**
     * @brief Returns mutable Value that initially holds the given value.
     *
     * @param initialValue value
     */
    [[nodiscard]] static Value mutableValue(T initialValue);

    [[nodiscard]] static Value computed(function<T()> func) noexcept {
        return Value{
            std::move(func),
            nullptr, // no-op
            nullptr,
        };
    }

    [[nodiscard]] static Value listener(Callback<T> callback, BindingAddress range) noexcept {
        return Value{
            nullptr,
            [callback = std::move(callback)](T newValue) {
                callback(std::move(newValue));
            },
            { range },
            range,
        };
    }

    [[nodiscard]] static Value listener(Callback<> callback, BindingAddress range) noexcept {
        return Value{
            nullptr,
            [callback = std::move(callback)](T newValue) {
                callback();
            },
            { range },
            range,
        };
    }

    [[nodiscard]] static Value variable(T* pvalue) noexcept;

    [[nodiscard]] static Value variable(T* pvalue, NotifyFn notify) noexcept;

    [[nodiscard]] static Value variable(AtomicType* pvalue) noexcept
        requires AtomicCompatible<T>;

    [[nodiscard]] static Value variable(AtomicType* pvalue, NotifyFn notify) noexcept
        requires AtomicCompatible<T>;

    [[nodiscard]] bool isWritable() const noexcept {
        return !m_set.empty();
    }

    [[nodiscard]] bool isReadable() const noexcept {
        return !m_get.empty();
    }

    [[nodiscard]] bool hasAddress() const noexcept {
        return !m_srcAddresses.empty();
    }

    [[nodiscard]] T get() const {
        BRISK_ASSERT(!m_get.empty());
        return m_get();
    }

    void set(T newValue) const {
        if (m_set) [[likely]]
            m_set(std::move(newValue));
    }

    template <std::invocable<T> Forward, typename U = std::invoke_result_t<Forward, T>,
              std::invocable<U> Backward>
    Value<U> transform(Forward&& forward, Backward&& backward) && noexcept {
        return Value<U>{
            [forward = std::move(forward), get = std::move(m_get)]() -> U {
                return forward(get());
            },
            [backward = std::move(backward), set = std::move(m_set)](U newValue) {
                if (set)
                    set(backward(std::move(newValue)));
            },
            std::move(m_srcAddresses),
            std::move(m_destAddress),
        };
    }

    template <std::invocable<T> Forward, typename U = std::invoke_result_t<Forward, T>,
              std::invocable<T, U> Backward>
    Value<U> transform(Forward&& forward, Backward&& backward) && noexcept {
        return Value<U>{
            [forward = std::move(forward), get = m_get]() -> U {
                return forward(get());
            },
            [backward = std::move(backward), get = m_get, set = std::move(m_set)](U newValue) {
                if (set)
                    set(backward(get(), std::move(newValue)));
            },
            std::move(m_srcAddresses),
            std::move(m_destAddress),
        };
    }

    template <std::invocable<T> Forward, typename U = std::invoke_result_t<Forward, T>>
    Value<U> transform(Forward&& forward) && noexcept {
        return Value<U>{
            [forward = std::move(forward), get = std::move(m_get)]() -> U {
                return forward(get());
            },
            nullptr,
            std::move(m_srcAddresses),
            std::move(m_destAddress),
        };
    }

    template <std::invocable<T> Forward, typename U = std::invoke_result_t<Forward, T>,
              std::invocable<typename U::value_type> Backward>
    Value<U> transform(Forward&& forward, Backward&& backward) && noexcept
        requires(!IsOptional<T>) && IsOptional<U> &&
                IsOptional<std::invoke_result_t<Backward, typename U::value_type>>
    {
        return Value<U>{
            [forward = std::move(forward), get = std::move(m_get)]() -> U {
                return forward(get());
            },
            [backward = std::move(backward), set = std::move(m_set)](U newValue) {
                if (set && newValue.has_value())
                    if (auto val = backward(std::move(*newValue)))
                        set(*val);
            },
            std::move(m_srcAddresses),
            std::move(m_destAddress),
        };
    }

    template <std::invocable<T> Forward, typename U = std::invoke_result_t<Forward, T>,
              std::invocable<U> Backward>
    Value<U> transform(Forward&& forward, Backward&& backward) const& noexcept {
        return Value<T>{ *this }.transform(std::forward<Forward>(forward), std::forward<Backward>(backward));
    }

    template <std::invocable<T> Forward, typename U = std::invoke_result_t<Forward, T>,
              std::invocable<T, U> Backward>
    Value<U> transform(Forward&& forward, Backward&& backward) const& noexcept {
        return Value<T>{ *this }.transform(std::forward<Forward>(forward), std::forward<Backward>(backward));
    }

    template <std::invocable<T> Forward, typename U = std::invoke_result_t<Forward, T>>
    Value<U> transform(Forward&& forward) const& noexcept {
        return Value<T>{ *this }.transform(std::forward<Forward>(forward));
    }

    template <std::invocable<T, T> Fn, typename R = std::invoke_result_t<Fn, T, T>>
    friend Value<R> binary(Value left, Value right, Fn&& fn) noexcept {
        return Value<R>{
            [fn = std::move(fn), leftGet = std::move(left.m_get), rightGet = std::move(right.m_get)]() -> R {
                return fn(leftGet(), rightGet());
            },
            nullptr,
            mergeSmallVectors(std::move(left.m_srcAddresses), std::move(right.m_srcAddresses)),
            std::move(left.m_destAddress),
        };
    }

    template <std::invocable<T, T> Fn, typename R = std::invoke_result_t<Fn, T, T>>
    friend Value<R> binary(Value left, std::type_identity_t<T> right, Fn&& fn) noexcept {
        return Value<R>{
            [fn = std::move(fn), leftGet = std::move(left.m_get), right = std::move(right)]() -> R {
                return fn(leftGet(), right);
            },
            nullptr,
            std::move(left.m_srcAddresses),
            std::move(left.m_destAddress),
        };
    }

    template <std::invocable<T, T> Fn, typename R = std::invoke_result_t<Fn, T, T>>
    friend Value<R> binary(std::type_identity_t<T> left, Value right, Fn&& fn) noexcept {
        return Value<R>{
            [fn = std::move(fn), left = std::move(left), rightGet = std::move(right.m_get)]() -> R {
                return fn(left, rightGet());
            },
            nullptr,
            std::move(right.m_srcAddresses),
            std::move(right.m_destAddress),
        };
    }

    Value<bool> equal(std::type_identity_t<T> compare, bool bidirectional = true) && noexcept {
        return Value<bool>{
            [get = std::move(m_get), compare]() -> bool {
                return get() == compare;
            },
            bidirectional ? Value<bool>::SetFn([set = std::move(m_set), compare](bool newValue) {
                if (newValue)
                    if (set)
                        set(compare);
            })
                          : Value<bool>::SetFn(nullptr),
            std::move(m_srcAddresses),
            std::move(m_destAddress),
        };
    }

    Value<bool> equal(std::type_identity_t<T> compare, bool bidirectional = true) const& noexcept {
        return Value<T>{ *this }.equal(std::move(compare), bidirectional);
    }

    Value<std::optional<T>> makeOptional() && noexcept {
        return Value<std::optional<T>>{
            [get = std::move(m_get)]() -> std::optional<T> {
                return std::optional<T>(get());
            },
            [set = std::move(m_set)](std::optional<T> value) {
                if (value)
                    set(std::move(*value));
            },
            std::move(m_srcAddresses),
            std::move(m_destAddress),
        };
    }

    Value<std::optional<T>> makeOptional() const& noexcept {
        return Value<T>{ *this }.makeOptional();
    }

#define BRISK_BINDING_BINARY_OP(op)                                                                          \
    friend inline auto operator op(Value left, Value right) noexcept                                         \
        requires requires { left.get() op right.get(); }                                                     \
    {                                                                                                        \
        return binary(std::move(left), std::move(right), [](T l, T r) {                                      \
            return l op r;                                                                                   \
        });                                                                                                  \
    }                                                                                                        \
    friend inline auto operator op(Value left, T right) noexcept                                             \
        requires requires { left.get() op right; }                                                           \
    {                                                                                                        \
        return binary(std::move(left), std::move(right), [](T l, T r) {                                      \
            return l op r;                                                                                   \
        });                                                                                                  \
    }                                                                                                        \
    friend inline auto operator op(T left, Value right) noexcept                                             \
        requires requires { left op right.get(); }                                                           \
    {                                                                                                        \
        return binary(std::move(left), std::move(right), [](T l, T r) {                                      \
            return l op r;                                                                                   \
        });                                                                                                  \
    }

#define BRISK_BINDING_PREFIX_OP(op)                                                                          \
    friend auto operator op(Value op1) noexcept                                                              \
        requires requires { op op1.get(); }                                                                  \
    {                                                                                                        \
        return std::move(op1).transform(                                                                     \
            [](T x) -> T {                                                                                   \
                return op x;                                                                                 \
            },                                                                                               \
            [](T x) -> T {                                                                                   \
                return op x;                                                                                 \
            });                                                                                              \
    }

    BRISK_BINDING_BINARY_OP(+)
    BRISK_BINDING_BINARY_OP(-)
    BRISK_BINDING_BINARY_OP(*)
    BRISK_BINDING_BINARY_OP(/)
    BRISK_BINDING_BINARY_OP(%)
    BRISK_BINDING_BINARY_OP(&)
    BRISK_BINDING_BINARY_OP(^)
    BRISK_BINDING_BINARY_OP(|)
    BRISK_BINDING_BINARY_OP(&&)
    BRISK_BINDING_BINARY_OP(||)
    BRISK_BINDING_BINARY_OP(<)
    BRISK_BINDING_BINARY_OP(>)
    BRISK_BINDING_BINARY_OP(<<)
    BRISK_BINDING_BINARY_OP(>>)
    BRISK_BINDING_BINARY_OP(<=)
    BRISK_BINDING_BINARY_OP(>=)
    BRISK_BINDING_BINARY_OP(==)
    BRISK_BINDING_BINARY_OP(!=)

    BRISK_BINDING_PREFIX_OP(-)
    BRISK_BINDING_PREFIX_OP(~)
    BRISK_BINDING_PREFIX_OP(!)

    const GetFn& getter() const& noexcept {
        return m_get;
    }

    GetFn&& getter() && noexcept {
        return m_get;
    }

    const SetFn& setter() const& noexcept {
        return m_set;
    }

    SetFn&& setter() && noexcept {
        return m_set;
    }

    const BindingAddresses& srcAddresses() const noexcept {
        return m_srcAddresses;
    }

    BindingAddresses addresses() const noexcept {
        BindingAddresses result = m_srcAddresses;
        result.push_back(m_destAddress);
        return result;
    }

    [[nodiscard]] explicit Value(GetFn get, SetFn set, BindingAddresses srcAddresses,
                                 BindingAddress destAddress) noexcept
        : m_get(std::move(get)), m_set(std::move(set)), m_srcAddresses(std::move(srcAddresses)),
          m_destAddress(std::move(destAddress)) {}

    [[nodiscard]] explicit Value(GetFn get, SetFn set, BindingAddress address) noexcept
        : m_get(std::move(get)), m_set(std::move(set)), m_srcAddresses{ address }, m_destAddress(address) {}

private:
    template <typename U>
    Value<U> implicitConversion() && noexcept {
        if constexpr (std::is_convertible_v<U, T>) {
            return std::move(*this).transform(
                [](T value) -> U {
                    return static_cast<U>(value);
                },
                [](U value) -> T {
                    return static_cast<T>(value);
                });
        } else {
            return std::move(*this).transform([](T value) -> U {
                return static_cast<U>(value);
            });
        }
    }
    friend class Bindings;

    template <typename U>
    friend struct Value;

    GetFn m_get;
    SetFn m_set;
    BindingAddresses m_srcAddresses;
    BindingAddress m_destAddress;
};

template <typename U>
Value(U*) -> Value<U>;

template <typename U>
Value(std::atomic<U>*) -> Value<U>;

template <typename U>
Value(std::atomic<U>*, function<void()>) -> Value<U>;

template <typename U, typename NotifyClass>
Value(std::atomic<U>*, NotifyClass*, void (NotifyClass::*notify)()) -> Value<U>;

template <PropertyLike U>
Value(U*) -> Value<typename U::Type>;

template <PropertyLike U>
Value(const U*) -> Value<typename U::Type>;

template <typename... T, std::invocable<T...> Fn>
Value<std::invoke_result_t<Fn, T...>> transform(Fn&& fn, const Value<T>&... values) noexcept {
    BindingAddresses addresses;
    (addresses.insert(addresses.end(), values.srcAddresses().begin(), values.srcAddresses().end()), ...);

    return Value<std::invoke_result_t<Fn, T...>>(
        [getterTuple = std::tuple{ values.getter()... }, fn = std::move(fn)]() {
            return std::apply(
                [&fn](const auto&... vals) {
                    return fn(vals()...);
                },
                getterTuple);
        },
        nullptr, std::move(addresses), BindingAddress{});
}

namespace Internal {
template <typename T>
using floatingPointTypeOf = decltype(1.f * std::declval<T>());
}

template <typename T, typename FT = Internal::floatingPointTypeOf<T>>
Value<FT> remap(Value<T> value, std::type_identity_t<FT> min, std::type_identity_t<FT> max,
                std::type_identity_t<FT> curvature = 1) noexcept {
    return value.transform(
        [min, max, curvature](T inValue) -> FT {
            // full range to normalized
            FT value = (static_cast<FT>(inValue) - min) / (max - min);
            if (curvature != 1.f)
                value = std::pow(value, curvature);
            return value;
        },
        [min, max, curvature](FT inValue) -> T {
            // normalized to full range
            FT value = inValue;
            if (curvature != 1.f)
                value = std::pow(value, 1.f / curvature);
            if constexpr (std::is_integral_v<T>) {
                value = std::round(value);
            }
            return value * (max - min) + min;
        });
}

template <typename T, typename FT = Internal::floatingPointTypeOf<T>>
Value<FT> remapLog(Value<T> value, std::type_identity_t<FT> min, std::type_identity_t<FT> max,
                   std::type_identity_t<FT> cut = 0) noexcept {
    return value.transform(
        [min, max, cut](T inValue) -> FT {
            // full range to normalized
            FT value = (std::log10(std::max(static_cast<FT>(inValue), cut)) - std::log10(min)) /
                       (std::log10(max) - std::log10(min));
            return value;
        },
        [min, max, cut](FT inValue) -> T {
            // normalized to full range
            FT v = std::pow(10, inValue * (std::log10(max) - std::log10(min)) + std::log10(min));
            v    = v <= cut ? T(0) : v;
            if constexpr (std::is_integral_v<T>) {
                v = std::round(v);
            }
            return v;
        });
}

/**
 * @brief Converts a Value of any type to a Value of type std::string with optional formatting.
 *
 * @tparam T The type of the input Value.
 * @param value The Value to convert.
 * @param fmtstr Optional format string (default: "{}").
 * @return Value<std::string> The formatted string representation of the input Value.
 *
 * @code
 * int m_num = 42;
 * // Create a `Text` widget that displays the text `"number: 42"` and updates dynamically
 * // when `m_num` changes.
 * rcnew Text{ text = toString(Value<int>{ &m_num }, "number: {}") };
 * @endcode
 */
template <typename T>
Value<std::string> toString(Value<T> value, std::string fmtstr) noexcept {
    return std::move(value).transform([fmtstr = std::move(fmtstr)](T val) -> std::string {
        return fmt::format(fmt::runtime(fmtstr), val);
    });
}

template <typename T>
Value<std::string> toString(Value<T> value) noexcept {
    return std::move(value).transform([](T val) -> std::string {
        return fmt::to_string(val);
    });
}
/**
 * @brief Specifies how listeners will be notified in response to a value change.
 */
enum class BindType : uint8_t {
    Immediate, ///< Listeners will be notified immediately.
    Deferred,  ///< Listeners will be notified via a target object queue.

    Default = Immediate,
};

/**
 * @brief Handle that allows manually disconnecting bound values.
 */
struct BindingHandle {
    BindingHandle() noexcept = default;

    /**
     * @brief Checks if the handle is valid.
     * @return True if the handle is valid, false otherwise.
     */
    explicit operator bool() const noexcept {
        return m_id != 0;
    }

    bool operator!() const noexcept {
        return !operator bool();
    }

private:
    BindingHandle(uint64_t id) noexcept : m_id(id) {}

    friend class Bindings;

    /**
     * @brief Generates a unique ID starting from 1.
     * @return A new unique ID.
     */
    static uint64_t generate() noexcept {
        static std::atomic_uint64_t value{ 0 };
        return ++value;
    }

    uint64_t m_id = 0; ///< The unique identifier for the binding handle.
};

template <typename... Args>
struct BindableCallback {
    Callback<Args...> callback;
    BindingAddress address; ///< The associated binding address.

    constexpr BindableCallback() noexcept : address{} {}

    constexpr BindableCallback(Callback<Args...> callback, BindingAddress address) noexcept
        : callback(std::move(callback)), address(address) {}

    template <typename Class, typename... FnArgs>
    constexpr BindableCallback(Class* class_, void (Class::*method)(FnArgs...)) noexcept
        : callback([class_, method](Args... args) {
              (class_->*method)(std::forward<Args>(args)...);
          }),
          address(toBindingAddress(class_)) {}

    template <typename Class, typename... FnArgs>
    constexpr BindableCallback(const Class* class_, void (Class::*method)(FnArgs...) const) noexcept
        : callback([class_, method](Args... args) {
              (class_->*method)(std::forward<Args>(args)...);
          }),
          address(toBindingAddress(class_)) {}
};

/**
 * @brief Specifies the binding direction (used for disconnecting values).
 */
enum BindDir : uint8_t {
    Dest, ///< Value is a destination.
    Src,  ///< Value is a source.
    Both, ///< Value is either a source or a destination.
};

/**
 * @brief Singleton class for binding values and notifying about changes.
 *
 * This class provides mechanisms for connecting instances of `Value`,
 * notifying about variable changes (including batch notifications),
 * and registering regions with associated queues.
 *
 * Access the singleton instance via the `bindings` global variable.
 *
 * @threadsafe All public methods are thread-safe and can be called from any thread.
 */
class Bindings {
private:
    mutable std::recursive_mutex m_mutex;

public:
    Bindings();
    ~Bindings();

    Bindings(const Bindings&)            = delete;
    Bindings(Bindings&&)                 = delete;
    Bindings& operator=(const Bindings&) = delete;
    Bindings& operator=(Bindings&&)      = delete;

    /**
     * @brief Connects two values using bidirectional binding.
     *
     * When `src` changes, `dest` is updated to match `src`.
     * When `dest` changes, `src` is updated to match `dest`.
     * The connection is automatically removed when either `dest` or `src` region is removed.
     *
     * @tparam TDest The type of the destination value.
     * @tparam TSrc The type of the source value.
     * @param dest The destination value.
     * @param src The source value.
     * @param type The binding type (Immediate or Deferred).
     * @param updateNow If true, immediately updates `dest` with the current value of `src`.
     * @param destDesc Optional description for the destination value.
     * @param srcDesc Optional description for the source value.
     * @return BindingHandle A handle that allows manually disconnecting the binding.
     */
    template <typename TDest, typename TSrc>
    BindingHandle connectBidir(Value<TDest> dest, Value<TSrc> src, BindType type = BindType::Default,
                               bool updateNow = true, std::string_view destDesc = {},
                               std::string_view srcDesc = {}) {
        std::lock_guard lk(m_mutex);
        static_assert(std::is_convertible_v<typename Internal::optional_value_type<TDest>::type,
                                            typename Internal::optional_value_type<TSrc>::type> &&
                      std::is_convertible_v<typename Internal::optional_value_type<TSrc>::type,
                                            typename Internal::optional_value_type<TDest>::type>);
        uint64_t id  = BindingHandle::generate();
        int numAdded = 0;
        numAdded += internalConnect(id, dest, src, type, updateNow, destDesc, srcDesc);
        numAdded += internalConnect(id, std::move(src), std::move(dest), type, false, srcDesc, destDesc);
        if (numAdded == 0)
            return BindingHandle();
        return BindingHandle(id);
    }

    /**
     * @brief Connects two values using one-way binding.
     *
     * When `src` changes, `dest` is updated to match `src`.
     * The connection is automatically removed when either `dest` or `src` region is removed.
     *
     * @tparam TDest The type of the destination value.
     * @tparam TSrc The type of the source value.
     * @param dest The destination value.
     * @param src The source value.
     * @param type The binding type (Immediate or Deferred).
     * @param updateNow If true, immediately updates `dest` with the current value of `src`.
     * @param destDesc Optional description for the destination value.
     * @param srcDesc Optional description for the source value.
     * @return BindingHandle A handle that allows manually disconnecting the binding.
     */
    template <typename TDest, typename TSrc>
    BindingHandle connect(Value<TDest> dest, Value<TSrc> src, BindType type = BindType::Default,
                          bool updateNow = true, std::string_view destDesc = {},
                          std::string_view srcDesc = {}) {
        std::lock_guard lk(m_mutex);
        static_assert(std::is_convertible_v<TSrc, TDest>);
        uint64_t id = BindingHandle::generate();
        int numAdded =
            internalConnect(id, std::move(dest), std::move(src), type, updateNow, destDesc, srcDesc);
        if (numAdded == 0)
            return BindingHandle();
        return BindingHandle(id);
    }

    /**
     * @brief Remove all bindings where the destination address matches `dest` and
     * the source address matches `src`.
     *
     * @tparam TDest The type of the destination value.
     * @tparam TSrc The type of the source value.
     * @param dest The destination value.
     * @param src The source value.
     */
    template <typename TDest, typename TSrc>
    void disconnect(Value<TDest> dest, Value<TSrc> src) {
        std::lock_guard lk(m_mutex);
        BindingAddresses srcAddresses = src.m_srcAddresses;
        BindingAddress destAddress    = dest.m_destAddress;
        internalDisconnect(std::move(destAddress), std::move(srcAddresses));
    }

    /**
     * @brief Disconnects all bindings where the given value is either a source or destination.
     *
     * @tparam T The type of the value.
     * @param val The value to disconnect.
     * @param dir The direction to disconnect (source, destination, or both).
     */
    template <typename T>
    void disconnect(Value<T> val, BindDir dir) {
        std::lock_guard lk(m_mutex);
        BindingAddresses addresses = val.addresses();
        internalDisconnect(std::move(addresses), dir);
    }

    /**
     * @brief Disconnects a binding using a previously saved handle.
     *
     * @param handle The handle representing the binding to be disconnected.
     */
    void disconnect(BindingHandle handle);

    /**
     * @brief Registers a region with an associated queue.
     *
     * The region must not be registered before calling this method.
     *
     * @param region The binding address representing the region.
     * @param queue The scheduler queue associated with the region.
     */
    void registerRegion(BindingAddress region, Rc<Scheduler> queue);

    /**
     * @brief Unregisters a previously registered region.
     *
     * The region must have been previously registered using `registerRegion`.
     *
     * @param region The binding address representing the region to unregister.
     */
    void unregisterRegion(BindingAddress region);

    /**
     * @brief Unregisters a previously registered region by its memory address.
     *
     * The region must have been previously registered using `registerRegion`.
     *
     * @param regionBegin Pointer to the beginning of the region to unregister.
     */
    void unregisterRegion(const uint8_t* regionBegin);

    template <typename T>
    BindingHandle listen(Value<T> src, Callback<> callback, BindingAddress address = staticBindingAddress,
                         BindType type = BindType::Default) {
        return connect(Value<T>::listener(std::move(callback), address), src, type, false);
    }

    template <typename T>
    BindingHandle listen(Value<T> src, Callback<ValueArgument<T>> callback,
                         BindingAddress address = staticBindingAddress, BindType type = BindType::Default) {
        return connect(Value<ValueArgument<T>>::listener(std::move(callback), address), src, type, false);
    }

    template <typename T>
    BindingHandle listen(Value<T> src, BindableCallback<> callback, BindType type = BindType::Default) {
        return connect(Value<ValueArgument<T>>::listener(std::move(callback.callback), callback.address), src,
                       type, false);
    }

    template <typename T>
    BindingHandle listen(Value<T> src, BindableCallback<ValueArgument<T>> callback,
                         BindType type = BindType::Default) {
        return connect(Value<ValueArgument<T>>::listener(std::move(callback.callback), callback.address), src,
                       type, false);
    }

    /**
     * @brief Notify that the variable has changed
     * This triggers update of all dependant values.
     *
     * @param range address range
     * @return int Number of handlers called
     */
    int notifyRange(BindingAddress range);

    /**
     * @brief Notify that the variable has changed.
     * This triggers update of all dependant values.
     * @return int Number of handlers called
     */
    template <typename T>
    int notify(T* variable) {
        return notifyRange(toBindingAddress(variable));
    }

    template <std::equality_comparable T>
    struct AutoNotify {
        void operator=(std::type_identity_t<T> newValue) {
            if (newValue != value) {
                value = std::move(newValue);
                bindings->notify(&value);
            }
        }

        template <typename U>
        void operator+=(U&& argument) {
            operator=(value + std::forward<U>(argument));
        }

        template <typename U>
        void operator-=(U&& argument) {
            operator=(value - std::forward<U>(argument));
        }

        void operator++() {
            if constexpr (std::is_same_v<T, bool>) {
                value = !value;
            } else {
                ++value;
            }
            // comparison is not needed
            bindings->notify(&value);
        }

        void operator--() {
            if constexpr (std::is_same_v<T, bool>) {
                value = !value;
            } else {
                --value;
            }
            // comparison is not needed
            bindings->notify(&value);
        }

        void operator++(int /*dummy*/) {
            operator++();
        }

        void operator--(int /*dummy*/) {
            operator--(0);
        }

    private:
        friend class Bindings;

        constexpr AutoNotify(Bindings* bindings, T& value) noexcept : bindings(bindings), value(value) {}

        Bindings* bindings;
        T& value;
    };

    template <std::equality_comparable T>
    [[deprecated("Use *bindings->modify()")]] AutoNotify<T> assign(T& variable) {
        return AutoNotify<T>{ this, variable };
    }

    template <typename T>
    class ModifyProxy {
    private:
        Bindings* bindings;
        T* value;

    public:
        explicit ModifyProxy(Bindings* bindings, T* value) noexcept : bindings(bindings), value(value) {}

        ModifyProxy(ModifyProxy&&)                 = default;
        ModifyProxy(const ModifyProxy&)            = delete;
        ModifyProxy& operator=(ModifyProxy&&)      = delete;
        ModifyProxy& operator=(const ModifyProxy&) = delete;

        ~ModifyProxy() {
            bindings->notify(value);
        }

        T* operator->() const noexcept {
            return value;
        }

        T& operator*() const noexcept {
            return *value;
        }
    };

    template <std::equality_comparable T>
    ModifyProxy<T> modify(T& variable) noexcept {
        return ModifyProxy<T>{ this, &variable };
    }

    template <std::equality_comparable T>
    bool assign(T& variable, std::type_identity_t<T> newValue) {
        if (newValue != variable) {
            variable = std::move(newValue);
            notify(&variable);
            return true;
        } else {
            return false;
        }
    }

    template <std::equality_comparable T>
    bool assign(std::atomic<T>& variable, std::type_identity_t<T> newValue) {
        T oldValue = variable.exchange(newValue);
        if (oldValue != newValue) {
            notify(&variable);
            return true;
        } else {
            return false;
        }
    }

    size_t numRegions() const noexcept;
    size_t numHandlers() const noexcept;

private:
    friend struct BindingHandle;

    using Handler = Callback<>;

    struct Entry;
    struct Region;

    void internalDisconnect(const BindingAddress& destAddress, const BindingAddresses& srcAddresses);
    void internalDisconnect(const BindingAddresses& addresses, BindDir dir);

    template <typename T>
    static std::string toStringSafe(const T& value, std::string fallback = "(value)") {
        if constexpr (fmt::has_formatter<T, fmt::format_context>()) {
            return fmt::to_string(value);
        } else {
            return fallback;
        }
    }

    Rc<Region> lookupRegion(BindingAddress address);

    static void enqueueInto(Rc<Scheduler> queue, VoidFunc fn, ExecuteImmediately mode);

    using RegionList = SmallVector<Rc<Region>, 1>;

    Rc<Scheduler> getQueue(const RegionList& regions) noexcept {
        for (const Rc<Region>& r : regions) {
            if (r && r->queue) {
                return r->queue;
            }
        }
        return nullptr;
    }

    template <typename TDest, typename TSrc>
    int internalConnect(uint64_t id, Value<TDest> dest, Value<TSrc> src, BindType type, bool updateNow,
                        std::string_view destDesc = {}, std::string_view srcDesc = {}) {
        if (dest.empty() || src.empty() || !dest.isWritable())
            return 0;

        BindingAddresses srcAddresses = src.m_srcAddresses;
        BindingAddress destAddress    = dest.m_destAddress;

        Rc<Region> destRegion         = lookupRegion(destAddress);
        BRISK_ASSERT_MSG("Bindings: destination value address isn't registered", destRegion);

        RegionList srcRegions;
        srcRegions.reserve(srcAddresses.size());
        for (BindingAddress a : srcAddresses) {
            Rc<Region> srcRegion = lookupRegion(a);
            BRISK_ASSERT_MSG("Bindings: source value address isn't registered", srcRegion);
            srcRegions.push_back(std::move(srcRegion));
        }
        Rc<Scheduler> srcQueue  = getQueue(srcRegions);
        Rc<Scheduler> destQueue = destRegion->queue;

        if (updateNow) {
            enqueueInto(
                srcQueue,
                [src, dest, destQueue]() {
                    TSrc value = src.get();
                    enqueueInto(
                        destQueue,
                        [dest, value = std::move(value)]() {
                            if (auto optValue = Internal::wrapOptional(std::move(value))) {
                                dest.set(*optValue);
                            }
                        },
                        ExecuteImmediately::IfOnThread);
                },
                ExecuteImmediately::IfOnThread);
        }
        if (srcAddresses.empty())
            return 0;

        WeakRc<Region> destRegionWeak = destRegion;

        Handler handler = [srcQueue, destQueue, type, dest = std::move(dest), src = std::move(src),
                           destRegionWeak = std::move(destRegionWeak)]() {
            TSrc val = src.get();
            LOG_BINDING(binding, "handler: get | {} <- ({}) <- {}", destDesc, toStringSafe(val), srcDesc);
            enqueueInto(
                destQueue,
                [=, dest = std::move(dest), val = std::move(val),
                 destRegionWeak = std::move(destRegionWeak)]() {
                    if (auto destRegion = destRegionWeak.lock()) {
                        if (auto optVal = Internal::wrapOptional(val)) {
                            dest.set(*optVal);
                            LOG_BINDING(binding, "handler: set | {} <- ({}) <- {}", destDesc,
                                        toStringSafe(val), srcDesc);
                        }
                    }
                },
                type == BindType::Immediate ? ExecuteImmediately::IfOnThread
                                            : ExecuteImmediately::IfProcessing);
        };

        return addHandler(srcRegions, id, std::move(handler), std::move(srcAddresses), destRegion.get(),
                          destAddress, type, destDesc, srcDesc, srcQueue);
    }

    void removeConnection(uint64_t id);

    // Returns number of added handlers
    int addHandler(const RegionList& srcRegions, uint64_t id, Handler handler, BindingAddresses srcAddresses,
                   Region* destRegion, BindingAddress destAddress, BindType type,
                   std::string_view destDesc = {}, std::string_view srcDesc = {},
                   Rc<Scheduler> srcQueue = nullptr);

    void removeIndirectDependencies(Region* region);

    bool isRegisteredRegion(BindingAddress region) const;

    bool isFullyWithinRegion(BindingAddress region) const;

    struct Entry {
        uint64_t id;
        Handler handler;
        Region* destRegion;
        BindingAddress destAddress;
        BindType type;
        std::string_view destDesc;
        std::string_view srcDesc;
        Rc<Scheduler> srcQueue;
        uint32_t counter;
    };

    struct BindingAddressCmp {
        bool operator()(BindingAddress lh, BindingAddress rh) const noexcept {
            return lh.address < rh.address;
        }
    };

    struct Region {
        Region(BindingAddress region, Rc<Scheduler> queue) : region(region), queue(std::move(queue)) {}

        BindingAddress region;
        std::multimap<BindingAddress, Entry, BindingAddressCmp> entries;
        bool entriesChanged = false;
        Rc<Scheduler> queue;

        void disconnectIf(function_ref<bool(const std::pair<BindingAddress, Entry>&)> pred);
    };

    uint32_t m_counter = 0;
    std::map<const uint8_t*, Rc<Region>> m_regions;
    std::vector<uint64_t> m_stack;

    bool inStack(uint64_t id) noexcept;
};

extern AutoSingleton<Bindings> bindings;

template <typename... Args>
inline int Trigger<Args...>::trigger(Args... args) {
    this->arg          = Type{ std::move(args)... };
    int handlersCalled = bindings->notify(this);
    this->arg          = std::nullopt;
    return handlersCalled;
}

template <typename T, typename U>
inline bool assignAndTrigger(T& target, U&& newValue, Trigger<>& trigger) {
    if (target != newValue) {
        target = std::forward<U>(newValue);
        trigger.trigger();
        return true;
    }
    return false;
}

template <typename T, typename U>
inline bool assignAndTrigger(T& target, U&& newValue, Trigger<std::type_identity_t<T>>& trigger) {
    if (target != newValue) {
        target = std::forward<U>(newValue);
        trigger.trigger(target);
        return true;
    }
    return false;
}

namespace Internal {

template <typename Fn, template <typename... Args> typename Tpl>
struct DeduceArgs;

template <typename Fn, typename FnRet, typename... FnArgs, template <typename... Args> typename Tpl>
struct DeduceArgs<FnRet (Fn::*)(FnArgs...), Tpl> {
    using Type = Tpl<FnArgs...>;
};

template <typename Fn, typename FnRet, typename... FnArgs, template <typename... Args> typename Tpl>
struct DeduceArgs<FnRet (Fn::*)(FnArgs...) const, Tpl> {
    using Type = Tpl<FnArgs...>;
};

} // namespace Internal

/**
 * @brief Automatically registers an object for binding.
 *
 * This struct can be used as a field within a target object to enable binding registration.
 * Example usage:
 * @code
 * struct BindingEnabled {
 *     std::string field;
 *     BindingRegistration reg{ this, nullptr };
 * };
 * @endcode
 */
struct BindingRegistration {
    BindingRegistration()                           = delete;
    BindingRegistration(const BindingRegistration&) = delete;
    BindingRegistration(BindingRegistration&&)      = delete;

    /**
     * @brief Constructs a BindingRegistration for a given object and queue.
     * @tparam T Type of the object to register.
     * @param thiz Pointer to the object being registered.
     * @param queue Reference-counted pointer to the queue.
     */
    template <typename T>
    BindingRegistration(const T* thiz, Rc<Scheduler> queue) : m_address(toBindingAddress(thiz).min()) {
        bindings->registerRegion(toBindingAddress(thiz), std::move(queue));
    }

    /**
     * @brief Destructor that unregisters the binding region.
     */
    ~BindingRegistration() {
        bindings->unregisterRegion(m_address);
    }

    const uint8_t* m_address;
};

/**
 * @brief Utility struct to link a callback to the lifetime of an associated object.
 *
 * This struct ensures that a callback is automatically removed when the associated object is deleted.
 * The object pointed to by `thiz` must reside within a registered memory range.
 */
struct BindingLifetime {
    /**
     * @brief Constructs a BindingLifetime for a specific object.
     * @tparam T Type of the object to associate with.
     * @param thiz Pointer to the object whose lifetime is being tracked.
     */
    template <typename T>
    constexpr BindingLifetime(const T* thiz) noexcept : m_address(thiz) {}

    const void* m_address;
};

/**
 * @brief Static BindingLifetime instance for callbacks with no lifetime protection.
 *
 * This can be used for callbacks that only access global or static variables.
 */
constinit const inline BindingLifetime staticLifetime{ &staticBinding };

/**
 * @brief Constructs a BindingLifetime for a specific object to use in callback binding.
 * @tparam T Type of the object to associate with.
 * @param thiz Pointer to the object whose lifetime is being tracked.
 * @return A BindingLifetime instance for the given object.
 */
template <typename T>
inline BindingLifetime lifetimeOf(T* thiz) noexcept {
    return BindingLifetime{ thiz };
}

template <typename Fn>
using DeduceBindableCallback =
    typename Internal::DeduceArgs<decltype(&Fn::operator()), BindableCallback>::Type;

/**
 * @brief Combines a BindingRegistration with a callback to create a bindable callback.
 * @tparam Fn Type of the callback function.
 * @param reg The BindingRegistration instance providing the address.
 * @param callback The callback function to associate.
 * @return A deduced bindable callback type combining the callback and registration address.
 */
template <typename Fn>
inline constexpr DeduceBindableCallback<Fn> operator|(const BindingRegistration& reg, Fn callback) noexcept {
    return { std::move(callback), reg.m_address };
}

/**
 * @brief Combines a BindingLifetime with a callback to create a bindable callback.
 * @tparam Fn Type of the callback function.
 * @param lt The BindingLifetime instance providing the address.
 * @param callback The callback function to associate.
 * @return A deduced bindable callback type combining the callback and lifetime address.
 */
template <typename Fn>
inline constexpr DeduceBindableCallback<Fn> operator|(const BindingLifetime& lt, Fn callback) noexcept {
    return { std::move(callback), toBindingAddress(lt.m_address) };
}

template <typename T>
[[nodiscard]] inline Value<T> Value<T>::mutableValue(T initialValue) {
    struct RegisteredValue {
        RegisteredValue(T value) noexcept : value(std::move(value)) {}

        T value;
        BindingRegistration registration{ this, nullptr };
    };

    Rc<RegisteredValue> val = std::make_shared<RegisteredValue>(initialValue);

    return Value{
        [val]() -> T {
            return val->value;
        },
        [val](T newValue) {
            bindings->assign(val->value, std::move(newValue));
        },
        { toBindingAddress(&val->value) },
        toBindingAddress(&val->value),
    };
}

template <typename T>
[[nodiscard]] inline Value<T> Value<T>::variable(T* pvalue) noexcept {
    if constexpr (std::is_const_v<T>) {
        return Value{
            [pvalue]() -> T {
                return *pvalue;
            },
            nullptr,
            { toBindingAddress(pvalue) },
            toBindingAddress(pvalue),
        };
    } else {
        return Value{
            [pvalue]() -> T {
                return *pvalue;
            },
            [pvalue](T newValue) {
                bindings->assign(*pvalue, std::move(newValue));
            },
            { toBindingAddress(pvalue) },
            toBindingAddress(pvalue),
        };
    }
}

template <typename T>
[[nodiscard]] inline Value<T> Value<T>::variable(T* pvalue, NotifyFn notify) noexcept {
    static_assert(!std::is_const_v<T>);
    return Value{
        [pvalue]() -> T {
            return *pvalue;
        },
        [pvalue, notify = std::move(notify)](T newValue) {
            if (bindings->assign(*pvalue, std::move(newValue))) {
                notify();
            }
        },
        { toBindingAddress(pvalue) },
        toBindingAddress(pvalue),
    };
}

template <typename T>
[[nodiscard]] inline Value<T> Value<T>::variable(AtomicType* pvalue) noexcept
    requires AtomicCompatible<T>
{
    static_assert(!std::is_const_v<T>);
    return Value{
        [pvalue]() -> T {
            return pvalue->load(std::memory_order::relaxed);
        },
        [pvalue](T newValue) {
            pvalue->store(newValue, std::memory_order::relaxed);
            bindings->notify(pvalue);
        },
        { toBindingAddress(pvalue) },
        toBindingAddress(pvalue),
    };
}

template <typename T>
[[nodiscard]] inline Value<T> Value<T>::variable(AtomicType* pvalue, NotifyFn notify) noexcept
    requires AtomicCompatible<T>
{
    static_assert(!std::is_const_v<T>);
    return Value{
        [pvalue]() -> T {
            return *pvalue;
        },
        [pvalue, notify = std::move(notify)](T newValue) {
            T oldValue = pvalue->exchange(newValue, std::memory_order::relaxed);
            if (oldValue != newValue) {
                bindings->notify(pvalue);
                notify();
            }
        },
        { toBindingAddress(pvalue) },
        toBindingAddress(pvalue),
    };
}

template <PropertyLike Prop, typename Type = typename Prop::Type>
inline void operator++(Prop& prop /* pre */)
    requires requires(Type v) { ++v; }
{
    Type val = prop.get();
    ++val;
    prop.set(std::move(val));
}

template <PropertyLike Prop, typename Type = typename Prop::Type>
inline void operator--(Prop& prop /* pre */)
    requires requires(Type v) { --v; }
{
    Type val = prop.get();
    --val;
    prop.set(std::move(val));
}

template <PropertyLike Prop, typename Type = typename Prop::Type>
inline void operator++(Prop& prop, int /* post */)
    requires requires(Type v) { v++; }
{
    Type val = prop.get();
    val++;
    prop.set(std::move(val));
}

template <PropertyLike Prop, typename Type = typename Prop::Type>
inline void operator--(Prop& prop, int /* post */)
    requires requires(Type v) { v--; }
{
    Type val = prop.get();
    val--;
    prop.set(std::move(val));
}

template <PropertyLike Prop, typename Type = typename Prop::Type, typename Arg>
inline void operator+=(Prop& prop, Arg&& arg)
    requires requires(Type v, Arg a) { v + a; }
{
    prop.set(prop.get() + std::forward<Arg>(arg));
}

template <PropertyLike Prop, typename Type = typename Prop::Type, typename Arg>
inline void operator-=(Prop& prop, Arg&& arg)
    requires requires(Type v, Arg a) { v - a; }
{
    prop.set(prop.get() - std::forward<Arg>(arg));
}

template <PropertyLike Prop, typename Type = typename Prop::Type, typename Arg>
inline void operator*=(Prop& prop, Arg&& arg)
    requires requires(Type v, Arg a) { v* a; }
{
    prop.set(prop.get() * std::forward<Arg>(arg));
}

template <PropertyLike Prop, typename Type = typename Prop::Type, typename Arg>
inline void operator/=(Prop& prop, Arg&& arg)
    requires requires(Type v, Arg a) { v / a; }
{
    prop.set(prop.get() / std::forward<Arg>(arg));
}

template <PropertyLike Prop, typename Type = typename Prop::Type, typename Arg>
inline void operator%=(Prop& prop, Arg&& arg)
    requires requires(Type v, Arg a) { v % a; }
{
    prop.set(prop.get() % std::forward<Arg>(arg));
}

template <PropertyLike Prop, typename Type = typename Prop::Type, typename Arg>
inline void operator<<=(Prop& prop, Arg&& arg)
    requires requires(Type v, Arg a) { v << a; }
{
    prop.set(prop.get() << std::forward<Arg>(arg));
}

template <PropertyLike Prop, typename Type = typename Prop::Type, typename Arg>
inline void operator>>=(Prop& prop, Arg&& arg)
    requires requires(Type v, Arg a) { v >> a; }
{
    prop.set(prop.get() >> std::forward<Arg>(arg));
}

template <PropertyLike Prop, typename Type = typename Prop::Type, typename Arg>
inline void operator&=(Prop& prop, Arg&& arg)
    requires requires(Type v, Arg a) { v & a; }
{
    prop.set(prop.get() & std::forward<Arg>(arg));
}

template <PropertyLike Prop, typename Type = typename Prop::Type, typename Arg>
inline void operator|=(Prop& prop, Arg&& arg)
    requires requires(Type v, Arg a) { v | a; }
{
    prop.set(prop.get() | std::forward<Arg>(arg));
}

template <PropertyLike Prop, typename Type = typename Prop::Type, typename Arg>
inline void operator^=(Prop& prop, Arg&& arg)
    requires requires(Type v, Arg a) { v ^ a; }
{
    prop.set(prop.get() ^ std::forward<Arg>(arg));
}

template <typename T>
using ValueOrConstRef = std::conditional_t<std::is_trivially_copyable_v<T>, T, const T&>;

template <typename T, typename Class, typename ValueType>
concept PropertyTraits = requires(T traits, Class* self, ValueType value) {
    { traits.address(self) } noexcept -> std::convertible_to<BindingAddress>;
    {
        traits.get(const_cast<const Class*>(self))
    } noexcept -> std::same_as<ValueOrConstRef<std::remove_const_t<ValueType>>>;
    { traits.name } -> std::convertible_to<const char*>;
} && (std::is_const_v<ValueType> || requires(T traits, Class* self, ValueType value) {
                             { traits.set(self, std::move(value)) };
                             { traits.set(self, static_cast<const ValueType&>(value)) };
                         });

using PropertyIndex = uint32_t;

struct PropertyId {
    const void* address;
    constexpr auto operator<=>(const PropertyId& other) const noexcept = default;
};

template <typename Class, typename T, PropertyIndex index>
struct Property {
    static_assert(!std::is_volatile_v<T>);
    static_assert(!std::is_reference_v<T>);

    using Type      = std::remove_const_t<T>;
    using ValueType = Type;

    Class* this_pointer;

    constexpr static bool isTrigger = Internal::isTrigger<T>;

    constexpr static bool isMutable = !std::is_const_v<T>;

    constexpr static const /* PropertyTraits<Class, T> */ auto& traits() noexcept {
        return tuplet::get<index>(Class::properties());
    }

    void listen(Callback<ValueArgument<T>> callback, BindingAddress address = staticBindingAddress,
                BindType bindType = BindType::Default) {
        bindings->listen(Value{ this }, std::move(callback), address, bindType);
    }

    void listen(Callback<> callback, BindingAddress address = staticBindingAddress,
                BindType bindType = BindType::Default) {
        bindings->listen(Value{ this }, std::move(callback), address, bindType);
    }

    operator PropertyId() const noexcept {
        return { &traits() };
    }

    operator ValueOrConstRef<Type>() const noexcept {
        return get();
    }

    void operator=(const ValueType& value) {
        set(value);
    }

    void operator=(ValueType&& value) {
        set(std::move(value));
    }

    template <typename U>
    void operator=(U&& value) {
        set(std::forward<U>(value));
    }

    template <typename U>
    static consteval bool accepts() noexcept {
        return requires(Class* ptr, U value) { traits().set(ptr, std::move(value)); };
    }

    [[nodiscard]] ValueOrConstRef<T> get() const noexcept {
        BRISK_ASSERT(this_pointer);
        BRISK_ASSUME(this_pointer);
        return traits().get(this_pointer);
    }

    [[nodiscard]] decltype(auto) current() const noexcept {
        BRISK_ASSERT(this_pointer);
        BRISK_ASSUME(this_pointer);
        if constexpr (requires {
                          { traits().current(this_pointer) } noexcept;
                      }) {
            return traits().current(this_pointer);
        } else {
            // Fallback to get() if current() is not defined
            return get();
        }
    }

    void set(const ValueType& value) const {
        static_assert(isMutable, "Attempt to write to immutable property");
        traits().set(this_pointer, value);
    }

    void set(ValueType&& value) const {
        static_assert(isMutable, "Attempt to write to immutable property");
        traits().set(this_pointer, std::move(value));
    }

    template <typename U>
    void set(U&& value) const
        requires(accepts<U>())
    {
        traits().set(this_pointer, std::forward<U>(value));
    }

    [[nodiscard]] static std::string_view name() noexcept {
        return traits().name == nullptr ? std::string_view{} : traits().name;
    }

    void operator=(Value<Type> value) {
        set(std::move(value));
    }

    void operator=(std::same_as<Value<std::optional<Type>>> auto value)
        requires(!IsOptional<Type>)
    {
        set(std::move(value));
    }

    void operator=(BindableCallback<> bindableCallback) {
        bindings->listen(Value{ this }, std::move(bindableCallback));
    }

    void operator=(BindableCallback<ValueArgument<T>> bindableCallback) {
        bindings->listen(Value{ this }, std::move(bindableCallback));
    }

    void set(Value<Type> value) {
        BRISK_ASSERT(this_pointer);
        BRISK_ASSUME(this_pointer);
        bindings->connectBidir(Value{ this }, std::move(value), BindType::Default);
    }

    void set(std::same_as<Value<std::optional<Type>>> auto value)
        requires(!IsOptional<Type>)
    {
        BRISK_ASSERT(this_pointer);
        BRISK_ASSUME(this_pointer);
        bindings->connectBidir(Value{ this }, std::move(value), BindType::Default);
    }

    BindingAddress address() const noexcept {
        BRISK_ASSERT(this_pointer);
        BRISK_ASSUME(this_pointer);
        return traits().address(this_pointer);
    }
};

namespace Internal {

template <typename Class, typename ValueType>
struct PropField {
    ValueType(Class::* field);
    const char* name = nullptr;

    void set(Class* self, ValueType value) const {
        bindings->assign(self->*field, std::move(value));
    }

    ValueOrConstRef<ValueType> get(const Class* self) const noexcept {
        return (self->*field);
    }

    BindingAddress address(const Class* self) const noexcept {
        return toBindingAddress(&(self->*field));
    }
};

template <typename Class, typename ValueType>
struct PropField<Class, std::atomic<ValueType>> {
    std::atomic<ValueType>(Class::* field);
    const char* name = nullptr;

    void set(Class* self, ValueType value) const {
        ValueType previous = (self->*field).exchange(value, std::memory_order_relaxed);
        if (previous != value) {
            bindings->notify(&(self->*field));
        }
    }

    ValueOrConstRef<ValueType> get(const Class* self) const noexcept {
        return (self->*field).load(std::memory_order_relaxed);
    }

    BindingAddress address(const Class* self) const noexcept {
        return toBindingAddress(&(self->*field));
    }
};

template <typename Class, typename ValueType>
PropField(ValueType(Class::*), const char* = nullptr) -> PropField<Class, ValueType>;

template <typename Class, typename ValueType>
struct alignas(sizeof(void*)) PropFieldNotify {
    ValueType(Class::* field);

    uint32_t isConst = 0;

    union {
        void (Class::*notify)();
        void (Class::*notifyConst)() const;
    };

    const char* name                                           = nullptr;

    constexpr PropFieldNotify(const PropFieldNotify&) noexcept = default;
    constexpr PropFieldNotify(PropFieldNotify&&) noexcept      = default;

    template <typename Class2>
    constexpr PropFieldNotify(ValueType(Class::* field), void (Class2::*notify)(),
                              const char* name = nullptr) noexcept
        : field(field), isConst{ 0 }, notify{ static_cast<void (Class::*)()>(notify) }, name(name) {}

    template <typename Class2>
    constexpr PropFieldNotify(ValueType(Class::* field), void (Class2::*notifyConst)() const,
                              const char* name = nullptr) noexcept
        : field(field), isConst{ 1 }, notifyConst{ static_cast<void (Class::*)() const>(notifyConst) },
          name(name) {}

    void set(Class* self, ValueType value) const {
        if (bindings->assign(self->*field, std::move(value))) {
            if (!isConst) {
                (self->*(notify))();
            } else if (notifyConst) {
                (self->*(notifyConst))();
            }
        }
    }

    ValueOrConstRef<ValueType> get(const Class* self) const noexcept {
        return (self->*field);
    }

    BindingAddress address(const Class* self) const noexcept {
        return toBindingAddress(&(self->*field));
    }
};

template <typename Class, typename Fn, typename ValueType>
PropFieldNotify(ValueType(Class::*), Fn&&, const char* = nullptr) -> PropFieldNotify<Class, ValueType>;

template <typename Class, typename ValueType>
struct PropFieldSetter {
    ValueType(Class::* field);
    void (Class::*setter)(ValueType);
    const char* name = nullptr;

    void set(Class* self, ValueType value) const {
        (self->*setter)(std::move(value));
    }

    ValueType get(const Class* self) const noexcept {
        return (self->*field);
    }

    BindingAddress address(const Class* self) const noexcept {
        return toBindingAddress(&(self->*field));
    }
};

template <typename Class, typename ValueType>
PropFieldSetter(ValueType(Class::*), void (Class::*)(ValueType), const char* = nullptr)
    -> PropFieldSetter<Class, ValueType>;

template <typename Class, typename FieldType, typename ValueType>
struct PropGetterSetter {
    FieldType(Class::* field);
    ValueType (Class::*getter)() const noexcept;
    void (Class::*setter)(ValueType);
    const char* name = "";

    void set(Class* self, ValueType value) const {
        (self->*setter)(std::move(value));
    }

    ValueType get(const Class* self) const noexcept {
        return (self->*getter)();
    }

    BindingAddress address(const Class* self) const noexcept {
        return toBindingAddress(&(self->*field));
    }
};

template <typename Class, typename FieldType, typename ValueType>
PropGetterSetter(FieldType(Class::*), ValueType (Class::*)() const noexcept, void (Class::*)(ValueType),
                 const char* = nullptr) -> PropGetterSetter<Class, FieldType, ValueType>;

} // namespace Internal

#define BRISK_PROPERTIES union

#define BRISK_PROPERTIES_BEGIN                                                                               \
    union {                                                                                                  \
        void* propInit = nullptr;

#define BRISK_PROPERTIES_END                                                                                 \
    }                                                                                                        \
    ;                                                                                                        \
    int propInitDummy = (propInit = this, 0);

class Object {
public:
    virtual ~Object() noexcept {}
};

template <typename T>
concept PointerToScheduler = requires(T p) {
    { *p } -> std::convertible_to<Rc<Scheduler>>;
};

template <typename Derived, PointerToScheduler auto scheduler = static_cast<Rc<Scheduler>*>(nullptr)>
class BindableObject : public Object,
                       public std::enable_shared_from_this<BindableObject<Derived, scheduler>> {
private:
    using Base = std::enable_shared_from_this<BindableObject<Derived, scheduler>>;

public:
    ~BindableObject() override {}

    using Ptr = std::shared_ptr<Derived>;

    [[nodiscard]] std::shared_ptr<Derived> shared_from_this() {
        return std::static_pointer_cast<Derived>(Base::shared_from_this());
    }

    [[nodiscard]] std::shared_ptr<const Derived> shared_from_this() const {
        return std::static_pointer_cast<const Derived>(Base::shared_from_this());
    }

    static void* operator new(size_t sz) {
        void* ptr = alignedAlloc(sz, cacheAlignment);
        Rc<Scheduler> sched;
        BRISK_CLANG_PRAGMA(GCC diagnostic push)
        BRISK_CLANG_PRAGMA(GCC diagnostic ignored "-Wpointer-bool-conversion")
        if constexpr (scheduler) {
            sched = *scheduler;
        }
        bindings->registerRegion(BindingAddress{ ptr, sz }, std::move(sched));
        BRISK_CLANG_PRAGMA(GCC diagnostic pop)
        return ptr;
    }

    static void operator delete(void* ptr) {
        bindings->unregisterRegion(reinterpret_cast<uint8_t*>(ptr));
        alignedFree(ptr);
    }

    BindingLifetime lifetime() const noexcept {
        return lifetimeOf(this);
    }
};

template <PointerToScheduler auto scheduler_ = static_cast<Rc<Scheduler>*>(nullptr)>
struct SchedulerParam {
    static constexpr auto scheduler = scheduler_;
};

template <typename T, typename SchedulerPtr>
class BindableAllocator {
public:
    // --- Member Types (Required by Allocator concept) ---
    using value_type                                               = T;
    using size_type                                                = std::size_t;
    using difference_type                                          = std::ptrdiff_t;
    using pointer                                                  = T*;
    using const_pointer                                            = const T*;

    // --- Propagation Traits (Control allocator behavior on container ops) ---
    using propagate_on_container_copy_assignment                   = std::true_type;
    using propagate_on_container_move_assignment                   = std::true_type;
    using propagate_on_container_swap                              = std::true_type;

    // --- Statelessness Trait (All instances are equivalent) ---
    // This is important for optimizations.
    using is_always_equal                                          = std::true_type;

    // --- Constructors ---
    // Default constructor
    constexpr BindableAllocator() noexcept                         = default;

    // Copy constructor (needed for rebinding)
    constexpr BindableAllocator(const BindableAllocator&) noexcept = default;

    // Template copy constructor (enables rebinding)
    template <typename U>
    constexpr BindableAllocator(const BindableAllocator<U, SchedulerPtr>&) noexcept {}

    // Destructor
    ~BindableAllocator() = default;

    // --- Core Allocator Functions ---

    /**
     * @brief Allocates uninitialized storage.
     * @param n The number of objects to allocate storage for.
     * @return A pointer to the allocated storage.
     * @throws std::bad_alloc If allocation fails.
     * @throws std::length_error If n * sizeof(T) overflows size_t.
     */
    [[nodiscard]] pointer allocate(size_type n) {
        if (n > std::numeric_limits<size_type>::max() / sizeof(T)) {
            throw std::length_error("BindableAllocator::allocate: size overflow");
        }

        size_type bytes_to_allocate = n * sizeof(T);
        if (bytes_to_allocate == 0) {
            return nullptr; // Standard allows returning nullptr for zero size
        }

        // Allocate memory using global operator new
        // Using the sized delete version requires C++14 or later support for ::operator new
        void* p = ::operator new(bytes_to_allocate);
        if (!p) {
            throw std::bad_alloc(); // Should theoretically not happen as ::operator new throws
        }

        // --- Call the custom registration function ---
        try {
            Rc<Scheduler> sched;
            BRISK_CLANG_PRAGMA(GCC diagnostic push)
            BRISK_CLANG_PRAGMA(GCC diagnostic ignored "-Wpointer-bool-conversion")
            if constexpr (SchedulerPtr::scheduler) {
                sched = *SchedulerPtr::scheduler;
            }
            bindings->registerRegion(BindingAddress{ p, bytes_to_allocate }, std::move(sched));
        } catch (...) {
            // If bindingRegister throws, we must free the allocated memory
            // before propagating the exception.
            ::operator delete(p, bytes_to_allocate); // Use sized delete if available/appropriate
            throw;                                   // Re-throw the exception from bindingRegister
        }

        return static_cast<pointer>(p);
    }

    /**
     * @brief Deallocates storage previously allocated by allocate.
     * @param p Pointer to the memory to deallocate. Must have been returned by a prior call to allocate(n).
     * @param n The number of objects for which storage was allocated. Must be the same value passed to
     * allocate.
     */
    void deallocate(pointer p, size_type n) noexcept {
        if (p == nullptr || n == 0) {
            return; // Deallocating nullptr or zero size is a no-op
        }

        size_type bytes_to_deallocate = n * sizeof(T);

        try {
            bindings->unregisterRegion(BindingAddress{ static_cast<void*>(p), bytes_to_deallocate });
        } catch (...) {
            BRISK_ASSERT_MSG("WARNING: unregisterRegion threw an exception during deallocate! Memory might "
                             "leak if not handled",
                             false);
            std::terminate();
        }

        ::operator delete(static_cast<void*>(p), bytes_to_deallocate);
    }

    // --- Comparison Operators (Required for stateless allocators) ---

    template <typename U>
    constexpr bool operator==(const BindableAllocator<U, SchedulerPtr>&) const noexcept {
        return true;
    }

    template <typename U>
    constexpr bool operator!=(const BindableAllocator<U, SchedulerPtr>&) const noexcept {
        return false;
    }
};

template <typename T, PointerToScheduler auto scheduler = static_cast<Rc<Scheduler>*>(nullptr)>
using BindableList = std::deque<T, BindableAllocator<T, SchedulerParam<scheduler>>>;

} // namespace Brisk
