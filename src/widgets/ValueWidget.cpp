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
#include <brisk/widgets/ValueWidget.hpp>

namespace Brisk {

std::string defaultFormatter(double x) {
    if (x == 0 || !std::isfinite(x))
        return fmt::to_string(x);
    double lg = std::log10(std::abs(x));
    if (std::abs(lg) > 6)
        return fmt::format("{:.5e}", x);
    else
        return fmt::format("{:.{}f}", x, std::clamp(4 - (int)lg, 0, 7));
}

ValueWidget::ValueWidget(Construction construction, ArgumentsView<ValueWidget> args)
    : Widget{ construction, nullptr } {
    args.apply(this);
}

RC<Widget> ValueWidget::cloneThis() const {
    BRISK_CLONE_IMPLEMENTATION
}

void ValueWidget::onConstructed() {
    onChanged();
}

void ValueWidget::onChanged() {}

static double roundTo(double x, double r) {
    return std::round(x / r) * r;
}

void ValueWidget::setValue(double value) {
    value = std::clamp(value, m_minimum, m_maximum);
    if (m_snap && m_step > 0) {
        value = roundTo(value - m_minimum, m_step) + m_minimum;
    }
    m_value = value;
}

void ValueWidget::onChangedParams() {
    setValue(m_value);
}

double ValueWidget::getNormValue() const {
    return (m_value - m_minimum) / (m_maximum - m_minimum);
}

void ValueWidget::setNormValue(double normValue) {
    value = normValue * (m_maximum - m_minimum) + m_minimum;
}

void ValueWidget::pageUp(int amount) {
    shift(-amount, true);
}

void ValueWidget::pageDown(int amount) {
    shift(amount, true);
}

void ValueWidget::decrement(int amount) {
    shift(-amount);
}

void ValueWidget::increment(int amount) {
    shift(amount);
}

void ValueWidget::shift(int amount, bool page) {
    double step = amount * (page ? this->m_pageStep : this->m_step);
    value       = m_value + step;
}

void ValueWidget::startModifying() {
    if (m_modifying)
        return;
    m_modifying = true;
    if (m_hintFormatter)
        m_hint = m_hintFormatter(m_value);
}

void ValueWidget::stopModifying() {
    if (!m_modifying)
        return;
    m_modifying = false;
    m_hint      = {};
}

} // namespace Brisk
