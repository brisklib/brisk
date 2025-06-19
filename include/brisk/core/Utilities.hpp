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

#include <tuple>
#include <atomic>
#include <utility>
#include <memory>
#include <vector>
#include <type_traits>
#include <functional>
#include "Brisk.h"
#include <brisk/core/Exceptions.hpp>
#include "internal/Optional.hpp"
#include "internal/Typename.hpp"
#include <brisk/core/internal/Debug.hpp>

namespace Brisk {

/**
 * @brief A caching structure that stores a value and its associated parameters.
 *        Updates the value if the input parameters change.
 *
 * @tparam T The type of the cached value.
 * @tparam Args The types of the parameters used to generate the cached value.
 */
template <typename T, typename... Args>
struct Cache {
    /// The cached value.
    std::optional<T> data;
    /// The tuple storing the input parameters associated with the cached value.
    std::tuple<Args...> parameters;

    /**
     * @brief Updates the cached value by calling the provided function if the input parameters change.
     *
     * @tparam Fn The type of the function used to compute the value.
     * @param fn The function to compute the value.
     * @param args The input parameters for the function.
     */
    template <typename Fn>
    void update(Fn&& fn, const Args&... args) {
        std::tuple<const Args&...> newParameters = std::make_tuple(std::cref(args)...);
        if (!data.has_value() || parameters != newParameters) {
            data       = fn(args...);
            parameters = newParameters;
        }
    }

    /**
     * @brief Calls the provided function and updates the cached value if necessary, then returns the cached
     * value.
     *
     * @tparam Fn The type of the function used to compute the value.
     * @param fn The function to compute the value.
     * @param args The input parameters for the function.
     * @return The updated cached value.
     */
    template <typename Fn>
    T& operator()(Fn&& fn, const Args&... args) {
        update(std::forward<Fn>(fn), args...);
        return get();
    }

    /**
     * @brief Returns a reference to the cached value.
     *
     * @return A reference to the cached value.
     */
    T& get() {
        return *data;
    }

    /**
     * @brief Returns a constant reference to the cached value.
     *
     * @return A constant reference to the cached value.
     */
    const T& get() const {
        return *data;
    }
};

/**
 * @brief A utility that checks if the given arguments have changed since the last call.
 *
 * @tparam Args The types of the arguments to track.
 */
template <typename... Args>
struct IfChanged {
    static_assert(sizeof...(Args) > 0);

    /// The last stored tuple of arguments.
    std::optional<std::tuple<Args...>> data;

    /**
     * @brief Checks if the given arguments have changed since the last call.
     *
     * @param args The arguments to check for changes.
     * @return True if the arguments have changed, otherwise false.
     */
    bool operator()(const Args&... args) {
        std::tuple<Args...> newData = std::make_tuple(args...);
        if (!data.has_value() || newData != *data) {
            data = newData;
            return true;
        }
        return false;
    }

    /**
     * @brief Resets the internal state, clearing the stored arguments.
     */
    void reset() {
        data = std::nullopt;
    }
};

/**
 * @brief A cache that stores an instance of a value and can be copied or moved.
 *
 * @tparam T The type of the cached value.
 */
template <typename T>
struct InstanceCache {
    /**
     * @brief Constructs an InstanceCache object with the given arguments.
     *
     * @tparam Args The types of the arguments used to construct the value.
     * @param args The arguments used to construct the value.
     */
    template <typename... Args>
    InstanceCache(Args&&... args) noexcept(std::is_nothrow_constructible_v<T, Args...>)
        : value{ std::forward<Args>(args)... } {}

    /**
     * @brief Copy constructor. Initializes the value with default construction instead of copying.
     */
    InstanceCache(const InstanceCache& other) noexcept(std::is_nothrow_default_constructible_v<T>)
        : value{} {}

    /**
     * @brief Move constructor. Initializes the value with default construction instead of moving.
     */
    InstanceCache(InstanceCache&& other) noexcept(std::is_nothrow_default_constructible_v<T>) : value{} {}

    /**
     * @brief Copy assignment operator. Does nothing and returns *this.
     */
    InstanceCache& operator=(const InstanceCache& other) {
        return *this;
    }

    /**
     * @brief Move assignment operator. Does nothing and returns *this.
     */
    InstanceCache& operator=(InstanceCache&& other) {
        return *this;
    }

    /// Destructor.
    ~InstanceCache() = default;

    /// The cached value.
    T value;
};

/**
 * @brief A simple cache that stores a value based on a key and a member function of a class.
 *
 * @tparam CachedValue The type of the cached value.
 * @tparam Key The type of the key.
 * @tparam Class The class type containing the member function.
 * @tparam Func The member function that returns the cached value.
 */
template <typename CachedValue, std::equality_comparable Key, typename Class,
          CachedValue (Class::*Func)(const Key&)>
struct SimpleCache {
    /// Pointer to the instance of the class that holds the member function.
    Class* self{};

    /**
     * @brief Returns the key associated with the cached value.
     *
     * @return The key associated with the cached value.
     */
    const Key& key() const {
        return m_cache->first;
    }

    /**
     * @brief Returns the cached value.
     *
     * @return The cached value.
     */
    const CachedValue& get() const {
        return m_cache->second;
    }

    /**
     * @brief Updates the cache if the key has changed by invoking the member function.
     *
     * @param key The key to check and update.
     * @return True if the cache was updated, otherwise false.
     */
    bool update(const Key& key) {
        if (!m_cache.has_value() || m_cache->first != key) {
            m_cache = std::make_pair(key, (self->*Func)(key));
            return true;
        }
        return false;
    }

    /// Optionally holds the cached key-value pair.
    std::optional<std::pair<Key, CachedValue>> m_cache;
};

/**
 * @brief A cache that stores a value based on a key and invalidates it when the key changes.
 *
 * @tparam CachedValue The type of the cached value.
 * @tparam Key The type of the key.
 * @tparam Class The class type containing the member function.
 * @tparam Func The member function that returns the cached value.
 */
template <typename CachedValue, std::equality_comparable Key, typename Class,
          CachedValue (Class::*Func)(const Key&)>
struct CacheWithInvalidation {
    /// Pointer to the instance of the class that holds the member function.
    Class* self{};
    /// The key associated with the cached value.
    Key m_key{};

    /// Optionally holds the cached value.
    mutable std::optional<CachedValue> m_value;

    /**
     * @brief Returns a pointer to the cached value, updating it if necessary.
     *
     * @return A pointer to the cached value.
     */
    const CachedValue* operator->() const {
        update();
        return &(*m_value);
    }

    /**
     * @brief Returns the key associated with the cached value.
     *
     * @return The key associated with the cached value.
     */
    const Key& key() const {
        return m_key;
    }

    /**
     * @brief Returns the cached value, updating it if necessary.
     *
     * @return The cached value.
     */
    const CachedValue& value() const {
        update();
        return *m_value;
    }

    /**
     * @brief Updates the cached value if it is not already set.
     */
    void update() const {
        if (!m_value.has_value()) {
            m_value = (self->*Func)(m_key);
        }
    }

    /**
     * @brief Invalidates the cached value if the key has changed.
     *
     * @param key The new key to check.
     * @return True if the cache was invalidated, otherwise false.
     */
    bool invalidate(const Key& key, bool force = false) {
        if (force || m_key != key) {
            m_key   = key;
            m_value = std::nullopt;
            return true;
        }
        return false;
    }
};

template <typename T, typename Tag = T, bool thread = true>
struct ImplicitContextScope;

namespace Internal {
template <typename T, typename Tag, bool thread>
struct ImplicitContextStorage;

template <typename T, typename Tag>
struct ImplicitContextStorage<T, Tag, true> {
    inline static thread_local std::remove_const_t<T> instance{};
};

template <typename T, typename Tag>
struct ImplicitContextStorage<T, Tag, false> {
    inline static std::remove_const_t<T> instance{};
};
} // namespace Internal

template <typename T, typename Tag = T, bool thread = true>
struct ImplicitContext : private Internal::ImplicitContextStorage<T, Tag, thread> {
    using Scope = ImplicitContextScope<T, Tag, thread>;

    T& get() const {
        return Internal::ImplicitContextStorage<T, Tag, thread>::instance;
    }

    T& operator*() const {
        return get();
    }

    T* operator->() const {
        return &get();
    }

private:
    friend struct ImplicitContextScope<T, Tag, thread>;
};

template <typename T, typename Tag, bool thread>
struct ImplicitContext<T*, Tag, thread> : private Internal::ImplicitContextStorage<T*, Tag, thread> {
    using Scope = ImplicitContextScope<T*, Tag, thread>;

    T* get() const {
        T* ptr = get(nullptr);
        BRISK_ASSERT(ptr);
        return ptr;
    }

    T* get(T* fallback) const {
        return Internal::ImplicitContextStorage<T*, Tag, thread>::instance
                   ? Internal::ImplicitContextStorage<T*, Tag, thread>::instance
                   : fallback;
    }

    T& operator*() const {
        return *get();
    }

    T* operator->() const {
        return get();
    }

    bool operator!() const noexcept {
        return !static_cast<bool>(*this);
    }

    explicit operator bool() const noexcept {
        return Internal::ImplicitContextStorage<T*, Tag, thread>::instance != nullptr;
    }

private:
    friend struct ImplicitContextScope<T*, Tag, thread>;
};

template <typename T, typename Tag, bool thread>
struct ImplicitContextScope {
    ImplicitContextScope(std::remove_const_t<T> newCtx) {
        oldCtx                                    = std::move(ImplicitContext<T, Tag, thread>::instance);
        ImplicitContext<T, Tag, thread>::instance = std::move(newCtx);
    }

    ~ImplicitContextScope() {
        ImplicitContext<T, Tag, thread>::instance = std::move(oldCtx);
    }

private:
    std::remove_const_t<T> oldCtx;
};

/**
 * @brief A RAII-style helper class for temporarily changing a value and restoring it upon scope exit.
 *
 * The `ScopedValue` template class allows temporarily modifying a value within a given scope. When the
 * object of `ScopedValue` goes out of scope, it automatically restores the original value.
 *
 * @tparam T The type of the value being managed.
 */
template <typename T>
struct ScopedValue {
    /**
     * @brief Constructs a `ScopedValue` object that changes the target value to `newValue` and saves the
     * original value.
     *
     * @param target The reference to the value that will be modified.
     * @param newValue The new value to set for `target`.
     */
    ScopedValue(T& target, T newValue) : target(target), saved(std::move(target)) {
        target = std::move(newValue);
    }

    /**
     * @brief Restores the original value when the `ScopedValue` object goes out of scope.
     */
    ~ScopedValue() {
        target = std::move(saved);
    }

    T& target; ///< The reference to the value being modified.
    T saved;   ///< The original value saved for restoration.
};

/**
 * @brief Type deduction guide for `ScopedValue`.
 *
 * @tparam T The type of the value being managed.
 */
template <typename T>
ScopedValue(T&, T) -> ScopedValue<T>;

/**
 * @brief A RAII-style helper class for executing a callable object upon scope exit.
 *
 * The `ScopeExit` template class allows the user to specify a function or lambda that will be executed
 * when the `ScopeExit` object goes out of scope.
 *
 * @tparam Fn The type of the callable object to be executed.
 */
template <typename Fn>
struct ScopeExit {
    /**
     * @brief Constructs a `ScopeExit` object with a callable function or lambda.
     *
     * @tparam Fn_ The type of the callable object.
     * @param fn The callable object to be executed upon scope exit.
     */
    template <typename Fn_>
    ScopeExit(Fn_&& fn) : fn(std::forward<Fn_>(fn)) {}

    /**
     * @brief Executes the callable object when the `ScopeExit` object goes out of scope.
     */
    ~ScopeExit() {
        fn();
    }

    Fn fn; ///< The callable object to be executed upon scope exit.

    ScopeExit()                 = delete;  ///< Default constructor is deleted.
    ScopeExit(const ScopeExit&) = delete;  ///< Copy constructor is deleted.
    ScopeExit(ScopeExit&&)      = default; ///< Move constructor is defaulted.
};

/**
 * @brief Macro to create a `ScopeExit` object that will execute a lambda function or callable object upon
 * scope exit.
 *
 * This macro creates a `ScopeExit` object that will invoke the specified lambda or callable when it goes
 * out of scope. It uses a unique name for the `ScopeExit` object to ensure that multiple uses of this macro
 * in the same scope do not conflict.
 *
 * Example usage:
 * @code
 * void someFunction() {
 *     // Create a ScopeExit object that will be invoked at the end of this scope
 *     SCOPE_EXIT {
 *         std::cout << "Scope exited!" << std::endl;
 *     };
 *
 *     // Do some work
 *     std::cout << "Doing work..." << std::endl;
 * }
 * @endcode
 */
#define SCOPE_EXIT ScopeExit BRISK_CONCAT(e_, __LINE__) = [&]()

/**
 * @brief Type deduction guide for `ScopeExit`.
 *
 * @tparam Fn The type of the callable object to be executed.
 */
template <typename Fn>
ScopeExit(Fn&&) -> ScopeExit<std::decay_t<Fn>>;

template <typename For, typename Type = int>
Type autoincremented() {
    static_assert(std::numeric_limits<Type>::is_integer);
    static std::atomic<Type> id = 0;
    return ++id;
}

/**
 * @brief A type alias for a key-value pair.
 *
 * This alias represents a standard pair containing a key of type Key
 * and a value of type Value.
 *
 * @tparam Key The type of the key.
 * @tparam Value The type of the value.
 */
template <typename Key, typename Value>
using KeyValue = std::pair<Key, Value>;

/**
 * @brief A type alias for an ordered list of key-value pairs.
 *
 * This alias represents a vector of KeyValue pairs.
 *
 * @tparam Key The type of the key.
 * @tparam Value The type of the value.
 */
template <typename Key, typename Value>
using KeyValueOrderedList = std::vector<KeyValue<Key, Value>>;

/**
 * @brief A type alias for an ordered list of name-value pairs.
 *
 * This alias represents a vector of KeyValue pairs where the key is a string.
 *
 * @tparam Value The type of the value.
 */
template <typename Value>
using NameValueOrderedList = std::vector<KeyValue<std::string, Value>>;

/**
 * @brief Finds the index of a specified value in a list.
 *
 * Searches for the given value in a vector of type V.
 * Returns the index of the value if found, otherwise returns std::nullopt.
 *
 * @tparam V The type of the values in the list.
 * @tparam T The type of the value to find.
 * @param list The list to search through.
 * @param value The value to find.
 * @return std::optional<size_t> The index of the found value, or std::nullopt if not found.
 */
template <typename V, typename T>
inline std::optional<size_t> findValue(const std::vector<V>& list, const T& value) {
    for (size_t i = 0; i < list.size(); ++i) {
        if (list[i] == value)
            return i;
    }
    return std::nullopt;
}

/**
 * @brief Finds the index of a specified value in a key-value ordered list.
 *
 * Searches for the given value in a KeyValueOrderedList.
 * Returns the index of the value if found, otherwise returns std::nullopt.
 *
 * @tparam K The type of the keys.
 * @tparam V The type of the values.
 * @param list The list to search through.
 * @param value The value to find.
 * @return std::optional<size_t> The index of the found value, or std::nullopt if not found.
 */
template <typename K, typename V>
inline std::optional<size_t> findValue(const KeyValueOrderedList<K, V>& list, const V& value) {
    for (size_t i = 0; i < list.size(); ++i) {
        if (list[i].second == value)
            return i;
    }
    return std::nullopt;
}

/**
 * @brief Finds the index of a specified key in a key-value ordered list.
 *
 * Searches for the given key in a KeyValueOrderedList.
 * Returns the index of the key if found, otherwise returns std::nullopt.
 *
 * @tparam K The type of the keys.
 * @tparam V The type of the values.
 * @param list The list to search through.
 * @param name The key to find.
 * @return std::optional<size_t> The index of the found key, or std::nullopt if not found.
 */
template <typename K, typename V>
inline std::optional<size_t> findKey(const KeyValueOrderedList<K, V>& list, const K& name) {
    for (size_t i = 0; i < list.size(); ++i) {
        if (list[i].first == name)
            return i;
    }
    return std::nullopt;
}

/**
 * @brief Finds an iterator to a specified key in a key-value ordered list.
 *
 * Searches for the given key in a KeyValueOrderedList and returns an iterator
 * to the found element.
 *
 * @tparam K The type of the keys.
 * @tparam V The type of the values.
 * @param list The list to search through.
 * @param name The key to find.
 * @return Iterator to the found key or list.end() if not found.
 */
template <typename K, typename V>
inline auto findKeyIt(const KeyValueOrderedList<K, V>& list, const K& name) {
    return std::find_if(list.begin(), list.end(), [&](const KeyValue<K, V>& pair) {
        return pair.first == name;
    });
}

/**
 * @brief Converts a value to its corresponding key in a key-value ordered list.
 *
 * Searches for the given value and returns the associated key if found,
 * otherwise returns std::nullopt.
 *
 * @tparam K The type of the keys.
 * @tparam V The type of the values.
 * @param list The list to search through.
 * @param value The value to find.
 * @return std::optional<K> The associated key, or std::nullopt if not found.
 */
template <typename K, typename V>
inline std::optional<K> valueToKey(const KeyValueOrderedList<K, V>& list, const V& value) {
    for (size_t i = 0; i < list.size(); ++i) {
        if (list[i].second == value)
            return list[i].first;
    }
    return std::nullopt;
}

/**
 * @brief Converts a key to its corresponding value in a key-value ordered list.
 *
 * Searches for the given key and returns the associated value if found,
 * otherwise returns std::nullopt.
 *
 * @tparam K The type of the keys.
 * @tparam V The type of the values.
 * @param list The list to search through.
 * @param name The key to find.
 * @return std::optional<V> The associated value, or std::nullopt if not found.
 */
template <typename K, typename V>
inline std::optional<V> keyToValue(const KeyValueOrderedList<K, V>& list, const K& name) {
    for (size_t i = 0; i < list.size(); ++i) {
        if (list[i].first == name)
            return list[i].second;
    }
    return std::nullopt;
}

/**
 * @brief Sets a value by its corresponding key in a key-value ordered list.
 *
 * Updates the value associated with the given key if it exists,
 * otherwise adds a new key-value pair to the list.
 *
 * @tparam K The type of the keys.
 * @tparam V The type of the values.
 * @param list The list to update.
 * @param key The key for which to set the value.
 * @param value The value to set.
 */
template <typename K, typename V>
inline void setValueByKey(KeyValueOrderedList<K, V>& list, const K& key, V value) {
    auto it = std::find_if(list.begin(), list.end(), [&](const KeyValue<K, V>& p) BRISK_INLINE_LAMBDA {
        return p.first == key;
    });
    if (it == list.end()) {
        list.push_back({ key, std::move(value) });
    } else {
        (*it).second = std::move(value);
    }
}

/**
 * @brief Removes a value by its corresponding key in a key-value ordered list.
 *
 * Searches for the specified key and removes the associated key-value pair
 * if found.
 *
 * @tparam K The type of the keys.
 * @tparam V The type of the values.
 * @param list The list from which to remove the key-value pair.
 * @param key The key to remove.
 */
template <typename K, typename V>
inline void removeValueByKey(KeyValueOrderedList<K, V>& list, const K& key) {
    auto it = std::find_if(list.begin(), list.end(), [&](const KeyValue<K, V>& p) BRISK_INLINE_LAMBDA {
        return p.first == key;
    });
    if (it != list.end()) {
        list.erase(it);
    }
}

/**
 * @brief Finds a value in a vector by a specified field.
 *
 * Searches for the given name in a vector of objects of type V by accessing
 * the specified field.
 *
 * @tparam V The type of the objects in the list.
 * @tparam K The type of the field used for searching.
 * @param list The list to search through.
 * @param field Pointer to the field to compare.
 * @param fieldValue The value to find.
 * @return std::optional<V> The found value, or std::nullopt if not found.
 */
template <typename V, typename K>
inline std::optional<V> keyToValue(const std::vector<V>& list, K(V::* field), const K& fieldValue) {
    for (size_t i = 0; i < list.size(); ++i) {
        if (list[i].*field == fieldValue)
            return list[i];
    }
    return std::nullopt;
}

/**
 * @brief Finds the index of a value in a vector by a specified field.
 *
 * Searches for the given value in a vector of objects of type V by accessing
 * the specified field.
 *
 * @tparam V The type of the objects in the list.
 * @tparam K The type of the field used for searching.
 * @param list The list to search through.
 * @param field Pointer to the field to compare.
 * @param fieldValue The value to find.
 * @return std::optional<size_t> The index of the found key, or std::nullopt if not found.
 */
template <typename V, typename K>
inline std::optional<size_t> findKey(const std::vector<V>& list, K(V::* field), const K& fieldValue) {
    for (size_t i = 0; i < list.size(); ++i) {
        if (list[i].*field == fieldValue)
            return i;
    }
    return std::nullopt;
}

/**
 * @brief A compile-time function for mapping values.
 *
 * Always returns the fallback value.
 *
 * @tparam Tout The type of the fallback value.
 * @tparam Tin The type of the input value.
 * @tparam Args Additional argument types (unused).
 * @param value The value to compare.
 * @param fallback The fallback value to return.
 * @return Tout The fallback value.
 */
template <typename Tout, typename Tin, typename... Args>
constexpr Tout staticMap(Tin value, Tout fallback) noexcept {
    return fallback;
}

/**
 * @brief A compile-time function for mapping values.
 *
 * Compares the input value with the specified type and returns
 * the corresponding output if found.
 *
 * @tparam Tout The type of the output value.
 * @tparam Tin The type of the input value.
 * @tparam Args Additional argument types for comparison.
 * @param value The value to compare.
 * @param in The input type to match.
 * @param out The output value to return if a match is found.
 * @param args Additional arguments for further comparisons.
 * @return Tout The corresponding output value.
 */
template <typename Tout, typename Tin, typename... Args>
constexpr Tout staticMap(Tin value, std::type_identity_t<Tin> in, Tout out, Args... args) noexcept {
    if (value == in)
        return out;
    return staticMap<Tout>(value, args...);
}

template <typename T>
struct ClonablePtr {
    template <typename... Args>
    ClonablePtr(Args&&... args) : m_ptr(new T{ std::forward<Args>(args)... }) {}

    ~ClonablePtr() {
        delete m_ptr;
    }

    ClonablePtr(ClonablePtr&& ptr) noexcept : m_ptr(nullptr) {
        swap(ptr);
    }

    ClonablePtr(const ClonablePtr& ptr) noexcept : m_ptr(nullptr) {
        ClonablePtr(*ptr).swap(*this);
    }

    ClonablePtr& operator=(ClonablePtr&& ptr) noexcept {
        swap(ptr);
    }

    ClonablePtr& operator=(const ClonablePtr& ptr) noexcept {
        ClonablePtr(*ptr).swap(*this);
    }

    const T& operator*() const noexcept {
        return *m_ptr;
    }

    T& operator*() noexcept {
        return *m_ptr;
    }

    const T* operator->() const noexcept {
        return m_ptr;
    }

    T* operator->() noexcept {
        return m_ptr;
    }

    const T* get() const noexcept {
        return m_ptr;
    }

    T* get() noexcept {
        return m_ptr;
    }

    void swap(ClonablePtr& other) noexcept {
        std::swap(m_ptr, other.m_ptr);
    }

    T* m_ptr;
};

/**
 * @class ENullDeref
 * @brief Exception thrown when attempting to dereference a null pointer.
 */
class ENullDeref : public Exception<std::runtime_error> {
public:
    using Exception<std::runtime_error>::Exception;
};

/**
 * @class Nullable
 * @brief A safe pointer wrapper that throws an exception on null dereference.
 * @tparam T The type of the object the pointer points to.
 * @details Provides a safer alternative to raw pointers by throwing ENullDeref when dereferencing a null
 * pointer.
 */
template <typename T>
struct Nullable {
public:
    /**
     * @brief Default constructor, initializes pointer to nullptr.
     */
    constexpr Nullable() noexcept = default;

    /**
     * @brief Constructs a Nullable from a raw pointer.
     * @param ptr The raw pointer to wrap.
     */
    constexpr Nullable(T* ptr) noexcept : m_ptr(ptr) {}

    /**
     * @brief Constructs a Nullable from nullptr.
     */
    constexpr Nullable(std::nullptr_t) noexcept : m_ptr(nullptr) {}

    /**
     * @brief Default copy constructor.
     */
    constexpr Nullable(const Nullable&) noexcept            = default;

    /**
     * @brief Default copy assignment operator.
     * @return Reference to this Nullable.
     */
    constexpr Nullable& operator=(const Nullable&) noexcept = default;

    /**
     * @brief Assigns a raw pointer to this Nullable.
     * @param ptr The raw pointer to assign.
     * @return Reference to this Nullable.
     */
    constexpr Nullable& operator=(T* ptr) noexcept {
        m_ptr = ptr;
        return *this;
    }

    /**
     * @brief Assigns nullptr to this Nullable.
     * @return Reference to this Nullable.
     */
    constexpr Nullable& operator=(std::nullptr_t) noexcept {
        m_ptr = nullptr;
        return *this;
    }

    /**
     * @brief Arrow operator for accessing members of the pointed-to object.
     * @return Pointer to the object.
     * @throws ENullDeref if the pointer is null.
     */
    [[nodiscard]] constexpr T* operator->() const {
        ensureNonNull();
        return m_ptr;
    }

    /**
     * @brief Dereference operator for accessing the pointed-to object.
     * @return Reference to the object.
     * @throws ENullDeref if the pointer is null.
     */
    [[nodiscard]] constexpr T& operator*() const {
        ensureNonNull();
        return *m_ptr;
    }

    void ensureNonNull() const {
        if (!m_ptr)
            throwException(ENullDeref("Dereferencing null pointer of type {}", Internal::typeName<T>()));
    }

    /**
     * @brief Explicit conversion to bool to check if the pointer is non-null.
     * @return True if the pointer is non-null, false otherwise.
     */
    [[nodiscard]] constexpr explicit operator bool() const noexcept {
        return m_ptr != nullptr;
    }

    /**
     * @brief Gets the raw pointer.
     * @return The raw pointer.
     */
    [[nodiscard]] constexpr T* get() const noexcept {
        return m_ptr;
    }

    /**
     * @brief Maps a lambda function over the pointed-to object if non-null.
     * @tparam F Type of the lambda function.
     * @param f Lambda function to apply to the pointed-to object.
     * @return std::optional containing the result of the lambda if the pointer is non-null, otherwise
     * std::nullopt.
     */
    template <typename F>
    [[nodiscard]] constexpr auto map(F&& f) const noexcept -> std::optional<std::invoke_result_t<F, T&>> {
        if (m_ptr) {
            return std::invoke(std::forward<F>(f), *m_ptr);
        }
        return std::nullopt;
    }

private:
    T* m_ptr = nullptr; ///< The wrapped raw pointer.
};

/**
 * @brief A utility structure to provide automatic singleton management.
 *
 * This struct manages the creation of a singleton instance of type `T` using `std::unique_ptr`.
 * The instance is lazily created upon the first access.
 *
 * @tparam T The type of the singleton instance to manage.
 */
template <typename T>
struct AutoSingleton {
    /**
     * @brief Provides access to the singleton instance of type `T`.
     *
     * This operator ensures the creation of the singleton instance (if not already created)
     * and returns a pointer to it.
     *
     * @return T* Pointer to the singleton instance of type `T`.
     */
    T* operator->() {
        static std::unique_ptr<T> instance{ new T{} };
        return instance.get();
    }

    /**
     * @brief Implicit conversion to pointer of type `T`.
     *
     * This conversion operator allows the `AutoSingleton` object to be used
     * as a pointer to the singleton instance of type `T`. This simplifies the
     * access to the singleton object without needing to call the `operator->()` explicitly.
     *
     * @return T* Pointer to the singleton instance of type `T`.
     */
    operator T*() noexcept {
        return operator->();
    }
};

template <typename T, typename Func>
std::optional<std::invoke_result_t<Func, const T&>> transformOptional(const std::optional<T>& opt,
                                                                      Func&& func) {
    if (opt.has_value()) {
        return std::optional<std::invoke_result_t<Func, const T&>>{ std::forward<Func>(func)(*opt) };
    }
    return std::nullopt;
}

template <typename T, typename Func>
std::optional<std::invoke_result_t<Func, T&&>> transformOptional(std::optional<T>&& opt, Func&& func) {
    if (opt.has_value()) {
        return std::optional<std::invoke_result_t<Func, T&&>>{ std::forward<Func>(func)(std::move(*opt)) };
    }
    return std::nullopt;
}

template <typename Enum>
std::underlying_type_t<Enum> to_underlying(Enum e)
    requires std::is_enum_v<Enum>
{
    return static_cast<std::underlying_type_t<Enum>>(e);
}

template <typename T>
concept IsOptional = std::same_as<T, std::optional<typename T::value_type>>;

namespace Internal {
template <typename T>
struct optional_value_type {
    using type = T;
};

template <IsOptional T>
struct optional_value_type<T> {
    using type = typename T::value_type;
};

template <typename T>
std::optional<T> wrapOptional(std::optional<T>&& value) {
    return std::move(value);
}

template <typename T>
const std::optional<T>& wrapOptional(const std::optional<T>& value) {
    return value;
}

template <typename T>
struct FakeOptional {
    T value;

    const T& operator*() const& noexcept {
        return value;
    }

    T&& operator*() && noexcept {
        return std::move(value);
    }

    constexpr operator bool() const noexcept {
        return true;
    }
};

template <typename T>
FakeOptional<T&&> wrapOptional(T&& value)
    requires(!IsOptional<std::remove_cvref_t<T>>)
{
    return FakeOptional<T&&>{ std::move(value) };
}

template <typename T>
FakeOptional<const T&> wrapOptional(const T& value)
    requires(!IsOptional<std::remove_cvref_t<T>>)
{
    return FakeOptional<const T&>{ value };
}

} // namespace Internal

template <typename... Types>
struct TypeList {};

} // namespace Brisk

template <std::size_t I, typename... T>
struct std::tuple_element<I, Brisk::TypeList<T...>> {
    using type = std::tuple_element_t<I, std::tuple<T...>>;
};

template <typename... T>
struct std::tuple_size<Brisk::TypeList<T...>> : std::integral_constant<size_t, sizeof...(T)> {};
