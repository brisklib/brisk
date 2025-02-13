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
#include <brisk/core/Memory.hpp>
#include <brisk/core/RC.hpp>
#include <brisk/core/BasicTypes.hpp>
#include <brisk/core/Utilities.hpp>
#include <brisk/core/internal/SmallVector.hpp>
#include <brisk/core/Log.hpp>
#include <brisk/core/Threading.hpp>

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

// Type alias for a binding address
using BindingAddress = Range<const uint8_t*>;

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
constexpr BindingAddress toBindingAddress(T* value) noexcept {
    return BindingAddress{
        reinterpret_cast<const uint8_t*>(value),
        reinterpret_cast<const uint8_t*>(value) + sizeof(T),
    };
}

/**
 * @brief A special object for static binding.
 */
inline Empty staticBinding{};

/**
 * @brief The BindingAddress for the static binding object.
 */
inline BindingAddress staticBindingAddress = toBindingAddress(&staticBinding);

/**
 * @brief A collection of BindingAddresses.
 */
using BindingAddresses                     = SmallVector<BindingAddress, 1>;

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

    operator Type() const {
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
Value<Type> asValue(const Prop& prop) {
    return Value<Type>(
        [prop]() -> Type {
            return prop.get();
        },
        nullptr, // read-only
        prop.address());
}

template <PropertyLike Prop, typename Type = typename Prop::ValueType>
Value<Type> asValue(Prop& prop) {
    return Value<Type>(
        [prop]() -> Type {
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
Value<std::invoke_result_t<Fn, T...>> transform(Fn&& fn, const Value<T>&... values);

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
    [[nodiscard]] explicit Value(U* property) : Value(Internal::asValue(*property)) {}

    template <PropertyLike U>
    [[nodiscard]] explicit Value(const U* property) : Value(Internal::asValue(*property)) {}

    [[nodiscard]] explicit Value(T* value) : Value(variable(value)) {}

    [[nodiscard]] explicit Value(T* value, NotifyFn notify) : Value(variable(value, std::move(notify))) {}

    template <typename NotifyClass>
    [[nodiscard]] explicit Value(T* value, NotifyClass* self, void (NotifyClass::*notify)())
        : Value(variable(value, [self, notify]() {
              (self->*notify)();
          })) {}

    [[nodiscard]] explicit Value(AtomicType* value)
        requires AtomicCompatible<T>
        : Value(variable(value)) {}

    [[nodiscard]] explicit Value(AtomicType* value, NotifyFn notify)
        requires AtomicCompatible<T>
        : Value(variable(value, std::move(notify))) {}

    template <typename NotifyClass>
    [[nodiscard]] explicit Value(AtomicType* value, NotifyClass* self, void (NotifyClass::*notify)())
        requires AtomicCompatible<T>
        : Value(variable(value, [self, notify]() {
              (self->*notify)();
          })) {}

    template <typename U>
    Value<U> explicitConversion() && {
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
    Value(Value<U> other)
        requires std::is_convertible_v<U, T>
        : Value(std::move(other).template implicitConversion<T>()) {}

    template <typename U>
    explicit Value(Value<U> other)
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
    [[nodiscard]] static Value constant(T constant) {
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
    [[nodiscard]] Value readOnly() && {
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
    [[nodiscard]] Value readOnly() const& {
        Value(*this).readOnly();
    }

    /**
     * @brief Returns mutable Value that initially holds the given value.
     *
     * @param initialValue value
     */
    [[nodiscard]] static Value mutableValue(T initialValue);

    [[nodiscard]] static Value computed(function<T()> func) {
        return Value{
            std::move(func),
            nullptr, // no-op
            nullptr,
        };
    }

    [[nodiscard]] static Value listener(Callback<T> listener, BindingAddress range) {
        return Value{
            nullptr,
            [listener = std::move(listener)](T newValue) {
                listener(std::move(newValue));
            },
            { range },
            range,
        };
    }

    [[nodiscard]] static Value listener(Callback<> listener, BindingAddress range) {
        return Value{
            nullptr,
            [listener = std::move(listener)](T newValue) {
                listener();
            },
            { range },
            range,
        };
    }

    [[nodiscard]] static Value variable(T* pvalue);

    [[nodiscard]] static Value variable(T* pvalue, NotifyFn notify);

    [[nodiscard]] static Value variable(AtomicType* pvalue)
        requires AtomicCompatible<T>;

    [[nodiscard]] static Value variable(AtomicType* pvalue, NotifyFn notify)
        requires AtomicCompatible<T>;

    [[nodiscard]] bool isWritable() const {
        return !m_set.empty();
    }

    [[nodiscard]] bool isReadable() const {
        return !m_get.empty();
    }

    [[nodiscard]] bool hasAddress() const {
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
    Value<U> transform(Forward&& forward, Backward&& backward) && {
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
    Value<U> transform(Forward&& forward, Backward&& backward) && {
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
    Value<U> transform(Forward&& forward) && {
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
              std::invocable<U> Backward>
    Value<U> transform(Forward&& forward, Backward&& backward) const& {
        return Value<T>{ *this }.transform(std::forward<Forward>(forward), std::forward<Backward>(backward));
    }

    template <std::invocable<T> Forward, typename U = std::invoke_result_t<Forward, T>,
              std::invocable<T, U> Backward>
    Value<U> transform(Forward&& forward, Backward&& backward) const& {
        return Value<T>{ *this }.transform(std::forward<Forward>(forward), std::forward<Backward>(backward));
    }

    template <std::invocable<T> Forward, typename U = std::invoke_result_t<Forward, T>>
    Value<U> transform(Forward&& forward) const& {
        return Value<T>{ *this }.transform(std::forward<Forward>(forward));
    }

    template <std::invocable<T, T> Fn>
    friend Value<std::invoke_result_t<Fn, T, T>> binary(Value left, Value right, Fn&& fn) {
        return Value<T>{
            [fn = std::move(fn), leftGet = std::move(left.m_get), rightGet = std::move(right.m_get)]() -> T {
                return fn(leftGet(), rightGet());
            },
            nullptr,
            mergeSmallVectors(std::move(left.m_srcAddresses), std::move(right.m_srcAddresses)),
            std::move(left.m_destAddress),
        };
    }

    template <std::invocable<T, T> Fn>
    friend Value<std::invoke_result_t<Fn, T, T>> binary(Value left, std::type_identity_t<T> right, Fn&& fn) {
        return Value<T>{
            [fn = std::move(fn), leftGet = std::move(left.m_get), right = std::move(right)]() -> T {
                return fn(leftGet(), right);
            },
            nullptr,
            std::move(left.m_srcAddresses),
            std::move(left.m_destAddress),
        };
    }

    template <std::invocable<T, T> Fn>
    friend Value<std::invoke_result_t<Fn, T, T>> binary(std::type_identity_t<T> left, Value right, Fn&& fn) {
        return Value<T>{
            [fn = std::move(fn), left = std::move(left), rightGet = std::move(right.m_get)]() -> T {
                return fn(left, rightGet());
            },
            nullptr,
            std::move(right.m_srcAddresses),
            std::move(right.m_destAddress),
        };
    }

    Value<bool> equal(std::type_identity_t<T> compare) && {
        return Value<bool>{
            [get = std::move(m_get), compare]() -> bool {
                return get() == compare;
            },
            [set = std::move(m_set), compare](bool newValue) {
                if (newValue)
                    if (set)
                        set(compare);
            },
            std::move(m_srcAddresses),
            std::move(m_destAddress),
        };
    }

    Value<bool> equal(std::type_identity_t<T> compare) const& {
        return Value<T>{ *this }.equal(std::move(compare));
    }

#define BRISK_BINDING_OP(oper, op)                                                                           \
    friend inline Value oper(Value left, Value right)                                                        \
        requires requires(T l, T r) {                                                                        \
            { l op r } -> std::convertible_to<T>;                                                            \
        }                                                                                                    \
    {                                                                                                        \
        return binary(std::move(left), std::move(right), [](T l, T r) {                                      \
            return l op r;                                                                                   \
        });                                                                                                  \
    }

    friend inline Value operator+(Value left, Value right)
        requires requires(T l, T r) {
            { l + r } -> std ::convertible_to<T>;
        }
    {
        return binary(std ::move(left), std ::move(right), [](T l, T r) {
            return l + r;
        });
    }

    friend inline Value operator-(Value left, Value right)
        requires requires(T l, T r) {
            { l - r } -> std ::convertible_to<T>;
        }
    {
        return binary(std ::move(left), std ::move(right), [](T l, T r) {
            return l - r;
        });
    }

    friend inline Value operator*(Value left, Value right)
        requires requires(T l, T r) {
            { l* r } -> std ::convertible_to<T>;
        }
    {
        return binary(std ::move(left), std ::move(right), [](T l, T r) {
            return l * r;
        });
    }

    friend inline Value operator/(Value left, Value right)
        requires requires(T l, T r) {
            { l / r } -> std ::convertible_to<T>;
        }
    {
        return binary(std ::move(left), std ::move(right), [](T l, T r) {
            return l / r;
        });
    }

    friend inline Value<bool> operator<(Value left, Value right)
        requires requires(T l, T r) {
            { l < r } -> std ::convertible_to<bool>;
        }
    {
        return binary(std ::move(left), std ::move(right), [](T l, T r) {
            return l < r;
        });
    }

    friend inline Value<bool> operator>(Value left, Value right)
        requires requires(T l, T r) {
            { l > r } -> std ::convertible_to<bool>;
        }
    {
        return binary(std ::move(left), std ::move(right), [](T l, T r) {
            return l > r;
        });
    }

    friend inline Value<bool> operator<=(Value left, Value right)
        requires requires(T l, T r) {
            { l <= r } -> std ::convertible_to<bool>;
        }
    {
        return binary(std ::move(left), std ::move(right), [](T l, T r) {
            return l <= r;
        });
    }

    friend inline Value<bool> operator>=(Value left, Value right)
        requires requires(T l, T r) {
            { l >= r } -> std ::convertible_to<bool>;
        }
    {
        return binary(std ::move(left), std ::move(right), [](T l, T r) {
            return l >= r;
        });
    }

    struct Operand {
        const Value& value;

        friend Value<bool> operator==(Operand op1, Operand op2) {
            return Brisk::transform(
                [](T x, T y) -> bool {
                    return x == y;
                },
                op1.value, op2.value);
        }

        friend Value<bool> operator!=(Operand op1, Operand op2) {
            return Brisk::transform(
                [](T x, T y) -> bool {
                    return x != y;
                },
                op1.value, op2.value);
        }
    };

    Operand operator*() const {
        return *this;
    }

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

    const BindingAddresses& srcAddresses() const {
        return m_srcAddresses;
    }

    BindingAddresses addresses() const {
        BindingAddresses result = m_srcAddresses;
        result.push_back(m_destAddress);
        return result;
    }

    [[nodiscard]] explicit Value(GetFn get, SetFn set, BindingAddresses srcAddresses,
                                 BindingAddress destAddress)
        : m_get(std::move(get)), m_set(std::move(set)), m_srcAddresses(std::move(srcAddresses)),
          m_destAddress(std::move(destAddress)) {}

    [[nodiscard]] explicit Value(GetFn get, SetFn set, BindingAddress address)
        : m_get(std::move(get)), m_set(std::move(set)), m_srcAddresses{ address }, m_destAddress(address) {}

private:
    template <typename U>
    Value<U> implicitConversion() && {
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
Value<std::invoke_result_t<Fn, T...>> transform(Fn&& fn, const Value<T>&... values) {
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
                std::type_identity_t<FT> curvature = 1) {
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
                   std::type_identity_t<FT> cut = 0) {
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
Value<std::string> toString(Value<T> value, std::string fmtstr = "{}") {
    return value.transform([get = value.getter(), fmtstr = std::move(fmtstr)]() {
        return fmt::format(fmtstr, get());
    });
}
/**
 * @brief Specifies how listeners will be notified in response to a value change.
 */
enum class BindType : uint8_t {
    Immediate, ///< Listeners will be notified immediately.
    Deferred,  ///< Listeners will be notified via a target object queue.
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
    BindingHandle(uint64_t id) : m_id(id) {}

    friend class Bindings;

    /**
     * @brief Generates a unique ID starting from 1.
     * @return A new unique ID.
     */
    static uint64_t generate() {
        static std::atomic_uint64_t value{ 0 };
        return ++value;
    }

    uint64_t m_id = 0; ///< The unique identifier for the binding handle.
};

template <typename... Args>
struct Listener {
    Callback<Args...> callback;
    BindingAddress address; ///< The associated binding address.

    constexpr Listener(Callback<Args...> callback, BindingAddress address)
        : callback(std::move(callback)), address(address) {}

    template <typename Class>
    constexpr Listener(Class* class_, void (Class::*method)(Args...))
        : callback([class_, method](Args... args) {
              (class_->*method)(std::forward<Args>(args)...);
          }),
          address(toBindingAddress(class_)) {}

    template <typename Class>
    constexpr Listener(const Class* class_, void (Class::*method)(Args...) const)
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
     * @param type The binding type (Immediate or Deferred, default: Deferred).
     * @param updateNow If true, immediately updates `dest` with the current value of `src`.
     * @param destDesc Optional description for the destination value.
     * @param srcDesc Optional description for the source value.
     * @return BindingHandle A handle that allows manually disconnecting the binding.
     */
    template <typename TDest, typename TSrc>
    BindingHandle connectBidir(Value<TDest> dest, Value<TSrc> src, BindType type = BindType::Deferred,
                               bool updateNow = true, std::string_view destDesc = {},
                               std::string_view srcDesc = {}) {
        std::lock_guard lk(m_mutex);
        static_assert(std::is_convertible_v<TDest, TSrc> && std::is_convertible_v<TSrc, TDest>);
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
     * @param type The binding type (Immediate or Deferred, default: Deferred).
     * @param updateNow If true, immediately updates `dest` with the current value of `src`.
     * @param destDesc Optional description for the destination value.
     * @param srcDesc Optional description for the source value.
     * @return BindingHandle A handle that allows manually disconnecting the binding.
     */
    template <typename TDest, typename TSrc>
    BindingHandle connect(Value<TDest> dest, Value<TSrc> src, BindType type = BindType::Deferred,
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
    void registerRegion(BindingAddress region, RC<Scheduler> queue);

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
                         BindType type = BindType::Immediate) {
        return connect(Value<T>::listener(std::move(callback), address), src, type, false);
    }

    template <typename T>
    BindingHandle listen(Value<T> src, Callback<ValueArgument<T>> callback,
                         BindingAddress address = staticBindingAddress, BindType type = BindType::Immediate) {
        return connect(Value<ValueArgument<T>>::listener(std::move(callback), address), src, type, false);
    }

    template <typename T>
    BindingHandle listen(Value<T> src, Listener<> callback, BindType type = BindType::Immediate) {
        return connect(Value<ValueArgument<T>>::listener(std::move(callback.callback), callback.address), src,
                       type, false);
    }

    template <typename T>
    BindingHandle listen(Value<T> src, Listener<ValueArgument<T>> callback,
                         BindType type = BindType::Immediate) {
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
    AutoNotify<T> assign(T& variable) {
        return AutoNotify<T>{ this, variable };
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

    RC<Region> lookupRegion(BindingAddress address);

    static void enqueueInto(RC<Scheduler> queue, VoidFunc fn, ExecuteImmediately mode);

    using RegionList = SmallVector<RC<Region>, 1>;

    RC<Scheduler> getQueue(const RegionList& regions) {
        for (const RC<Region>& r : regions) {
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

        RC<Region> destRegion         = lookupRegion(destAddress);
        BRISK_ASSERT_MSG("Bindings: destination value address isn't registered", destRegion);

        RegionList srcRegions;
        srcRegions.reserve(srcAddresses.size());
        for (BindingAddress a : srcAddresses) {
            RC<Region> srcRegion = lookupRegion(a);
            BRISK_ASSERT_MSG("Bindings: source value address isn't registered", srcRegion);
            srcRegions.push_back(std::move(srcRegion));
        }
        RC<Scheduler> srcQueue  = getQueue(srcRegions);
        RC<Scheduler> destQueue = destRegion->queue;

        if (updateNow) {
            enqueueInto(
                srcQueue,
                [src, dest, destQueue]() {
                    TSrc value = src.get();
                    enqueueInto(
                        destQueue,
                        [dest, value = std::move(value)]() {
                            dest.set(static_cast<TDest>(value));
                        },
                        ExecuteImmediately::IfOnThread);
                },
                ExecuteImmediately::IfOnThread);
        }
        if (srcAddresses.empty())
            return 0;

        WeakRC<Region> destRegionWeak = destRegion;

        Handler handler = [srcQueue, destQueue, type, dest = std::move(dest), src = std::move(src),
                           destRegionWeak = std::move(destRegionWeak)]() {
            TSrc val = src.get();
            LOG_BINDING(binding, "handler: get | {} <- ({}) <- {}", destDesc, toStringSafe(val), srcDesc);
            enqueueInto(
                destQueue,
                [=, dest = std::move(dest), val = std::move(val),
                 destRegionWeak = std::move(destRegionWeak)]() {
                    if (auto destRegion = destRegionWeak.lock()) {
                        dest.set(static_cast<TDest>(val));
                        LOG_BINDING(binding, "handler: set | {} <- ({}) <- {}", destDesc, toStringSafe(val),
                                    srcDesc);
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
                   RC<Scheduler> srcQueue = nullptr);

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
        RC<Scheduler> srcQueue;
        uint32_t counter;
    };

    struct BindingAddressCmp {
        bool operator()(BindingAddress lh, BindingAddress rh) const noexcept {
            return lh.min < rh.min;
        }
    };

    struct Region {
        Region(BindingAddress region, RC<Scheduler> queue) : region(region), queue(std::move(queue)) {}

        BindingAddress region;
        std::multimap<BindingAddress, Entry, BindingAddressCmp> entries;
        bool entriesChanged = false;
        RC<Scheduler> queue;

        void disconnectIf(std::function<bool(const std::pair<BindingAddress, Entry>&)> pred);
    };

    uint32_t m_counter = 0;
    std::map<const uint8_t*, RC<Region>> m_regions;
    std::vector<uint64_t> m_stack;

    bool inStack(uint64_t id);
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

struct BindingRegistration {
    BindingRegistration()                           = delete;
    BindingRegistration(const BindingRegistration&) = delete;
    BindingRegistration(BindingRegistration&&)      = delete;

    template <typename T>
    BindingRegistration(T* thiz, RC<Scheduler> queue) : m_address(toBindingAddress(thiz).min) {
        bindings->registerRegion(toBindingAddress(thiz), std::move(queue));
    }

    ~BindingRegistration() {
        bindings->unregisterRegion(m_address);
    }

    const uint8_t* m_address;
};

struct BindingLifetime {
    template <typename T>
    BindingLifetime(T* thiz) noexcept : m_address(toBindingAddress(thiz).min) {}

    const uint8_t* m_address;
};

const inline BindingLifetime staticLifetime{ &staticBinding };

template <typename T>
inline BindingLifetime lifetimeOf(T* thiz) noexcept {
    return BindingLifetime{ thiz };
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

template <typename Fn>
using DeduceListener = typename Internal::DeduceArgs<decltype(&Fn::operator()), Listener>::Type;

template <typename Fn>
inline DeduceListener<Fn> operator|(const BindingRegistration& reg, Fn callback) {
    return { std::move(callback), reg.m_address };
}

template <typename Fn>
inline DeduceListener<Fn> operator|(const BindingLifetime& lt, Fn callback) {
    return { std::move(callback), toBindingAddress(lt.m_address) };
}

template <typename T>
[[nodiscard]] inline Value<T> Value<T>::mutableValue(T initialValue) {
    struct RegisteredValue {
        RegisteredValue(T value) : value(std::move(value)) {}

        T value;
        BindingRegistration registration{ this, nullptr };
    };

    RC<RegisteredValue> val = std::make_shared<RegisteredValue>(initialValue);

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
[[nodiscard]] inline Value<T> Value<T>::variable(T* pvalue) {
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

template <typename T>
[[nodiscard]] inline Value<T> Value<T>::variable(T* pvalue, NotifyFn notify) {
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
[[nodiscard]] inline Value<T> Value<T>::variable(AtomicType* pvalue)
    requires AtomicCompatible<T>
{
    return Value{
        [pvalue]() -> T {
            return pvalue->load(std::memory_order::relaxed);
        },
        [pvalue](T newValue) {
            bindings->assign(*pvalue, std::move(newValue));
        },
        { toBindingAddress(pvalue) },
        toBindingAddress(pvalue),
    };
}

template <typename T>
[[nodiscard]] inline Value<T> Value<T>::variable(AtomicType* pvalue, NotifyFn notify)
    requires AtomicCompatible<T>
{
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

template <typename Class, typename T, auto std::type_identity_t<Class>::* field,
          std::remove_const_t<T> (Class::*getter)() const = nullptr,
          void (Class::*setter)(std::remove_const_t<T>) = nullptr, auto changed = nullptr, bool notify = true>
struct Property {
public:
    static_assert(!std::is_volatile_v<T>);
    static_assert(!std::is_reference_v<T>);

    using Type                      = std::remove_const_t<T>;
    using ValueType                 = Type;

    constexpr static bool isTrigger = Internal::isTrigger<T>;

    constexpr static bool isMutable = !std::is_const_v<T>;

    static_assert(field != nullptr || getter != nullptr);

    void listen(Callback<ValueArgument<T>> callback, BindingAddress address = staticBindingAddress,
                BindType bindType = BindType::Immediate) {
        bindings->listen(Value{ this }, std::move(callback), address, bindType);
    }

    void listen(Callback<> callback, BindingAddress address = staticBindingAddress,
                BindType bindType = BindType::Immediate) {
        bindings->listen(Value{ this }, std::move(callback), address, bindType);
    }

    operator Type() const noexcept {
        return get();
    }

    void operator=(Type value)
        requires isMutable
    {
        set(std::move(value));
    }

    Type get() const noexcept {
        BRISK_ASSERT(this_pointer);
        BRISK_ASSUME(this_pointer);
        if BRISK_IF_GNU_ATTR (constexpr)
            (getter == nullptr) {
                static_assert(field != nullptr);
                return this_pointer->*field;
            }
        else {
            return (this_pointer->*getter)();
        }
    }

    void set(Type value)
        requires isMutable
    {
        BRISK_ASSERT(this_pointer);
        BRISK_ASSUME(this_pointer);
        if BRISK_IF_GNU_ATTR (constexpr)
            (setter == nullptr) {
                static_assert(field != nullptr);
                if constexpr (requires { this_pointer->*field = std::move(value); }) {
                    // NOLINTBEGIN(clang-analyzer-core.NonNullParamChecker,clang-analyzer-core.NullDereference)
                    if (value == this_pointer->*field)
                        return; // Not changed
                    // NOLINTEND(clang-analyzer-core.NonNullParamChecker,clang-analyzer-core.NullDereference)
                    this_pointer->*field = std::move(value);
                }
            }
        else {
            (this_pointer->*setter)(std::move(value));
        }
        if constexpr (notify) {
            bindings->notify(&(this_pointer->*field));
        }
        if constexpr (changed != nullptr) {
            (this_pointer->*changed)();
        }
    }

    void operator=(Value<Type> value) {
        set(std::move(value));
    }

    void operator=(Listener<> listener) {
        bindings->listen(Value{ this }, std::move(listener));
    }

    void operator=(Listener<ValueArgument<T>> listener) {
        bindings->listen(Value{ this }, std::move(listener));
    }

    void set(Value<Type> value) {
        BRISK_ASSERT(this_pointer);
        BRISK_ASSUME(this_pointer);
        bindings->connectBidir(Value{ this }, std::move(value));
    }

    BindingAddress address() const {
        BRISK_ASSERT(this_pointer);
        BRISK_ASSUME(this_pointer);
        return toBindingAddress(&(this_pointer->*field));
    }

    Class* this_pointer;
};

namespace Internal {

struct Dummy1 {
    bool m_v;
};

static_assert(PropertyLike<Property<Dummy1, bool, &Dummy1::m_v>>);
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
    { *p } -> std::convertible_to<RC<Scheduler>>;
};

template <typename Derived, PointerToScheduler auto scheduler = static_cast<RC<Scheduler>*>(nullptr)>
class BindingObject : public Object, public std::enable_shared_from_this<BindingObject<Derived, scheduler>> {
private:
    using Base = std::enable_shared_from_this<BindingObject<Derived, scheduler>>;

public:
    using Ptr = std::shared_ptr<Derived>;

    [[nodiscard]] std::shared_ptr<Derived> shared_from_this() {
        return std::static_pointer_cast<Derived>(Base::shared_from_this());
    }

    [[nodiscard]] std::shared_ptr<const Derived> shared_from_this() const {
        return std::static_pointer_cast<const Derived>(Base::shared_from_this());
    }

    static void* operator new(size_t sz) {
        void* ptr = alignedAlloc(sz, cacheAlignment);
        RC<Scheduler> sched;
        BRISK_CLANG_PRAGMA(GCC diagnostic push)
        BRISK_CLANG_PRAGMA(GCC diagnostic ignored "-Wpointer-bool-conversion")
        if constexpr (scheduler) {
            sched = *scheduler;
        }
        bindings->registerRegion(
            BindingAddress{ reinterpret_cast<uint8_t*>(ptr), reinterpret_cast<uint8_t*>(ptr) + sz },
            std::move(sched));
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

} // namespace Brisk
