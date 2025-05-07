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
#include <brisk/gui/Styles.hpp>

namespace Brisk {

Rules::Rules(std::initializer_list<Rule> rules) : rules(std::move(rules)) {
    sort();
}

Rules::Rules(std::vector<Rule> rules, bool doSort) : rules(std::move(rules)) {
    if (doSort) {
        sort();
    }
}

void Rules::sort() {
    std::reverse(rules.begin(), rules.end());
    std::stable_sort(rules.begin(), rules.end(), RuleCmpLess{});
    rules.erase(std::unique(rules.begin(), rules.end(), RuleCmpEq{}), rules.end());
}

Rules& Rules::merge(const Rules& other) {
    if (rules.empty()) {
        rules = other.rules;
        return *this;
    }
    auto first = rules.begin();
    for (const Rule& r : other.rules) {
        auto it = std::upper_bound(first, rules.end(), r, RuleCmpLess{}); // first >
        if (it == first || !RuleCmpEq{}(*std::prev(it), r)) {
            first = rules.insert(it, r); // insert
        } else {
            *std::prev(it) = r; // replace
            first          = it;
        }
    }
    return *this;
}

void Rules::applyTo(Widget* widget) const {
    Widget::StyleApplying styleApplying(widget);
    for (const Rule& r : rules) {
        r.applyTo(widget);
    }
}

void Stylesheet::stylize(Widget* widget, bool isRoot) const {
    Rules rules;
    for (const auto& ss : inherited) {
        BRISK_ASSERT(ss);
        ss->stylizeInternal(rules, widget, isRoot);
    }
    stylizeInternal(rules, widget, isRoot);
    rules.applyTo(widget);
    widget->m_reapplyStyle = [rules = std::move(rules)](Widget* self) {
        rules.applyTo(self);
    };
}

void Stylesheet::stylizeInternal(Rules& rules, Widget* widget, bool isRoot) const {
    for (const Style& style : *this) {
        if (style.selector.matches(widget, isRoot ? MatchFlags::IsRoot : MatchFlags::None)) {
            rules.merge(style.rules);
        }
    }
}
} // namespace Brisk
