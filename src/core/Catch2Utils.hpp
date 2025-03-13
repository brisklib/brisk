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
#include <catch2/catch_all.hpp>
#include <brisk/core/internal/Expected.hpp>
#include <fmt/format.h>
#include <brisk/core/Bytes.hpp>
#include <brisk/core/BasicTypes.hpp>
#include <brisk/core/IO.hpp>
#include <brisk/core/SIMD.hpp>

inline std::string unicodeChar(char32_t value) {
    if (static_cast<int32_t>(value) < 0x1'0000)
        return fmt::format("U+{:04X}", static_cast<int32_t>(value));
    else
        return fmt::format("U+{:08X}", static_cast<int32_t>(value));
}

namespace Catch {
template <typename T, typename E>
struct StringMaker<Brisk::expected<T, E>> {
    static std::string convert(const Brisk::expected<T, E>& value) {
        if constexpr (std::is_void_v<T>) {
            if (value)
                return fmt::to_string("(success)");
            else
                return fmt::to_string(value.error());
        } else {
            if (value)
                return fmt::to_string(value.value());
            else
                return fmt::to_string(value.error());
        }
    }
};

template <typename T, bool inclusive>
struct StringMaker<Brisk::Range<T, inclusive>> {
    static std::string convert(const Brisk::Range<T, inclusive>& value) {
        return fmt::format("{}..{}", value.min, value.max);
    }
};

template <typename T>
    requires(!std::is_array_v<T> &&
             !(::Catch::is_range<T>::value && !::Catch::Detail::IsStreamInsertable<T>::value) &&
             fmt::has_formatter<T, fmt::format_context>::value)
struct StringMaker<T> {
    static std::string convert(const T& value) {
        return fmt::to_string(value);
    }
};

template <typename T>
struct StringMaker<std::optional<T>> {
    static std::string convert(const std::optional<T>& value) {
        if (value.has_value())
            return Catch::Detail::stringify(value.value());
        else
            return "(nullopt)";
    }
};

template <>
struct StringMaker<std::vector<std::byte>> {
    static std::string convert(const std::vector<std::byte>& value) {
        return Brisk::toHex(value);
    }
};

template <>
struct StringMaker<Brisk::BytesView> {
    static std::string convert(Brisk::BytesView value) {
        return Brisk::toHex(value);
    }
};

template <>
struct StringMaker<char32_t> {
    static std::string convert(char32_t value) {
        return unicodeChar(value);
    }
};

template <typename T, size_t N>
struct StringMaker<Brisk::SIMD<T, N>> {
    static std::string convert(Brisk::SIMD<T, N> value) {
        return fmt::format("{}", fmt::join(std::span<const T>(value.data(), N), ","));
    }
};
} // namespace Catch

template <>
struct fmt::formatter<char32_t> : fmt::formatter<std::string> {
    template <typename FormatContext>
    auto format(const char32_t& value, FormatContext& ctx) const {
        return fmt::formatter<std::string>::format(unicodeChar(value), ctx);
    }
};

namespace Catch {
namespace Matchers {

template <typename T, size_t N>
class SIMDWithinMatcher final : public MatcherBase<Brisk::SIMD<T, N>> {
public:
    SIMDWithinMatcher(const Brisk::SIMD<T, N>& target, double margin = 0.001)
        : m_target(target), m_margin(margin) {}

    bool match(const Brisk::SIMD<T, N>& matchee) const override {
        Brisk::SIMD<T, N> absdiff = Brisk::abs(matchee - m_target);
        return Brisk::horizontalAll(Brisk::lt(absdiff, Brisk::SIMD<T, N>(m_margin)));
    }

    std::string describe() const override {
        return "is approx. equal to " + ::Catch::Detail::stringify(m_target);
    }

private:
    Brisk::SIMD<T, N> m_target;
    double m_margin;
};

template <typename T, size_t N>
SIMDWithinMatcher(Brisk::SIMD<T, N>, double) -> SIMDWithinMatcher<T, N>;
template <typename T, size_t N>
SIMDWithinMatcher(Brisk::SIMD<T, N>) -> SIMDWithinMatcher<T, N>;

} // namespace Matchers
} // namespace Catch
