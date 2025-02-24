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

#include <cstdint>
#include <cstdlib>
#include <type_traits>
#include "Bytes.hpp"
#include "Reflection.hpp"

namespace Brisk {

/// Computes the CRC-32 checksum.
/// @param data The data to calculate the checksum for.
/// @param crc The initial CRC value (default is 0).
/// @return The CRC-32 checksum.
uint32_t crc32(BytesView data, uint32_t crc = 0);

/// Computes the CRC-32 checksum for a string.
/// @param data The string to calculate the checksum for.
/// @param crc The initial CRC value (default is 0).
/// @return The CRC-32 checksum.
inline uint32_t crc32(std::string_view data, uint32_t crc = 0) {
    return crc32(toBytesView(data), crc);
}

/// Computes a fast hash for strings and byte arrays.
/// @param data The data to hash.
/// @param seed The initial hash seed (default is 0).
/// @return The computed hash.
uint64_t fastHash(BytesView data, uint64_t seed = 0);

/// Computes a fast hash for basic string views.
/// @param data The string view to hash.
/// @param seed The initial hash seed (default is 0).
/// @return The computed hash.
template <typename C, typename Tr>
inline uint64_t fastHash(std::basic_string_view<C, Tr> data, uint64_t seed = 0) {
    return fastHash(toBytesView(data), seed);
}

/// Computes a fast hash for basic strings.
/// @param data The string to hash.
/// @param seed The initial hash seed (default is 0).
/// @return The computed hash.
template <typename C, typename Tr, typename Al>
inline uint64_t fastHash(const std::basic_string<C, Tr, Al>& data, uint64_t seed = 0) {
    return fastHash(toBytesView(data), seed);
}

/// Computes a fast hash for data types with a simple memory representation.
/// @tparam T Data type.
/// @param data The data to hash.
/// @param seed The initial hash seed (default is 0).
/// @return The computed hash.
template <typename T>
inline uint64_t fastHash(const T& data, uint64_t seed = 0)
    requires(simpleMemoryRepresentation<T>)
{
    return fastHash(asBytesView(data), seed);
}

template <size_t N>
inline uint64_t fastHash(const char (&data)[N], uint64_t seed = 0) {
    return fastHash(std::string_view(data), seed);
}

template <size_t N>
inline uint64_t fastHash(const char16_t (&data)[N], uint64_t seed = 0) {
    return fastHash(std::u16string_view(data), seed);
}

template <size_t N>
inline uint64_t fastHash(const char32_t (&data)[N], uint64_t seed = 0) {
    return fastHash(std::u32string_view(data), seed);
}

template <size_t N>
inline uint64_t fastHash(const wchar_t (&data)[N], uint64_t seed = 0) {
    return fastHash(std::wstring_view(data), seed);
}

namespace Internal {

/// Computes a fast hash for a tuple.
/// @param data The tuple to hash.
/// @param seed The initial hash seed.
/// @return The computed hash.
template <size_t... I, typename... T>
inline uint64_t fastHashTuple(size_constants<I...>, const std::tuple<T...>& data, uint64_t seed) {
    uint64_t hash = seed;
    (fastHashAccum(hash, std::get<I>(data)), ...);
    return hash;
}
} // namespace Internal

/// Computes a fast hash for tuples.
/// @param data The tuple to hash.
/// @param seed The initial hash seed (default is 0).
/// @return The computed hash.
template <typename... T>
inline uint64_t fastHash(const std::tuple<T...>& data, uint64_t seed = 0)
    requires(!simpleMemoryRepresentation<std::tuple<T...>>)
{
    return Internal::fastHashTuple(size_sequence<sizeof...(T)>{}, data, seed);
}

/// Updates the seed with the hash of a given data element.
/// @tparam T Data type.
/// @param seed The current hash seed.
/// @param data The data to hash and accumulate.
template <typename T>
inline void fastHashAccum(uint64_t& seed, const T& data) {
    seed = fastHash(data, seed);
}

/// Functor for computing fast hashes.
/// @tparam T Data type.
/// @param value The value to hash.
/// @return The computed hash.
struct FastHash {
    template <typename T>
    size_t operator()(const T& value) const {
        return fastHash(value);
    }
};

/// Functor for computing hashes of strings.
/// Supports transparent comparisons for heterogenous lookups.
struct StringHash {
    using is_transparent = void;

    size_t operator()(std::string_view s) const {
        return fastHash(s);
    }
};

namespace Internal {

/**
 * @brief Computes a hash for an object using its reflected fields.
 *
 * This function iterates over the fields of an object (as defined by its reflection metadata)
 * and accumulates a hash for each field.
 *
 * @tparam indices The indices of the fields in the reflection tuple.
 * @tparam T The type of the object being hashed.
 * @tparam Class The class type containing the fields.
 * @tparam FieldType The types of the fields being reflected.
 * @param seed The initial seed for the hash.
 * @param val The object whose fields are being hashed.
 * @param fields The tuple of `ReflectionField` objects representing the fields.
 * @return The accumulated hash value.
 */
template <size_t... indices, typename T, typename Class, typename... FieldType>
uint64_t reflectHash(uint64_t seed, size_constants<indices...>, const T& val,
                     const std::tuple<ReflectionField<Class, FieldType>...>& fields) {
    uint64_t hash = seed;
    (fastHashAccum(hash, val.*(std::get<indices>(fields).pointerToField)), ...);
    return hash;
}

} // namespace Internal

/**
 * @brief Computes a fast hash for an object using reflection metadata.
 *
 * This function calculates a hash for an object by hashing each of its fields, as defined
 * by its reflection metadata. The function is only enabled for types that do not have
 * a simple memory representation.
 *
 * @tparam T The type of the object being hashed.
 * @param val The object to hash.
 * @param seed The initial seed for the hash (default is 0).
 * @return The hash value of the object.
 */
template <HasReflection T>
uint64_t fastHash(const T& val, uint64_t seed = 0)
    requires(!simpleMemoryRepresentation<T>)
{
    constexpr static auto numFields = std::tuple_size_v<decltype(reflectionOf<T>())>;
    return Internal::reflectHash(seed, size_sequence<numFields>{}, val, reflectionOf<T>());
}

} // namespace Brisk
