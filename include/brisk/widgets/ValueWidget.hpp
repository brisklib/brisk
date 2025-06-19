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

#include <brisk/gui/Gui.hpp>

namespace Brisk {

std::string defaultFormatter(double x);

struct ValueFormatter {
    function<std::string(double)> fmtFn;

    bool operator==(const ValueFormatter&) const noexcept = default;

    ValueFormatter()                                      = default;

    template <std::invocable<double> Fn>
    ValueFormatter(Fn fmtFn) : fmtFn(std::move(fmtFn)) {}

    ValueFormatter(function<std::string(double)> fmtFn) : fmtFn(std::move(fmtFn)) {}

    template <std::convertible_to<std::string_view> Str>
    ValueFormatter(Str fmtStr)
        : fmtFn([fmtStr = std::move(fmtStr)](double x) {
              return fmt::format(fmt::runtime(fmtStr), x);
          }) {}

    [[nodiscard]] std::string operator()(double x) const {
        if (fmtFn)
            return fmtFn(x);
        return defaultFormatter(x);
    }

    [[nodiscard]] explicit operator bool() const noexcept {
        return fmtFn.operator bool();
    }
};

class WIDGET ValueWidget : public Widget {
    BRISK_DYNAMIC_CLASS(ValueWidget, Widget)
public:
    using Base = Widget;
    void increment(int amount = 1);
    void decrement(int amount = 1);
    void pageDown(int amount = 1);
    void pageUp(int amount = 1);

    void shift(int amount = 1, bool page = false);

protected:
    double m_value    = 0;
    double m_maximum  = +INT_MAX;
    double m_minimum  = -INT_MAX;
    double m_step     = 1;
    double m_pageStep = 10;
    bool m_snap       = false;
    bool m_modifying  = false;
    ValueFormatter m_hintFormatter;

    KeyModifiers m_wheelModifiers = KeyModifiers::Alt;

    virtual void onChanged();
    void startModifying();
    void stopModifying();

    void onConstructed() override;

    Ptr cloneThis() const override;

    explicit ValueWidget(Construction construction, ArgumentsView<ValueWidget> args);

private:
    double getNormValue() const noexcept;
    void setNormValue(double value);
    void setValue(double value);
    void onChangedParams();

public:
    static const auto& properties() noexcept {
        static constexpr tuplet::tuple props{
            /*0*/ Internal::PropFieldSetter{ &ValueWidget::m_value, &ValueWidget::setValue, "value" },
            /*1*/
            Internal::PropGetterSetter{ &ValueWidget::m_value, &ValueWidget::getNormValue,
                                        &ValueWidget::setNormValue, "normalizedValue" },
            /*2*/
            Internal::PropFieldNotify{ &ValueWidget::m_minimum, &ValueWidget::onChangedParams, "minimum" },
            /*3*/
            Internal::PropFieldNotify{ &ValueWidget::m_maximum, &ValueWidget::onChangedParams, "maximum" },
            /*4*/ Internal::PropFieldNotify{ &ValueWidget::m_step, &ValueWidget::onChangedParams, "step" },
            /*5*/ Internal::PropFieldNotify{ &ValueWidget::m_snap, &ValueWidget::onChangedParams, "snap" },
            /*6*/
            Internal::PropFieldNotify{ &ValueWidget::m_pageStep, &ValueWidget::onChangedParams, "pageStep" },
            /*7*/ Internal::PropField{ &ValueWidget::m_hintFormatter, "hintFormatter" },
            /*8*/ Internal::PropField{ &ValueWidget::m_wheelModifiers, "wheelModifiers" },
        };
        return props;
    }

public:
    BRISK_PROPERTIES_BEGIN
    Property<ValueWidget, double, 0> value;
    Property<ValueWidget, double, 1> normalizedValue;

    Property<ValueWidget, double, 2> minimum;
    Property<ValueWidget, double, 3> maximum;
    Property<ValueWidget, double, 4> step;
    Property<ValueWidget, bool, 5> snap;
    Property<ValueWidget, double, 6> pageStep;

    Property<ValueWidget, ValueFormatter, 7> hintFormatter;
    Property<ValueWidget, KeyModifiers, 8> wheelModifiers;
    BRISK_PROPERTIES_END
};

template <typename T>
void applier(ValueWidget* target, ArgVal<Tag::Named<"value">, T> value) {
    target->value = value.value;
}

template <typename T>
void applier(ValueWidget* target, ArgVal<Tag::Named<"maximum">, T> value) {
    target->maximum = value.value;
}

template <typename T>
void applier(ValueWidget* target, ArgVal<Tag::Named<"minimum">, T> value) {
    target->minimum = value.value;
}

template <typename T>
void applier(ValueWidget* target, ArgVal<Tag::Named<"step">, T> value) {
    target->step = value.value;
}

template <typename T>
void applier(ValueWidget* target, ArgVal<Tag::Named<"pageStep">, T> value) {
    target->pageStep = value.value;
}

template <typename T>
void applier(ValueWidget* target, ArgVal<Tag::Named<"snap">, T> value) {
    target->snap = value.value;
}

inline namespace Arg {

#ifndef BRISK__VALUE_ARG_DEFINED
#define BRISK__VALUE_ARG_DEFINED
constexpr inline Argument<Tag::Named<"value">> value{};
#endif
constexpr inline Argument<Tag::Named<"maximum">> maximum{};
constexpr inline Argument<Tag::Named<"minimum">> minimum{};
constexpr inline Argument<Tag::Named<"step">> step{};
constexpr inline Argument<Tag::Named<"pageStep">> pageStep{};
constexpr inline Argument<Tag::Named<"snap">> snap{};

constexpr inline Argument<Tag::PropArg<decltype(ValueWidget::wheelModifiers)>> wheelModifiers{};
constexpr inline Argument<Tag::PropArg<decltype(ValueWidget::hintFormatter)>> hintFormatter{};

} // namespace Arg
} // namespace Brisk
