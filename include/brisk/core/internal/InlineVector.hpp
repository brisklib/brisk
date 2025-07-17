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
#include <cstdint>
#include <type_traits>
#include <stdexcept>
#include <algorithm>
#include "Throw.hpp"

namespace Brisk {

namespace Internal {

#if defined _LIBCPP_VERSION && _LIBCPP_VERSION < 170000
template <typename It1, typename It2, typename Cmp>
constexpr auto lexicographical_compare_three_way(It1 f1, It1 l1, It2 f2, It2 l2, Cmp cmp)
    -> decltype(cmp(*f1, *f2)) {

    bool empty1 = (f1 == l1);
    bool empty2 = (f2 == l2);
    for (; !empty1 && !empty2; empty1 = (++f1 == l1), empty2 = (++f2 == l2))
        if (auto c = cmp(*f1, *f2); c != 0)
            return c;

    return !empty1   ? std::strong_ordering::greater
           : !empty2 ? std::strong_ordering::less
                     : std::strong_ordering::equal;
}
#endif
} // namespace Internal

/**
 * @brief A resizeable vector-like container with fixed capacity.
 *
 * The `inline_vector` class is a container that stores a fixed number of elements
 * of type `T`. It provides similar functionalities to a standard vector, but it is
 * designed to operate with a fixed capacity determined at compile time.
 *
 * The class enforces that the type `T` is trivially copyable, movable, and destructible,
 * and ensures that the size `N` is within valid bounds. It supports basic operations
 * such as element access, size retrieval, and iteration.
 *
 * @tparam T The type of the elements stored in the vector.
 * @tparam N The maximum number of elements that the vector can hold.
 *
 * @note The class is designed to be efficient in terms of memory and performance
 * by storing elements in a contiguous array.
 *
 * @throw std::range_error If operations exceed the defined bounds (e.g., accessing
 * an index out of range, or initializing with more elements than allowed).
 */
template <typename T, size_t N>
struct inline_vector {
    static_assert(N > 0 && N <= INT32_MAX);
    static_assert(std::is_trivially_copy_constructible_v<T>);
    static_assert(std::is_trivially_copy_assignable_v<T>);
    static_assert(std::is_trivially_move_assignable_v<T>);
    static_assert(std::is_trivially_move_constructible_v<T>);
    static_assert(std::is_trivially_destructible_v<T>);

    using size_type        = size_t;
    using stored_size_type = std::conditional_t<(N >= UINT16_MAX), uint32_t,
                                                std::conditional_t<(N >= UINT8_MAX), uint16_t, uint8_t>>;
    using value_type       = T;
    using pointer          = T*;
    using const_pointer    = const T*;
    using reference        = T&;
    using const_reference  = const T&;
    using iterator         = pointer;
    using const_iterator   = const_pointer;

    constexpr inline_vector() noexcept : m_size(0) {}

    constexpr ~inline_vector() noexcept                               = default;

    constexpr inline_vector(const inline_vector&) noexcept            = default;
    constexpr inline_vector(inline_vector&&) noexcept                 = default;
    constexpr inline_vector& operator=(const inline_vector&) noexcept = default;
    constexpr inline_vector& operator=(inline_vector&&) noexcept      = default;

    [[noreturn]] static void throw_range_error(const char* msg) {
        throwException(std::range_error(msg));
    }

    constexpr inline_vector(size_type initial_size) : m_size(initial_size) {
        if (initial_size > N) {
            throw_range_error("inline_vector: invalid initial_size");
        }
    }

    constexpr inline_vector(size_type initial_size, T initial_value) : m_size(initial_size) {
        if (initial_size > N) {
            throw_range_error("inline_vector: invalid initial_size");
        }
        std::fill_n(begin(), initial_size, initial_value);
    }

    constexpr inline_vector(std::initializer_list<T> list) : inline_vector(list.begin(), list.end()) {}

    template <typename Iter>
    constexpr inline_vector(Iter first, Iter last) {
        iterator dest = begin();
        size_t i      = 0;
        for (; i < N && first != last; ++i)
            *dest++ = *first++;
        if (first != last) {
            throw_range_error("inline_vector: too many items");
        }
        m_size = i;
    }

    constexpr const_reference at(size_type index) const {
        if (index >= m_size) {
            throw_range_error("inline_vector: invalid index");
        }
        return m_values[index];
    }

    constexpr reference at(size_type index) {
        if (index >= m_size) {
            throw_range_error("inline_vector: invalid index");
        }
        return m_values[index];
    }

    constexpr const_reference operator[](size_type index) const noexcept {
        return m_values[index];
    }

    constexpr reference operator[](size_type index) noexcept {
        return m_values[index];
    }

    constexpr size_type size() const noexcept {
        return m_size;
    }

    constexpr pointer data() noexcept {
        return m_values;
    }

    constexpr const_pointer data() const noexcept {
        return m_values;
    }

    constexpr bool empty() const noexcept {
        return m_size == 0;
    }

    constexpr iterator begin() noexcept {
        return m_values;
    }

    constexpr iterator end() noexcept {
        return m_values + m_size;
    }

    constexpr const_iterator begin() const noexcept {
        return m_values;
    }

    constexpr const_iterator end() const noexcept {
        return m_values + m_size;
    }

    constexpr const_iterator cbegin() const noexcept {
        return m_values;
    }

    constexpr const_iterator cend() const noexcept {
        return m_values + m_size;
    }

    constexpr reference front() noexcept {
        return m_values[0];
    }

    constexpr reference back() noexcept {
        return m_values[m_size - 1];
    }

    constexpr const_reference front() const noexcept {
        return m_values[0];
    }

    constexpr const_reference back() const noexcept {
        return m_values[m_size - 1];
    }

    constexpr void push_back(T value) {
        if (m_size == N) {
            throw_range_error("inline_vector: vector is full");
        }
        m_values[m_size++] = value;
    }

    constexpr bool operator==(const inline_vector& other) const
        noexcept(noexcept(std::declval<T>() == std::declval<T>())) {
        return std::equal(begin(), end(), other.begin(), other.end());
    }

    constexpr std::strong_ordering operator<=>(const inline_vector& other) const
        noexcept(noexcept(std::declval<T>() <=> std::declval<T>())) {
#if defined _LIBCPP_VERSION && _LIBCPP_VERSION < 170000
        return Internal::lexicographical_compare_three_way(begin(), end(), other.begin(), other.end(),
                                                           std::compare_three_way{});
#else
        return std::lexicographical_compare_three_way(begin(), end(), other.begin(), other.end(),
                                                      std::compare_three_way{});
#endif
    }

    T m_values[N]{};
    stored_size_type m_size;
};
} // namespace Brisk
