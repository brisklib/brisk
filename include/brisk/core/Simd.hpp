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

#include <bit>
#include <array>
#include <stdint.h>
#include <stdlib.h>
#include <type_traits>
#include <algorithm>
#include <cmath>
#include "Math.hpp"
#include "BasicTypes.hpp"

#if 0
#if defined(_M_IX86) || defined(__i386__) || defined(_M_X64) || defined(__x86_64__) || defined(__wasm)
#define SIMD_USE_X86
#define SIMD_USE_SIMD
#include <x86intrin.h>
#elif defined(__arm__) || defined(__arm64__) || defined(_M_ARM) || defined(__aarch64__)
#define SIMD_USE_ARM
#define SIMD_USE_SIMD
#include <arm_neon.h>
#endif
#endif

namespace Brisk {

namespace SimdInternal {} // namespace SimdInternal

/**
 * @brief Concept to define the types compatible with SIMD operations.
 *
 * The SimdCompatible concept ensures that only floating-point or integral types
 * (excluding `bool`) are accepted as SIMD data types.
 *
 * @tparam T Type to be checked.
 */
template <typename T>
concept SimdCompatible = (std::is_floating_point_v<T> || std::is_integral_v<T>) && !std::is_same_v<T, bool>;

/**
 * @brief A template class that represents a SIMD (Single Instruction Multiple Data) type.
 *
 * The SIMD class provides various utility functions and operations for working with
 * arrays of SIMD-compatible types. It supports element-wise arithmetic, comparisons,
 * shuffling, and more.
 *
 * @tparam T The data type contained in the SIMD array (must satisfy SimdCompatible).
 * @tparam N The number of elements in the SIMD array.
 */
template <SimdCompatible T, size_t N>
struct alignas(std::bit_ceil(N) * sizeof(T)) Simd {
    static_assert(N >= 1 && N <= 16);

    /// Default constructor that initializes the SIMD array to zeros.
    constexpr Simd() noexcept : m_data{} {}

    /// Copy constructor.
    constexpr Simd(const Simd&) noexcept = default;

    /**
     * @brief Constructs a SIMD from another SIMD of possibly different type.
     *
     * @tparam U The type of the other SIMD.
     * @param value The other SIMD to copy values from.
     */
    template <SimdCompatible U>
    explicit constexpr Simd(Simd<U, N> value) noexcept {
        for (size_t i = 0; i < N; i++) {
            m_data[i] = static_cast<T>(value.m_data[i]);
        }
    }

    /**
     * @brief Variadic constructor that initializes the SIMD with a list of values.
     *
     * @tparam Args The types of the values.
     * @param values The values to initialize the SIMD.
     */
    template <std::convertible_to<T>... Args>
    constexpr Simd(Args... values) noexcept
        requires(sizeof...(Args) == N)
        : m_data{ static_cast<T>(values)... } {}

    /**
     * @brief Broadcast constructor that initializes all elements with the same value.
     *
     * @param value The value to broadcast.
     */
    constexpr Simd(T value) noexcept
        requires(N > 1)
    {
        for (size_t i = 0; i < N; i++) {
            m_data[i] = value;
        }
    }

    /// Returns the size of the SIMD array.
    static constexpr size_t size() noexcept {
        return N;
    }

    /**
     * @brief Access operator for reading elements.
     *
     * @param i Index of the element.
     * @return T The element at index `i`.
     */
    constexpr T operator[](size_t i) const noexcept {
        return m_data[i];
    }

    /**
     * @brief Access operator for modifying elements.
     *
     * @param i Index of the element.
     * @return T& Reference to the element at index `i`.
     */
    constexpr T& operator[](size_t i) noexcept {
        return m_data[i];
    }

    /**
     * @brief Static function to read data from an array into a Simd.
     *
     * @param data Pointer to the data array.
     * @return Simd<T, N> A Simd object populated with the array data.
     */
    static Simd<T, N> read(const T* data) noexcept {
        Simd<T, N> result;
        memcpy(result.data(), data, sizeof(T) * N);
        return result;
    }

    /**
     * @brief Writes the contents of the Simd to an array.
     *
     * @param data Pointer to the destination array.
     */
    void write(T* data) noexcept {
        memcpy(data, this->data(), sizeof(T) * N);
    }

    /**
     * @brief Shuffles the elements of the SIMD according to the provided indices.
     *
     * @tparam indices The indices to shuffle the elements by.
     * @return Simd<T, sizeof...(indices)> A new SIMD with the shuffled elements.
     */
    template <size_t... indices>
    constexpr Simd<T, sizeof...(indices)> shuffle(size_constants<indices...>) const noexcept {
        return Simd<T, sizeof...(indices)>{ m_data[indices]... };
    }

    /**
     * @brief Returns the first Nout elements of the SIMD.
     *
     * @tparam Nout The number of elements to return.
     * @return Simd<T, Nout> A new SIMD containing the first Nout elements.
     */
    template <size_t Nout>
    constexpr Simd<T, Nout> firstn(size_constant<Nout> = {}) const noexcept
        requires((Nout <= N))
    {
        return shuffle(size_sequence<Nout>{});
    }

    /**
     * @brief Returns the last Nout elements of the SIMD.
     *
     * @tparam Nout The number of elements to return.
     * @return Simd<T, Nout> A new SIMD containing the last Nout elements.
     */
    template <size_t Nout>
    constexpr Simd<T, Nout> lastn(size_constant<Nout> = {}) const noexcept
        requires((Nout <= N))
    {
        return shuffle(size_sequence<Nout>{} + size_constant<N - Nout>{});
    }

    /**
     * @brief Splits the SIMD in half, returning the lower half.
     *
     * @return Simd<T, N/2> A SIMD containing the lower half of elements.
     */
    Simd<T, N / 2> low() const noexcept
        requires(N % 2 == 0)
    {
        return shuffle(size_sequence<N / 2>{});
    }

    /**
     * @brief Splits the SIMD in half, returning the upper half.
     *
     * @return Simd<T, N/2> A SIMD containing the upper half of elements.
     */
    Simd<T, N / 2> high() const noexcept
        requires(N % 2 == 0)
    {
        return shuffle(size_sequence<N / 2>{} + size_constant<N / 2>{});
    }

    /// Returns the first element in the SIMD.
    T front() const noexcept {
        return m_data[0];
    }

    /// Returns a reference to the first element in the SIMD.
    T& front() noexcept {
        return m_data[0];
    }

    /// Returns the last element in the SIMD.
    T back() const noexcept {
        return m_data[N - 1];
    }

    /// Returns a reference to the last element in the SIMD.
    T& back() noexcept {
        return m_data[N - 1];
    }

    /// Returns a pointer to the underlying data.
    const T* data() const noexcept {
        return m_data;
    }

    /// Returns a mutable pointer to the underlying data.
    T* data() noexcept {
        return m_data;
    }

    /// Underlying storage for the SIMD elements.
    T m_data[(N)];
};

/**
 * @brief Template deduction guide for SIMD.
 *
 * This deduction guide allows for automatic template deduction based on constructor arguments.
 */
template <typename... Args>
Simd(Args&&...) -> Simd<std::common_type_t<Args...>, sizeof...(Args)>;

/**
 * @typedef SimdMask
 * @brief Alias for a boolean mask used with SIMD operations.
 *
 * @tparam N The size of the mask.
 */
template <size_t N>
using SimdMask = std::array<bool, N>;

template <size_t N>
constexpr auto maskToBits(SimdMask<N> mask) {
    static_assert(N <= 32);

    using ReturnType = std::conditional_t<(N > 16), uint32_t, std::conditional_t<(N > 8), uint16_t, uint8_t>>;

    ReturnType result = 0;
    for (size_t bit = 0; bit < N; ++bit) {
        if (mask[bit])
            result |= 1 << bit;
    }
    return result;
}

/**
 * @typedef SimdIndices
 * @brief Alias for an index array used in SIMD shuffling operations.
 *
 * @tparam N The number of indices.
 */
template <size_t N>
using SimdIndices = std::array<size_t, N>;

/**
 * @brief Performs element-wise addition between two SIMD objects.
 *
 * @tparam T The data type contained in the SIMD array.
 * @tparam N The number of elements in the SIMD array.
 * @param lhs The left-hand side SIMD object.
 * @param rhs The right-hand side SIMD object.
 * @return Simd<T, N>& A reference to the updated SIMD object after addition.
 */
template <typename T, size_t N>
constexpr Simd<T, N>& operator+=(Simd<T, N>& lhs, Simd<T, N> rhs) noexcept {
    for (size_t i = 0; i < N; i++) {
        lhs.m_data[i] += rhs.m_data[i];
    }
    return lhs;
}

/**
 * @brief Adds two SIMD objects element-wise.
 *
 * @tparam T The data type contained in the SIMD array.
 * @tparam N The number of elements in the SIMD array.
 * @param lhs The left-hand side SIMD object.
 * @param rhs The right-hand side SIMD object.
 * @return Simd<T, N> A new SIMD object containing the result of the addition.
 */
template <typename T, size_t N>
constexpr Simd<T, N> operator+(Simd<T, N> lhs, Simd<T, N> rhs) noexcept {
    Simd<T, N> result(lhs);
    return result += rhs;
}

/**
 * @brief Adds a scalar value to each element of a SIMD object.
 *
 * @tparam T The data type contained in the SIMD array.
 * @tparam N The number of elements in the SIMD array.
 * @param lhs The SIMD object.
 * @param rhs The scalar value.
 * @return Simd<T, N>& A reference to the updated SIMD object after addition.
 */
template <typename T, size_t N>
constexpr Simd<T, N>& operator+=(Simd<T, N>& lhs, T rhs) noexcept {
    return lhs += Simd<T, N>(rhs);
}

/**
 * @brief Adds a scalar value to each element of a SIMD object.
 *
 * @tparam T The data type contained in the SIMD array.
 * @tparam N The number of elements in the SIMD array.
 * @param lhs The SIMD object.
 * @param rhs The scalar value.
 * @return Simd<T, N> A new SIMD object containing the result of the addition.
 */
template <typename T, size_t N>
constexpr Simd<T, N> operator+(Simd<T, N> lhs, T rhs) noexcept {
    return lhs + Simd<T, N>(rhs);
}

/**
 * @brief Adds a SIMD object to a scalar value, element-wise.
 *
 * @tparam T The data type contained in the SIMD array.
 * @tparam N The number of elements in the SIMD array.
 * @param lhs The scalar value.
 * @param rhs The SIMD object.
 * @return Simd<T, N> A new SIMD object containing the result of the addition.
 */
template <typename T, size_t N>
constexpr Simd<T, N> operator+(T lhs, Simd<T, N> rhs) noexcept {
    return Simd<T, N>(lhs) + rhs;
}

/**
 * @brief Performs element-wise subtraction between two SIMD objects.
 *
 * @tparam T The data type contained in the SIMD array.
 * @tparam N The number of elements in the SIMD array.
 * @param lhs The left-hand side SIMD object.
 * @param rhs The right-hand side SIMD object.
 * @return Simd<T, N>& A reference to the updated SIMD object after subtraction.
 */
template <typename T, size_t N>
constexpr Simd<T, N>& operator-=(Simd<T, N>& lhs, Simd<T, N> rhs) noexcept {
    for (size_t i = 0; i < N; i++) {
        lhs.m_data[i] -= rhs.m_data[i];
    }
    return lhs;
}

/**
 * @brief Subtracts two SIMD objects element-wise.
 *
 * @tparam T The data type contained in the SIMD array.
 * @tparam N The number of elements in the SIMD array.
 * @param lhs The left-hand side SIMD object.
 * @param rhs The right-hand side SIMD object.
 * @return Simd<T, N> A new SIMD object containing the result of the subtraction.
 */
template <typename T, size_t N>
constexpr Simd<T, N> operator-(Simd<T, N> lhs, Simd<T, N> rhs) noexcept {
    Simd<T, N> result(lhs);
    return result -= rhs;
}

/**
 * @brief Subtracts a scalar value from each element of a SIMD object.
 *
 * @tparam T The data type contained in the SIMD array.
 * @tparam N The number of elements in the SIMD array.
 * @param lhs The SIMD object.
 * @param rhs The scalar value.
 * @return Simd<T, N>& A reference to the updated SIMD object after subtraction.
 */
template <typename T, size_t N>
constexpr Simd<T, N>& operator-=(Simd<T, N>& lhs, T rhs) noexcept {
    return lhs -= Simd<T, N>(rhs);
}

/**
 * @brief Subtracts a scalar value from each element of a SIMD object.
 *
 * @tparam T The data type contained in the SIMD array.
 * @tparam N The number of elements in the SIMD array.
 * @param lhs The SIMD object.
 * @param rhs The scalar value.
 * @return Simd<T, N> A new SIMD object containing the result of the subtraction.
 */
template <typename T, size_t N>
constexpr Simd<T, N> operator-(Simd<T, N> lhs, T rhs) noexcept {
    return lhs - Simd<T, N>(rhs);
}

/**
 * @brief Subtracts each element of a SIMD object from a scalar value, element-wise.
 *
 * @tparam T The data type contained in the SIMD array.
 * @tparam N The number of elements in the SIMD array.
 * @param lhs The scalar value.
 * @param rhs The SIMD object.
 * @return Simd<T, N> A new SIMD object containing the result of the subtraction.
 */
template <typename T, size_t N>
constexpr Simd<T, N> operator-(T lhs, Simd<T, N> rhs) noexcept {
    return Simd<T, N>(lhs) - rhs;
}

/**
 * @brief Performs element-wise multiplication between two SIMD objects.
 *
 * @tparam T The data type contained in the SIMD array.
 * @tparam N The number of elements in the SIMD array.
 * @param lhs The left-hand side SIMD object.
 * @param rhs The right-hand side SIMD object.
 * @return Simd<T, N>& A reference to the updated SIMD object after multiplication.
 */
template <typename T, size_t N>
constexpr Simd<T, N>& operator*=(Simd<T, N>& lhs, Simd<T, N> rhs) noexcept {
    for (size_t i = 0; i < N; i++) {
        lhs.m_data[i] *= rhs.m_data[i];
    }
    return lhs;
}

/**
 * @brief Multiplies two SIMD objects element-wise.
 *
 * @tparam T The data type contained in the SIMD array.
 * @tparam N The number of elements in the SIMD array.
 * @param lhs The left-hand side SIMD object.
 * @param rhs The right-hand side SIMD object.
 * @return Simd<T, N> A new SIMD object containing the result of the multiplication.
 */
template <typename T, size_t N>
constexpr Simd<T, N> operator*(Simd<T, N> lhs, Simd<T, N> rhs) noexcept {
    Simd<T, N> result(lhs);
    return result *= rhs;
}

/**
 * @brief Multiplies each element of a SIMD object by a scalar value.
 *
 * @tparam T The data type contained in the SIMD array.
 * @tparam N The number of elements in the SIMD array.
 * @param lhs The SIMD object.
 * @param rhs The scalar value.
 * @return Simd<T, N>& A reference to the updated SIMD object after multiplication.
 */
template <typename T, size_t N>
constexpr Simd<T, N>& operator*=(Simd<T, N>& lhs, T rhs) noexcept {
    return lhs *= Simd<T, N>(rhs);
}

/**
 * @brief Multiplies each element of a SIMD object by a scalar value.
 *
 * @tparam T The data type contained in the SIMD array.
 * @tparam N The number of elements in the SIMD array.
 * @param lhs The SIMD object.
 * @param rhs The scalar value.
 * @return Simd<T, N> A new SIMD object containing the result of the multiplication.
 */
template <typename T, size_t N>
constexpr Simd<T, N> operator*(Simd<T, N> lhs, T rhs) noexcept {
    return lhs * Simd<T, N>(rhs);
}

/**
 * @brief Multiplies a scalar value by each element of a SIMD object, element-wise.
 *
 * @tparam T The data type contained in the SIMD array.
 * @tparam N The number of elements in the SIMD array.
 * @param lhs The scalar value.
 * @param rhs The SIMD object.
 * @return Simd<T, N> A new SIMD object containing the result of the multiplication.
 */
template <typename T, size_t N>
constexpr Simd<T, N> operator*(T lhs, Simd<T, N> rhs) noexcept {
    return Simd<T, N>(lhs) * rhs;
}

/**
 * @brief Performs element-wise division between two SIMD objects.
 *
 * @tparam T The data type contained in the SIMD array.
 * @tparam N The number of elements in the SIMD array.
 * @param lhs The left-hand side SIMD object.
 * @param rhs The right-hand side SIMD object.
 * @return Simd<T, N>& A reference to the updated SIMD object after division.
 */
template <typename T, size_t N>
constexpr Simd<T, N>& operator/=(Simd<T, N>& lhs, Simd<T, N> rhs) noexcept {
    for (size_t i = 0; i < N; i++) {
        lhs.m_data[i] /= rhs.m_data[i];
    }
    return lhs;
}

/**
 * @brief Divides two SIMD objects element-wise.
 *
 * @tparam T The data type contained in the SIMD array.
 * @tparam N The number of elements in the SIMD array.
 * @param lhs The left-hand side SIMD object.
 * @param rhs The right-hand side SIMD object.
 * @return Simd<T, N> A new SIMD object containing the result of the division.
 */
template <typename T, size_t N>
constexpr Simd<T, N> operator/(Simd<T, N> lhs, Simd<T, N> rhs) noexcept {
    Simd<T, N> result(lhs);
    return result /= rhs;
}

/**
 * @brief Divides each element of a SIMD object by a scalar value.
 *
 * @tparam T The data type contained in the SIMD array.
 * @tparam N The number of elements in the SIMD array.
 * @param lhs The SIMD object.
 * @param rhs The scalar value.
 * @return Simd<T, N>& A reference to the updated SIMD object after division.
 */
template <typename T, size_t N>
constexpr Simd<T, N>& operator/=(Simd<T, N>& lhs, T rhs) noexcept {
    return lhs /= Simd<T, N>(rhs);
}

/**
 * @brief Divides each element of a SIMD object by a scalar value.
 *
 * @tparam T The data type contained in the SIMD array.
 * @tparam N The number of elements in the SIMD array.
 * @param lhs The SIMD object.
 * @param rhs The scalar value.
 * @return Simd<T, N> A new SIMD object containing the result of the division.
 */
template <typename T, size_t N>
constexpr Simd<T, N> operator/(Simd<T, N> lhs, T rhs) noexcept {
    return lhs / Simd<T, N>(rhs);
}

/**
 * @brief Divides a scalar value by each element of a SIMD object, element-wise.
 *
 * @tparam T The data type contained in the SIMD array.
 * @tparam N The number of elements in the SIMD array.
 * @param lhs The scalar value.
 * @param rhs The SIMD object.
 * @return Simd<T, N> A new SIMD object containing the result of the division.
 */
template <typename T, size_t N>
constexpr Simd<T, N> operator/(T lhs, Simd<T, N> rhs) noexcept {
    return Simd<T, N>(lhs) / rhs;
}

/**
 * @brief Unary plus operator for SIMD objects (returns the object unchanged).
 *
 * @tparam T The data type contained in the SIMD array.
 * @tparam N The number of elements in the SIMD array.
 * @param lhs The SIMD object.
 * @return Simd<T, N> The input SIMD object, unchanged.
 */
template <typename T, size_t N>
constexpr Simd<T, N> operator+(Simd<T, N> lhs) noexcept {
    return lhs;
}

/**
 * @brief Unary negation operator for SIMD objects (negates each element).
 *
 * @tparam T The data type contained in the SIMD array.
 * @tparam N The number of elements in the SIMD array.
 * @param lhs The SIMD object.
 * @return Simd<T, N> A new SIMD object with negated elements.
 */
template <typename T, size_t N>
constexpr Simd<T, N> operator-(Simd<T, N> lhs) noexcept {
    for (size_t i = 0; i < N; i++) {
        lhs.m_data[i] = -lhs.m_data[i];
    }
    return lhs;
}

/**
 * @brief Equality comparison between two SIMD objects.
 *
 * @tparam T The data type contained in the SIMD array.
 * @tparam N The number of elements in the SIMD array.
 * @param lhs The left-hand side SIMD object.
 * @param rhs The right-hand side SIMD object.
 * @return true if all elements are equal, false otherwise.
 */
template <typename T, size_t N>
constexpr bool operator==(Simd<T, N> lhs, Simd<T, N> rhs) noexcept {
    for (size_t i = 0; i < N; i++) {
        if (lhs.m_data[i] != rhs.m_data[i])
            return false;
    }
    return true;
}

/**
 * @brief Inequality comparison between two SIMD objects.
 *
 * @tparam T The data type contained in the SIMD array.
 * @tparam N The number of elements in the SIMD array.
 * @param lhs The left-hand side SIMD object.
 * @param rhs The right-hand side SIMD object.
 * @return true if any element differs, false otherwise.
 */
template <typename T, size_t N>
constexpr bool operator!=(Simd<T, N> lhs, Simd<T, N> rhs) noexcept {
    return !operator==(lhs, rhs);
}

/**
 * @brief Compares two SIMD objects for equality, element-wise.
 *
 * @tparam T The data type contained in the SIMD array.
 * @tparam N The number of elements in the SIMD array.
 * @param lhs The left-hand side SIMD object.
 * @param rhs The right-hand side SIMD object.
 * @return SimdMask<N> A mask indicating which elements are equal.
 */
template <typename T, size_t N>
constexpr SimdMask<N> eq(Simd<T, N> lhs, Simd<T, N> rhs) {
    SimdMask<N> result{};
    for (size_t i = 0; i < N; i++) {
        result[i] = lhs.m_data[i] == rhs.m_data[i];
    }
    return result;
}

/**
 * @brief Compares two SIMD objects for inequality, element-wise.
 *
 * @tparam T The data type contained in the SIMD array.
 * @tparam N The number of elements in the SIMD array.
 * @param lhs The left-hand side SIMD object.
 * @param rhs The right-hand side SIMD object.
 * @return SimdMask<N> A mask indicating which elements are not equal.
 */
template <typename T, size_t N>
constexpr SimdMask<N> ne(Simd<T, N> lhs, Simd<T, N> rhs) {
    SimdMask<N> result{};
    for (size_t i = 0; i < N; i++) {
        result[i] = lhs.m_data[i] != rhs.m_data[i];
    }
    return result;
}

/**
 * @brief Compares two SIMD objects to determine if the left-hand side is less than the right-hand side,
 * element-wise.
 *
 * @tparam T The data type contained in the SIMD array.
 * @tparam N The number of elements in the SIMD array.
 * @param lhs The left-hand side SIMD object.
 * @param rhs The right-hand side SIMD object.
 * @return SimdMask<N> A mask indicating which elements in the left-hand side are less than the corresponding
 * elements in the right-hand side.
 */
template <typename T, size_t N>
constexpr SimdMask<N> lt(Simd<T, N> lhs, Simd<T, N> rhs) {
    SimdMask<N> result{};
    for (size_t i = 0; i < N; i++) {
        result[i] = lhs.m_data[i] < rhs.m_data[i];
    }
    return result;
}

/**
 * @brief Compares two SIMD objects to determine if the left-hand side is greater than the right-hand side,
 * element-wise.
 *
 * @tparam T The data type contained in the SIMD array.
 * @tparam N The number of elements in the SIMD array.
 * @param lhs The left-hand side SIMD object.
 * @param rhs The right-hand side SIMD object.
 * @return SimdMask<N> A mask indicating which elements in the left-hand side are greater than the
 * corresponding elements in the right-hand side.
 */
template <typename T, size_t N>
constexpr SimdMask<N> gt(Simd<T, N> lhs, Simd<T, N> rhs) {
    SimdMask<N> result{};
    for (size_t i = 0; i < N; i++) {
        result[i] = lhs.m_data[i] > rhs.m_data[i];
    }
    return result;
}

/**
 * @brief Compares two SIMD objects to determine if the left-hand side is less than or equal to the right-hand
 * side, element-wise.
 *
 * @tparam T The data type contained in the SIMD array.
 * @tparam N The number of elements in the SIMD array.
 * @param lhs The left-hand side SIMD object.
 * @param rhs The right-hand side SIMD object.
 * @return SimdMask<N> A mask indicating which elements in the left-hand side are less than or equal to the
 * corresponding elements in the right-hand side.
 */
template <typename T, size_t N>
constexpr SimdMask<N> le(Simd<T, N> lhs, Simd<T, N> rhs) {
    SimdMask<N> result{};
    for (size_t i = 0; i < N; i++) {
        result[i] = lhs.m_data[i] <= rhs.m_data[i];
    }
    return result;
}

/**
 * @brief Compares two SIMD objects to determine if the left-hand side is greater than or equal to the
 * right-hand side, element-wise.
 *
 * @tparam T The data type contained in the SIMD array.
 * @tparam N The number of elements in the SIMD array.
 * @param lhs The left-hand side SIMD object.
 * @param rhs The right-hand side SIMD object.
 * @return SimdMask<N> A mask indicating which elements in the left-hand side are greater than or equal to the
 * corresponding elements in the right-hand side.
 */
template <typename T, size_t N>
constexpr SimdMask<N> ge(Simd<T, N> lhs, Simd<T, N> rhs) {
    SimdMask<N> result{};
    for (size_t i = 0; i < N; i++) {
        result[i] = lhs.m_data[i] >= rhs.m_data[i];
    }
    return result;
}

/**
 * @brief Performs a bitwise OR between two SIMD masks.
 *
 * @tparam N The number of elements in the mask.
 * @param lhs The left-hand side SIMD mask.
 * @param rhs The right-hand side SIMD mask.
 * @return SimdMask<N> A mask with the result of the OR operation applied element-wise.
 */
template <size_t N>
constexpr SimdMask<N> maskOr(SimdMask<N> lhs, SimdMask<N> rhs) {
    SimdMask<N> result{};
    for (size_t i = 0; i < N; i++) {
        result[i] = lhs[i] || rhs[i];
    }
    return result;
}

/**
 * @brief Performs a bitwise AND between two SIMD masks.
 *
 * @tparam N The number of elements in the mask.
 * @param lhs The left-hand side SIMD mask.
 * @param rhs The right-hand side SIMD mask.
 * @return SimdMask<N> A mask with the result of the AND operation applied element-wise.
 */
template <size_t N>
constexpr SimdMask<N> maskAnd(SimdMask<N> lhs, SimdMask<N> rhs) {
    SimdMask<N> result{};
    for (size_t i = 0; i < N; i++) {
        result[i] = lhs[i] && rhs[i];
    }
    return result;
}

/**
 * @brief Determines if all elements of a SIMD mask are true.
 *
 * @tparam N The number of elements in the mask.
 * @param value The SIMD mask to check.
 * @return true if all elements are true, false otherwise.
 */
template <size_t N>
constexpr bool horizontalAll(SimdMask<N> value) {
    return [&]<size_t... indices>(size_constants<indices...>) {
        return (value[indices] && ...);
    }(size_sequence<N>{});
}

/**
 * @brief Determines if any element of a SIMD mask is true.
 *
 * @tparam N The number of elements in the mask.
 * @param value The SIMD mask to check.
 * @return true if any element is true, false otherwise.
 */
template <size_t N>
constexpr bool horizontalAny(SimdMask<N> value) {
    return [&]<size_t... indices>(size_constants<indices...>) {
        return (value[indices] || ...);
    }(size_sequence<N>{});
}

/**
 * @brief Selects elements from two SIMD objects based on a mask.
 *
 * @tparam T The data type contained in the SIMD array.
 * @tparam N The number of elements in the SIMD array.
 * @param mask The SIMD mask used for selection.
 * @param trueval The SIMD object whose elements are selected where the mask is true.
 * @param falseval The SIMD object whose elements are selected where the mask is false.
 * @return Simd<T, N> The resulting SIMD object with selected elements.
 */
template <typename T, size_t N>
constexpr Simd<T, N> select(SimdMask<N> mask, Simd<T, N> trueval, Simd<T, N> falseval) {
    Simd<T, N> result{};
    for (size_t i = 0; i < N; i++) {
        result[i] = mask[i] ? trueval.m_data[i] : falseval.m_data[i];
    }
    return result;
}

/**
 * @brief Shuffles elements in the SIMD object based on specified indices.
 *
 * @tparam indices The indices to shuffle the elements.
 * @tparam T The data type contained in the SIMD array.
 * @tparam N The number of elements in the SIMD array.
 * @param source The SIMD object to shuffle.
 * @return Simd<T, sizeof...(indices)> The shuffled SIMD object.
 */
template <size_t... indices, typename T, size_t N>
constexpr Simd<T, sizeof...(indices)> shuffle(Simd<T, N> source) noexcept {
    return Simd<T, sizeof...(indices)>{ { source.m_data[indices]... } };
}

/**
 * @brief Concatenates multiple SIMD objects into a single SIMD object.
 *
 * @tparam T The data type contained in the SIMD arrays.
 * @tparam Nin The sizes of the SIMD arrays to concatenate.
 * @param source The SIMD objects to concatenate.
 * @return Simd<T, (Nin + ...)> The concatenated SIMD object.
 */
template <typename T, size_t... Nin>
constexpr Simd<T, (Nin + ...)> concat(const Simd<T, Nin>&... source) noexcept {
    Simd<T, (Nin + ...)> result;
    size_t j = 0;
    (([&](auto x) {
         for (size_t i = 0; i < x.size(); ++i) {
             result.m_data[j++] = x.m_data[i];
         }
     })(source),
     ...);
    return result;
}

/**
 * @brief Returns the element-wise minimum between two SIMD objects.
 *
 * @tparam T The data type contained in the SIMD array.
 * @tparam N The number of elements in the SIMD array.
 * @param lhs The left-hand side SIMD object.
 * @param rhs The right-hand side SIMD object.
 * @return Simd<T, N> A SIMD object containing the element-wise minimum values.
 */
template <typename T, size_t N>
constexpr Simd<T, N> min(Simd<T, N> lhs, Simd<T, N> rhs) noexcept {
    Simd<T, N> result;
    for (size_t i = 0; i < N; i++) {
        result.m_data[i] = std::min(lhs.m_data[i], rhs.m_data[i]);
    }
    return result;
}

/**
 * @brief Returns the element-wise maximum between two SIMD objects.
 *
 * @tparam T The data type contained in the SIMD array.
 * @tparam N The number of elements in the SIMD array.
 * @param lhs The left-hand side SIMD object.
 * @param rhs The right-hand side SIMD object.
 * @return Simd<T, N> A SIMD object containing the element-wise maximum values.
 */
template <typename T, size_t N>
constexpr Simd<T, N> max(Simd<T, N> lhs, Simd<T, N> rhs) noexcept {
    Simd<T, N> result;
    for (size_t i = 0; i < N; i++) {
        result.m_data[i] = std::max(lhs.m_data[i], rhs.m_data[i]);
    }
    return result;
}

/**
 * @brief Clamps each element of the SIMD object between a lower and upper bound.
 *
 * @tparam T The data type contained in the SIMD array.
 * @tparam N The number of elements in the SIMD array.
 * @param x The SIMD object to clamp.
 * @param low The lower bound.
 * @param high The upper bound.
 * @return Simd<T, N> A SIMD object with elements clamped between the low and high values.
 */
template <typename T, size_t N>
constexpr Simd<T, N> clamp(Simd<T, N> x, Simd<T, N> low, Simd<T, N> high) noexcept {
    Simd<T, N> result;
    for (size_t i = 0; i < N; i++) {
        result.m_data[i] = std::clamp(x.m_data[i], low.m_data[i], high.m_data[i]);
    }
    return result;
}

/**
 * @brief Blends two SIMD objects based on a mask.
 *
 * @tparam mask The mask for blending, where 1 means selecting from val1, 0 means selecting from val0.
 * @tparam T The data type contained in the SIMD array.
 * @tparam N The number of elements in the SIMD array.
 * @param val0 The first SIMD object.
 * @param val1 The second SIMD object.
 * @return Simd<T, N> The blended SIMD object.
 */
template <int... mask, typename T, size_t N>
constexpr Simd<T, N> blend(Simd<T, N> val0, Simd<T, N> val1) noexcept
    requires(sizeof...(mask) == N)
{
    return [&]<size_t... indices>(std::index_sequence<indices...>) {
        return Simd<T, N>{ (mask ? val1.m_data[indices] : val0.m_data[indices])... };
    }(std::make_index_sequence<N>{});
}

namespace Internal {
/**
 * @brief Computes the absolute value of a number, optimized for constant evaluation.
 *
 * @tparam T The type of the input value.
 * @param x The input value.
 * @return T The absolute value of the input.
 */
template <typename T>
constexpr T constexprAbs(T x) {
    if (std::is_constant_evaluated())
        return x < T(0) ? -x : x;
    else
        return std::abs(x);
}

/**
 * @brief Copies the sign from one value to another, optimized for constant evaluation.
 *
 * @tparam T The type of the input values.
 * @param x The value whose magnitude is used.
 * @param s The value whose sign is copied.
 * @return T The value of x with the sign of s.
 */
template <typename T>
constexpr T constexprCopysign(T x, T s) {
    if (std::is_constant_evaluated())
        return s < 0 ? -constexprAbs(x) : constexprAbs(x);
    else
        return std::copysign(x, s);
}
} // namespace Internal

/**
 * @brief Computes the minimum element of the SIMD object.
 *
 * @tparam T The data type contained in the SIMD array.
 * @tparam N The number of elements in the SIMD array.
 * @param lhs The SIMD object.
 * @return T The minimum value across all elements in the SIMD object.
 */
template <typename T, size_t N>
constexpr T horizontalMin(Simd<T, N> lhs) noexcept {
    T result = lhs[0];
    for (size_t i = 1; i < N; i++) {
        result = std::min(result, lhs.m_data[i]);
    }
    return result;
}

/**
 * @brief Computes the maximum element of the SIMD object.
 *
 * @tparam T The data type contained in the SIMD array.
 * @tparam N The number of elements in the SIMD array.
 * @param lhs The SIMD object.
 * @return T The maximum value across all elements in the SIMD object.
 */
template <typename T, size_t N>
constexpr T horizontalMax(Simd<T, N> lhs) noexcept {
    T result = lhs[0];
    for (size_t i = 1; i < N; i++) {
        result = std::max(result, lhs.m_data[i]);
    }
    return result;
}

/**
 * @brief Computes the maximum absolute element of the SIMD object.
 *
 * @tparam T The data type contained in the SIMD array.
 * @tparam N The number of elements in the SIMD array.
 * @param lhs The SIMD object.
 * @return T The maximum absolute value across all elements in the SIMD object.
 */
template <typename T, size_t N>
constexpr T horizontalAbsMax(Simd<T, N> lhs) noexcept {
    T result = Internal::constexprAbs(lhs[0]);
    for (size_t i = 1; i < N; i++) {
        result = std::max(result, Internal::constexprAbs(lhs.m_data[i]));
    }
    return result;
}

/**
 * @brief Computes the sum of all elements in the SIMD object.
 *
 * @tparam T The data type contained in the SIMD array.
 * @tparam N The number of elements in the SIMD array.
 * @param lhs The SIMD object.
 * @return T The sum of all elements in the SIMD object.
 */
template <typename T, size_t N>
constexpr T horizontalSum(Simd<T, N> lhs) noexcept {
    T result = lhs[0];
    for (size_t i = 1; i < N; i++) {
        result += lhs.m_data[i];
    }
    return result;
}

/**
 * @brief Computes the root mean square (RMS) of the elements in the SIMD object.
 *
 * @tparam T The data type contained in the SIMD array.
 * @tparam N The number of elements in the SIMD array.
 * @param lhs The SIMD object.
 * @return T The RMS value across all elements in the SIMD object.
 */
template <typename T, size_t N>
constexpr T horizontalRMS(Simd<T, N> lhs) noexcept {
    T result = sqr(lhs[0]);
    for (size_t i = 1; i < N; i++) {
        result += sqr(lhs.m_data[i]);
    }
    return std::sqrt(result);
}

/**
 * @brief Computes the dot product of two SIMD objects.
 *
 * @tparam T The data type contained in the SIMD array.
 * @tparam N The number of elements in the SIMD array.
 * @param lhs The left-hand side SIMD object.
 * @param rhs The right-hand side SIMD object.
 * @return T The dot product of the two SIMD objects.
 */
template <typename T, size_t N>
constexpr T dot(Simd<T, N> lhs, Simd<T, N> rhs) noexcept {
    T result = lhs[0] * rhs[0];
    for (size_t i = 1; i < N; i++) {
        result += lhs.m_data[i] * rhs.m_data[i];
    }
    return result;
}

/**
 * @brief Linearly interpolates between two SIMD objects.
 *
 * @tparam T The data type contained in the SIMD array.
 * @tparam N The number of elements in the SIMD array.
 * @param t The interpolation factor, typically between 0 and 1.
 * @param lhs The left-hand side SIMD object.
 * @param rhs The right-hand side SIMD object.
 * @return Simd<T, N> The interpolated SIMD object.
 */
template <typename T, size_t N>
constexpr Simd<T, N> mix(float t, Simd<T, N> lhs, Simd<T, N> rhs) {
    using SIMDf = Simd<std::common_type_t<T, float>, N>;
    return Simd<T, N>(SIMDf(lhs) * SIMDf(1 - t) + SIMDf(rhs) * SIMDf(t));
}

/**
 * @brief Raises each element of the SIMD object to the power of the corresponding element in another SIMD
 * object.
 *
 * @tparam T1 The data type of the base SIMD array.
 * @tparam T2 The data type of the exponent SIMD array.
 * @tparam N The number of elements in the SIMD arrays.
 * @tparam U The resulting type after the power operation.
 * @param lhs The base SIMD object.
 * @param rhs The exponent SIMD object.
 * @return Simd<U, N> A SIMD object containing the element-wise power results.
 */
template <typename T1, typename T2, size_t N,
          typename U = decltype(std::pow(std::declval<T1>(), std::declval<T2>()))>
constexpr Simd<U, N> pow(const Simd<T1, N>& lhs, const Simd<T2, N>& rhs) {
    Simd<U, N> result{};
    for (size_t i = 0; i < N; i++) {
        result[i] = std::pow(lhs.m_data[i], rhs.m_data[i]);
    }
    return result;
}

/**
 * @brief Computes the element-wise absolute value of a SIMD object.
 *
 * @tparam T The type of the elements in the SIMD object.
 * @tparam N The number of elements in the SIMD object.
 * @param val The SIMD object.
 * @return Simd<T, N> A new SIMD object with absolute values of each element.
 */
template <typename T, size_t N>
constexpr Simd<T, N> abs(Simd<T, N> val) {
    Simd<T, N> result{};
    for (size_t i = 0; i < N; i++) {
        result[i] = Internal::constexprAbs(val.m_data[i]);
    }
    return result;
}

/**
 * @brief Computes the element-wise square root of a SIMD object.
 *
 * @tparam T The type of the elements in the SIMD object.
 * @tparam N The number of elements in the SIMD object.
 * @param val The SIMD object.
 * @return Simd<T, N> A new SIMD object with square roots of each element.
 */
template <typename T, size_t N>
constexpr Simd<T, N> sqrt(Simd<T, N> val) {
    Simd<T, N> result{};
    for (size_t i = 0; i < N; i++) {
        result[i] = std::sqrt(val.m_data[i]);
    }
    return result;
}

template <typename T, size_t N>
constexpr Simd<T, N> cbrt(Simd<T, N> val) {
    Simd<T, N> result{};
    for (size_t i = 0; i < N; i++) {
        result[i] = std::cbrt(val.m_data[i]);
    }
    return result;
}

/**
 * @brief Computes the sine and cosine for each element of a SIMD object.
 *
 * @note The number of elements (N) must be even.
 *
 * @tparam T The type of the elements in the SIMD object.
 * @tparam N The number of elements in the SIMD object.
 * @param val The SIMD object.
 * @return Simd<T, N> A new SIMD object with alternating sine and cosine values.
 */
template <typename T, size_t N>
constexpr Simd<T, N> sincos(Simd<T, N> val)
    requires(N % 2 == 0)
{
    Simd<T, N> result{};
    for (size_t i = 0; i < N; i += 2) {
        result[i]     = std::sin(val.m_data[i]);
        result[i + 1] = std::cos(val.m_data[i + 1]);
    }
    return result;
}

/**
 * @brief Computes the cosine and sine for each element of a SIMD object.
 *
 * @note The number of elements (N) must be even.
 *
 * @tparam T The type of the elements in the SIMD object.
 * @tparam N The number of elements in the SIMD object.
 * @param val The SIMD object.
 * @return Simd<T, N> A new SIMD object with alternating cosine and sine values.
 */
template <typename T, size_t N>
constexpr Simd<T, N> cossin(Simd<T, N> val)
    requires(N % 2 == 0)
{
    Simd<T, N> result{};
    for (size_t i = 0; i < N; i += 2) {
        result[i]     = std::cos(val.m_data[i]);
        result[i + 1] = std::sin(val.m_data[i + 1]);
    }
    return result;
}

/**
 * @brief Copies the sign from one SIMD object to another element-wise.
 *
 * @tparam T The type of the elements in the SIMD objects.
 * @tparam N The number of elements in the SIMD objects.
 * @param lhs The SIMD object providing the magnitude.
 * @param rhs The SIMD object providing the sign.
 * @return Simd<T, N> A new SIMD object where each element has the magnitude of lhs and the sign of rhs.
 */
template <typename T, size_t N>
constexpr Simd<T, N> copysign(Simd<T, N> lhs, Simd<T, N> rhs) {
    Simd<T, N> result{};
    for (size_t i = 0; i < N; i++) {
        result[i] = Internal::constexprCopysign(lhs.m_data[i], rhs.m_data[i]);
    }
    return result;
}

/**
 * @brief Rounds each element in the SIMD object to the nearest integer.
 *
 * @tparam T The type of the elements in the SIMD object.
 * @tparam N The number of elements in the SIMD object.
 * @param val The SIMD object.
 * @return Simd<T, N> A new SIMD object with each element rounded.
 */
template <typename T, size_t N>
constexpr Simd<T, N> round(Simd<T, N> val) {
    Simd<T, N> result{};
    for (size_t i = 0; i < N; i++) {
        result[i] = std::round(val.m_data[i]);
    }
    return result;
}

/**
 * @brief Computes the element-wise floor (round down) of a SIMD object.
 *
 * @tparam T The type of the elements in the SIMD object.
 * @tparam N The number of elements in the SIMD object.
 * @param val The SIMD object.
 * @return Simd<T, N> A new SIMD object with floored values.
 */
template <typename T, size_t N>
constexpr Simd<T, N> floor(Simd<T, N> val) {
    Simd<T, N> result{};
    for (size_t i = 0; i < N; i++) {
        result[i] = std::floor(val.m_data[i]);
    }
    return result;
}

/**
 * @brief Computes the element-wise ceiling (round up) of a SIMD object.
 *
 * @tparam T The type of the elements in the SIMD object.
 * @tparam N The number of elements in the SIMD object.
 * @param val The SIMD object.
 * @return Simd<T, N> A new SIMD object with ceiling values.
 */
template <typename T, size_t N>
constexpr Simd<T, N> ceil(Simd<T, N> val) {
    Simd<T, N> result{};
    for (size_t i = 0; i < N; i++) {
        result[i] = std::ceil(val.m_data[i]);
    }
    return result;
}

/**
 * @brief Truncates each element in the SIMD object to its integer part.
 *
 * @tparam T The type of the elements in the SIMD object.
 * @tparam N The number of elements in the SIMD object.
 * @param val The SIMD object.
 * @return Simd<T, N> A new SIMD object with truncated values.
 */
template <typename T, size_t N>
constexpr Simd<T, N> trunc(Simd<T, N> val) {
    Simd<T, N> result{};
    for (size_t i = 0; i < N; i++) {
        result[i] = std::trunc(val.m_data[i]);
    }
    return result;
}

/**
 * @brief Swaps adjacent elements in the SIMD object.
 *
 * @note The number of elements (N) must be even.
 *
 * @tparam T The type of the elements in the SIMD object.
 * @tparam N The number of elements in the SIMD object.
 * @param val The SIMD object.
 * @return Simd<T, N> A new SIMD object with adjacent elements swapped.
 */
template <typename T, size_t N>
constexpr Simd<T, N> swapAdjacent(Simd<T, N> val)
    requires(N % 2 == 0)
{
    return val.shuffle(size_sequence<N>{} ^ size_constant<1>{});
}

/**
 * @brief Repeats the elements of a SIMD object multiple times.
 *
 * @tparam Ncount The number of times to repeat the elements.
 * @tparam T The type of the elements in the SIMD object.
 * @tparam N The number of elements in the SIMD object.
 * @param val The SIMD object.
 * @return Simd<T, Ncount * N> A new SIMD object with repeated elements.
 */
template <size_t Ncount, typename T, size_t N>
constexpr Simd<T, Ncount * N> repeat(Simd<T, N> val) {
    return val.shuffle(size_sequence<Ncount * N>{} % size_constant<N>{});
}

/**
 * @brief Rescales the elements of a SIMD object, either between different ranges or floating-point types.
 *
 * @note This overload is for rescaling with floating-point values.
 *
 * @tparam Tout The output type of the elements.
 * @tparam Mout The maximum value for the output range.
 * @tparam Min The minimum value for the input range.
 * @tparam Tin The input type of the elements.
 * @tparam N The number of elements in the SIMD object.
 * @param value The SIMD object to rescale.
 * @return Simd<Tout, N> A rescaled SIMD object.
 */
template <SimdCompatible Tout, int Mout, int Min, SimdCompatible Tin, size_t N>
constexpr Simd<Tout, N> rescale(Simd<Tin, N> value)
    requires(Mout != Min && (std::is_floating_point<Tin>::value || std::is_floating_point<Tout>::value))
{
    using Tcommon      = std::common_type_t<Tin, Tout>;
    Simd<Tcommon, N> x = static_cast<Simd<Tcommon, N>>(value) * Tcommon(Mout) / Tcommon(Min);
    if constexpr (!std::is_floating_point_v<Tout>) {
        x += Tcommon(0.5);
        x = clamp(x, Simd<Tcommon, N>(std::numeric_limits<Tout>::min()),
                  Simd<Tcommon, N>(std::numeric_limits<Tout>::max()));
    }
    return static_cast<Simd<Tout, N>>(x);
}

/**
 * @brief Rescales the elements of a SIMD object, either between different ranges or integral types.
 *
 * @note This overload is for rescaling with integral values.
 *
 * @tparam Tout The output type of the elements.
 * @tparam Mout The maximum value for the output range.
 * @tparam Min The minimum value for the input range.
 * @tparam Tin The input type of the elements.
 * @tparam N The number of elements in the SIMD object.
 * @param value The SIMD object to rescale.
 * @return Simd<Tout, N> A rescaled SIMD object.
 */
template <SimdCompatible Tout, int Mout, int Min, SimdCompatible Tin, size_t N>
constexpr Simd<Tout, N> rescale(Simd<Tin, N> value)
    requires(Mout != Min && !(std::is_floating_point<Tin>::value || std::is_floating_point<Tout>::value))
{
    using Tcommon =
        findIntegralType<std::numeric_limits<Tin>::min() * Mout, std::numeric_limits<Tin>::max() * Mout>;
    Simd<Tcommon, N> x = static_cast<Simd<Tcommon, N>>(value);
    if constexpr (Tcommon(std::max(Mout, Min)) % Tcommon(std::min(Mout, Min)) == 0) {
        constexpr Tcommon scale = std::max(Mout, Min) / std::min(Mout, Min);
        if constexpr (Mout > Min) {
            x = x * scale;
        } else {
            x = (x + scale / 2) / scale;
        }
    } else {
        x = (x * Tcommon(Mout) + Tcommon(Min) / 2) / Tcommon(Min);
    }
    if constexpr (Internal::fitsIntType<Tcommon>(std::numeric_limits<Tout>::max())) {
        x = min(x, Simd<Tcommon, N>(std::numeric_limits<Tout>::max()));
    }
    if constexpr (Internal::fitsIntType<Tcommon>(std::numeric_limits<Tout>::min())) {
        x = max(x, Simd<Tcommon, N>(std::numeric_limits<Tout>::min()));
    }
    return static_cast<Simd<Tout, N>>(static_cast<Simd<Tcommon, N>>(x));
}

/**
 * @brief Rescales a single value or a SIMD object when the input and output ranges are equal.
 *
 * @tparam Tout The output type of the value.
 * @tparam Mout The maximum value for the output range.
 * @tparam Min The minimum value for the input range.
 * @tparam Tin The input type of the value.
 * @return Simd<Tout, N> The rescaled value or SIMD object.
 */
template <SimdCompatible Tout, int Mout, int Min, SimdCompatible Tin, size_t N>
constexpr Simd<Tout, N> rescale(Simd<Tin, N> value)
    requires(Mout == Min)
{
    if constexpr (!std::is_floating_point_v<Tout>) {
        return static_cast<Simd<Tout, N>>(round(value));
    } else {
        return static_cast<Simd<Tout, N>>(value);
    }
}

/**
 * @brief Rescales a single value between two ranges.
 *
 * @tparam Tout The output type of the value.
 * @tparam Mout The maximum value for the output range.
 * @tparam Min The minimum value for the input range.
 * @tparam Tin The input type of the value.
 * @param value The value to rescale.
 * @return Tout The rescaled value.
 */
template <SimdCompatible Tout, int Mout, int Min, SimdCompatible Tin>
constexpr Tout rescale(Tin value) {
    return rescale<Tout, Mout, Min, Tin>(Simd<Tin, 1>(value)).front();
}

namespace Internal {
/**
 * @brief Byte-swaps a 16-bit unsigned integer.
 *
 * This constexpr function swaps the byte order of a 16-bit unsigned integer.
 * It can be used at compile time when the value is known at that time, but
 * switches to a platform-optimized version at runtime when needed.
 *
 * @param x The 16-bit unsigned integer to be byte-swapped.
 * @return The byte-swapped 16-bit unsigned integer.
 */
constexpr uint16_t byteswap(uint16_t x) {
#ifdef BRISK_GNU_ATTR
    return __builtin_bswap16(x); ///< Optimized GCC/Clang intrinsic for byte-swapping.
#elif defined _MSC_VER
    return std::rotr(x, 8); ///< MSVC implementation using rotate-right by 8 bits.
#endif
}

// Static assertion to ensure that the byte-swap works as expected at compile time.
static_assert(byteswap(static_cast<uint16_t>(0x1122)) == static_cast<uint16_t>(0x2211));

/**
 * @brief Byte-swaps a 32-bit unsigned integer.
 *
 * This constexpr function swaps the byte order of a 32-bit unsigned integer.
 * It can be used at compile time if the value is known, otherwise it switches
 * to a platform-optimized implementation at runtime.
 *
 * @param x The 32-bit unsigned integer to be byte-swapped.
 * @return The byte-swapped 32-bit unsigned integer.
 */
constexpr uint32_t byteswap(uint32_t x) {
#ifdef BRISK_GNU_ATTR
    return __builtin_bswap32(x); ///< Optimized GCC/Clang intrinsic for byte-swapping.
#elif defined _MSC_VER
    if (__builtin_is_constant_evaluated()) {
        // Compile-time byte-swapping using recursive bit-casting and swapping.
        auto a = std::bit_cast<std::array<uint16_t, 2>>(x);
        a      = { byteswap(a[1]), byteswap(a[0]) };
        return std::bit_cast<uint32_t>(a);
    } else {
        // Optimized runtime byte-swap using MSVC intrinsic.
        return _byteswap_ulong(x);
    }
#endif
}

// Static assertion to ensure that the byte-swap works as expected at compile time.
static_assert(byteswap(static_cast<uint32_t>(0x11223344)) == static_cast<uint32_t>(0x44332211));

/**
 * @brief Byte-swaps a 64-bit unsigned integer.
 *
 * This constexpr function swaps the byte order of a 64-bit unsigned integer.
 * It can be used at compile time if the value is known, otherwise it switches
 * to a platform-optimized implementation at runtime.
 *
 * @param x The 64-bit unsigned integer to be byte-swapped.
 * @return The byte-swapped 64-bit unsigned integer.
 */
constexpr uint64_t byteswap(uint64_t x) {
#ifdef BRISK_GNU_ATTR
    return __builtin_bswap64(x); ///< Optimized GCC/Clang intrinsic for byte-swapping.
#elif defined _MSC_VER
    if (__builtin_is_constant_evaluated()) {
        // Compile-time byte-swapping using recursive bit-casting and swapping.
        auto a = std::bit_cast<std::array<uint32_t, 2>>(x);
        a      = { byteswap(a[1]), byteswap(a[0]) };
        return std::bit_cast<uint64_t>(a);
    } else {
        // Optimized runtime byte-swap using MSVC intrinsic.
        return _byteswap_uint64(x);
    }
#endif
}

// Static assertion to ensure that the byte-swap works as expected at compile time.
static_assert(byteswap(static_cast<uint64_t>(0x1122334455667788)) ==
              static_cast<uint64_t>(0x8877665544332211));

} // namespace Internal

} // namespace Brisk
