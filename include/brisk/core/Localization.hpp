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

#include <brisk/core/Hash.hpp>
#include <brisk/core/internal/Constants.hpp>
#include <brisk/core/internal/FixedString.hpp>
#include <fmt/format.h>
#include <unordered_map>

namespace Brisk {

/**
 * @class Locale
 * @brief Abstract base class for localization.
 *
 * This class defines the interface for translating keys into localized strings.
 */
class Locale {
public:
    /**
     * @brief Translates a given key into a localized string.
     * @param key The key to translate.
     * @return A reference to the localized string.
     */
    virtual const std::string& translate(std::string_view key) const noexcept = 0;
};

/**
 * @brief External reference to the current locale.
 */
extern Rc<const Locale> locale;

namespace Internal {

/**
 * @brief Strips the locale context from a translation key.
 *
 * This function removes any context information from a translation key
 * that is separated by "||".
 *
 * @param key The translation key potentially containing context.
 * @return A string view to the stripped key.
 */
constexpr std::string_view stripLocaleContext(std::string_view key) {
    size_t p = key.find("||");
    if (p == std::string_view::npos) {
        return key;
    }
    return key.substr(0, p);
}
} // namespace Internal

/**
 * @class SimpleLocale
 * @brief A concrete implementation of the Locale class.
 *
 * This class provides methods to manage translations in a simple way,
 * including adding, removing, and clearing translations.
 */
class SimpleLocale final : public Locale {
public:
    /**
     * @brief Translates a given key into a localized string.
     * @param key The key to translate.
     * @return A reference to the localized string.
     */
    const std::string& translate(std::string_view key) const noexcept final;

    /**
     * @brief Removes a translation entry.
     * @param key The key for the translation to remove.
     */
    void removeTranslation(std::string_view key);

    /**
     * @brief Clears all translations.
     */
    void clear();

    /**
     * @brief Adds a new translation entry.
     * @param key The key for the translation.
     * @param value The localized string corresponding to the key.
     */
    void addTranslation(std::string_view key, std::string value);

private:
    mutable std::unordered_map<std::string, std::string, StringHash, std::equal_to<>> table;
};

/**
 * @tparam key A fixed string representing a translation key.
 * @struct LocaleFormatString
 * @brief A structure for formatting localized strings with arguments.
 *
 * This structure provides functionality for formatting strings
 * that may contain placeholders.
 */
template <Internal::FixedString key>
struct LocaleFormatString {

    /**
     * @brief Returns the stripped translation key.
     * @return A string view to the stripped key.
     */
    static constexpr std::string_view str() {
        return Internal::stripLocaleContext(key.string());
    }

    /**
     * @brief Checks the format arguments at compile-time.
     *
     * This function checks if the provided arguments are compatible
     * with the format string.
     *
     * @tparam Args The types of the arguments to check.
     */
    template <typename... Args>
    static consteval void checkFormatArgs() {
        [[maybe_unused]] fmt::format_string<Args...> fmt(str());
    }

    /**
     * @brief Formats a localized string with the provided arguments.
     *
     * @tparam Args The types of the arguments.
     * @param args The arguments to format into the string.
     * @return The formatted localized string.
     */
    template <typename... Args>
    std::string operator()(Args&&... args) const {
        checkFormatArgs<Args...>();
        return fmt::format(fmt::runtime(locale->translate(str())), std::forward<Args>(args)...);
    }
};

/**
 * @brief User-defined literal for translating a fixed string.
 *
 * This operator allows using the _tr literal to easily translate
 * fixed strings.
 *
 * @tparam s The fixed string to translate.
 * @return The translated string.
 *
 * @code
 * std::string greeting = "hello"_tr;  // Translates the key "hello" to the localized string.
 * std::cout << greeting;  // Output: "Hola" if the locale is Spanish.
 * @endcode
 */
template <Internal::FixedString s>
inline std::string operator""_tr() noexcept {
    return locale->translate(s.string());
}

/**
 * @brief User-defined literal for formatting a localized string.
 *
 * This operator allows using the _trfmt literal to create a
 * LocaleFormatString for fixed strings.
 *
 * @tparam s The fixed string for formatting.
 * @return A LocaleFormatString object.
 *
 * @code
 * constexpr auto welcomeMessage = "welcome_msg"_trfmt;  // Creates a LocaleFormatString for "welcome_msg".
 * std::string formattedMessage = welcomeMessage("John");  // Formats the message with the name "John".
 * std::cout << formattedMessage;  // Output: "Welcome, John!" if the translation is "Welcome, {0}!".
 * @endcode
 */
template <Internal::FixedString s>
constexpr LocaleFormatString<s> operator""_trfmt() noexcept {
    return {};
}

} // namespace Brisk
