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
#include <brisk/core/Localization.hpp>

namespace Brisk {

static SimpleLocale simpleLocale;
Rc<const Locale> locale = Rc<Locale>(&simpleLocale, [](auto) {});

const std::string& SimpleLocale::translate(std::string_view key) const noexcept {
    auto it = table.find(key);
    if (it != table.end()) {
        return it->second;
    }
    it =
        table.insert(it, std::pair<const std::string, std::string>{ key, Internal::stripLocaleContext(key) });
    return it->second;
}

void SimpleLocale::addTranslation(std::string_view key, std::string value) {
    table.insert_or_assign(std::string(key), std::move(value));
}

void SimpleLocale::removeTranslation(std::string_view key) {
    auto it = table.find(key);
    if (it != table.end()) {
        table.erase(it);
    }
}

void SimpleLocale::clear() {
    table.clear();
}
} // namespace Brisk
