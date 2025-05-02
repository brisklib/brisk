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
#include <vector>
#include <string>
#include "BasicTypes.hpp"
#include <fmt/format.h>
#include "internal/Optional.hpp"
#include <brisk/core/internal/Debug.hpp>

namespace Brisk {

/// @brief Converts string in Hex format to span of bytes
/// @returns number of bytes written or SIZE_MAX in case of error
[[nodiscard]] size_t fromHex(BytesMutableView data, std::string_view encoded);

/// @brief Converts span of bytes to string in Hex format
/// @returns number of characters written or SIZE_MAX in case of error
[[nodiscard]] size_t toHex(std::span<char> encoded, BytesView data, bool upperCase = true);

[[nodiscard]] std::optional<Bytes> fromHex(std::string_view encoded);
[[nodiscard]] std::string toHex(BytesView data, bool upperCase = true);

/// @brief Converts string in Base64 format to span of bytes
/// @returns number of bytes written or SIZE_MAX in case of error
[[nodiscard]] size_t fromBase64(BytesMutableView data, std::string_view encoded, bool urlSafe = false,
                                bool strict = false);

/// @brief Converts span of bytes to string in Base64 format
/// @returns number of characters written or SIZE_MAX in case of error
[[nodiscard]] size_t toBase64(std::span<char> encoded, BytesView data, bool urlSafe = false, bool pad = true);

[[nodiscard]] std::optional<Bytes> fromBase64(std::string_view encoded, bool urlSafe = false,
                                              bool strict = false);
[[nodiscard]] std::string toBase64(BytesView data, bool urlSafe = false, bool pad = true);

template <typename T>
T readFromBytes(BytesView data) {
    static_assert(std::is_trivially_copyable_v<T>);
    if (data.size() < sizeof(T))
        return T{};
    T result;
    memcpy(std::addressof(result), data.data(), sizeof(T));
    return result;
}

/// @brief Array of characters of fixed size
template <size_t Size>
struct CC {
    constexpr CC()          = default;
    constexpr CC(const CC&) = default;

    constexpr CC(const char (&str)[Size + 1]) {
        for (size_t i = 0; i < Size; i++)
            m_data[i] = static_cast<uint8_t>(str[i]);
    }

    constexpr bool operator==(const CC<Size>& other) const {
        for (size_t i = 0; i < Size; i++)
            if (m_data[i] != other.m_data[i])
                return false;
        return true;
    }

    constexpr bool operator!=(const CC<Size>& other) const {
        return !operator==(other);
    }

    constexpr bool operator<(const CC<Size>& other) const {
        for (size_t i = 0; i < Size; i++) {
            if (m_data[i] < other.m_data[i])
                return true;
            if (m_data[i] > other.m_data[i])
                return false;
        }
        return false;
    }

    std::string toString() const {
        return std::string(reinterpret_cast<const char*>(m_data), Size);
    }

    std::string_view toStringView() const {
        return std::string_view(reinterpret_cast<const char*>(m_data), Size);
    }

    explicit operator uint16_t() const noexcept
        requires(Size == 2)
    {
        return readFromBytes<uint16_t>(BytesView(*this));
    }

    explicit operator uint32_t() const noexcept
        requires(Size == 4)
    {
        return readFromBytes<uint32_t>(BytesView(*this));
    }

    explicit operator uint64_t() const noexcept
        requires(Size == 8)
    {
        return readFromBytes<uint64_t>(BytesView(*this));
    }

    bool matches(CC other) const {
        for (size_t i = 0; i < Size; i++) {
            if (other.m_data[i] != '?' && m_data[i] != other.m_data[i])
                return false;
        }
        return true;
    }

    using value_type     = uint8_t;
    using size_type      = size_t;
    using pointer        = value_type*;
    using const_pointer  = const value_type*;
    using iterator       = pointer;
    using const_iterator = const_pointer;

    pointer data() {
        return m_data;
    }

    const_pointer data() const {
        return m_data;
    }

    size_t size() const {
        return Size;
    }

    iterator begin() {
        return data();
    }

    const_iterator begin() const {
        return data();
    }

    const_iterator cbegin() const {
        return data();
    }

    iterator end() {
        return data() + size();
    }

    const_iterator end() const {
        return data() + size();
    }

    const_iterator cend() const {
        return data() + size();
    }

    uint8_t m_data[Size];
};

using FourCC = CC<4>;

/**
 * @brief Array of bytes of fixed size.
 *
 * This array is used to store a fixed number of bytes.
 * The size of the array is defined at compile time and cannot be changed at runtime.
 */
template <size_t Size>
struct FixedBytes {
    constexpr FixedBytes()                  = default;
    constexpr FixedBytes(const FixedBytes&) = default;

    template <size_t N>
    constexpr FixedBytes(const char (&str)[N]) {
        static_assert(N == Size * 2 + 1);
        std::ignore = Brisk::fromHex({ data(), size() }, str);
    }

    constexpr FixedBytes(std::string_view str) {
        BRISK_ASSERT(str.size() == Size * 2);
        std::ignore = Brisk::fromHex({ data(), size() }, str);
    }

    static std::optional<FixedBytes> fromHex(std::string_view str) {
        FixedBytes result;
        if (Brisk::fromHex({ result.data(), result.size() }, str) == Size)
            return result;
        else
            return std::nullopt;
    }

    static std::optional<FixedBytes> fromBase64(std::string_view str, bool urlSafe = false,
                                                bool strict = false) {
        FixedBytes result;
        if (Brisk::fromBase64({ result.data(), result.size() }, str, urlSafe, strict) == Size)
            return result;
        else
            return std::nullopt;
    }

    constexpr bool operator==(const FixedBytes<Size>& other) const {
        for (size_t i = 0; i < Size; i++)
            if (m_data[i] != other.m_data[i])
                return false;
        return true;
    }

    constexpr bool operator!=(const FixedBytes<Size>& other) const {
        return !operator==(other);
    }

    std::string toHex(bool upperCase = true) const {
        return Brisk::toHex({ data(), size() }, upperCase);
    }

    std::string toBase64(bool urlSafe = false, bool pad = true) const {
        return Brisk::toBase64({ data(), size() }, urlSafe, pad);
    }

    using value_type     = std::byte;
    using size_type      = size_t;
    using pointer        = value_type*;
    using const_pointer  = const value_type*;
    using iterator       = pointer;
    using const_iterator = const_pointer;

    pointer data() {
        return m_data;
    }

    const_pointer data() const {
        return m_data;
    }

    size_t size() const {
        return Size;
    }

    iterator begin() {
        return data();
    }

    const_iterator begin() const {
        return data();
    }

    const_iterator cbegin() const {
        return data();
    }

    iterator end() {
        return data() + size();
    }

    const_iterator end() const {
        return data() + size();
    }

    const_iterator cend() const {
        return data() + size();
    }

    value_type m_data[Size];
};

template <size_t Size>
using UUID = FixedBytes<Size>;

template <size_t Size>
using FixedBits = FixedBytes<(Size + 7) / 8>;

using GUID      = UUID<16>;

} // namespace Brisk

template <size_t N>
struct fmt::formatter<Brisk::CC<N>> : fmt::formatter<std::string_view> {
    template <typename FormatContext>
    auto format(const Brisk::CC<N>& val, FormatContext& ctx) const {
        std::string_view str = val.toStringView();
        return fmt::formatter<std::string_view>::format(str, ctx);
    }
};

template <size_t N>
struct fmt::formatter<Brisk::FixedBytes<N>> : fmt::formatter<std::string> {
    template <typename FormatContext>
    auto format(const Brisk::FixedBytes<N>& val, FormatContext& ctx) const {
        std::string str = val.toHex();
        return fmt::formatter<std::string>::format(str, ctx);
    }
};

template <size_t N>
struct std::hash<Brisk::CC<N>> {
    std::size_t operator()(const Brisk::CC<N>& value) const {
        return static_cast<std::size_t>(fastHash(value));
    }
};

template <size_t N>
struct std::hash<Brisk::FixedBytes<N>> {
    std::size_t operator()(const Brisk::FixedBytes<N>& value) const {
        return static_cast<std::size_t>(fastHash(value));
    }
};

namespace fmt {
template <size_t Size, typename Char>
struct range_format_kind<Brisk::FixedBytes<Size>, Char>
    : std::integral_constant<range_format, range_format::disabled> {};
} // namespace fmt
