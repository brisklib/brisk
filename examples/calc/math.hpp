#pragma once

#include <array>
#include <optional>
#include <brisk/core/Binding.hpp>
#include <brisk/core/internal/Debug.hpp>
#include <variant>
#include <string_view>
#include <vector>
#include <fmt/format.h>
#include <memory>
#include <brisk/core/Log.hpp>
#include <brisk/window/WindowApplication.hpp>
#include <brisk/core/Reflection.hpp>

extern "C" {
#include <decimal128.h>
}

namespace Brisk {

struct NumberCtx {
    decContext ctx;

    NumberCtx() {
        decContextDefault(&ctx, DEC_INIT_DECIMAL64);
    }
};

enum class AdditiveOperator { Add, Subtract };
enum class MultiplicativeOperator { Multiply, Divide };
enum class ExponentiationOperator { Power };
enum class UnaryOperator { Negate, SquareRoot, Reciprocal, Square };

template <>
inline constexpr std::initializer_list<NameValuePair<AdditiveOperator>> defaultNames<AdditiveOperator>{
    { "+", AdditiveOperator::Add },
    { "-", AdditiveOperator::Subtract },
};
template <>
inline constexpr std::initializer_list<NameValuePair<MultiplicativeOperator>>
    defaultNames<MultiplicativeOperator>{
        { "*", MultiplicativeOperator::Multiply },
        { "-", MultiplicativeOperator::Divide },
    };
template <>
inline constexpr std::initializer_list<NameValuePair<ExponentiationOperator>>
    defaultNames<ExponentiationOperator>{
        { "^", ExponentiationOperator::Power },
    };
template <>
inline constexpr std::initializer_list<NameValuePair<UnaryOperator>> defaultNames<UnaryOperator>{
    { "±", UnaryOperator::Negate },
    { "✓", UnaryOperator::SquareRoot },
};

// Minus,      // −
// Square,     // ²
// SquareRoot, // ✓
// Log,        // log
// Ln,         // ln
// Recip,      // 1/x
// Exp,        // exp

struct Number {
    decNumber num;

    static inline NumberCtx ctx;

    Number() {
        decNumberFromInt32(&num, 0);
    }

    Number(int32_t val) {
        decNumberFromInt32(&num, val);
    }

    Number(uint32_t val) {
        decNumberFromUInt32(&num, val);
    }

    void trim() & {
        decNumberTrim(&num);
    }

    static Number parse(const std::string& s) {
        Number result;
        decNumberFromString(&result.num, s.c_str(), &ctx.ctx);
        result.trim();
        return result;
    }

    bool isZero() const {
        return decNumberIsZero(&num);
    }

    bool isNan() const {
        return decNumberIsQNaN(&num);
    }

    Number operator+(Number rh) const {
        Number result;
        decNumberAdd(&result.num, &num, &rh.num, &ctx.ctx);
        result.trim();
        return result;
    }

    Number operator-(Number rh) const {
        Number result;
        decNumberSubtract(&result.num, &num, &rh.num, &ctx.ctx);
        result.trim();
        return result;
    }

    Number operator*(Number rh) const {
        Number result;
        decNumberMultiply(&result.num, &num, &rh.num, &ctx.ctx);
        result.trim();
        return result;
    }

    Number operator/(Number rh) const {
        Number result;
        decNumberDivide(&result.num, &num, &rh.num, &ctx.ctx);
        result.trim();
        return result;
    }

    Number operator<<(Number rh) const {
        Number result;
        decNumberShift(&result.num, &num, &rh.num, &ctx.ctx);
        result.trim();
        return result;
    }

    Number operator>>(Number rh) const {
        Number result;
        rh = -rh;
        decNumberShift(&result.num, &num, &rh.num, &ctx.ctx);
        result.trim();
        return result;
    }

    Number operator^(Number rh) const {
        Number result;
        decNumberPower(&result.num, &num, &rh.num, &ctx.ctx);
        result.trim();
        return result;
    }

    Number ln() const {
        Number result;
        decNumberLn(&result.num, &num, &ctx.ctx);
        result.trim();
        return result;
    }

    Number log10() const {
        Number result;
        decNumberLog10(&result.num, &num, &ctx.ctx);
        result.trim();
        return result;
    }

    Number exp() const {
        Number result;
        decNumberExp(&result.num, &num, &ctx.ctx);
        result.trim();
        return result;
    }

    Number sqrt() const {
        Number result;
        decNumberSquareRoot(&result.num, &num, &ctx.ctx);
        result.trim();
        return result;
    }

    Number square() const {
        Number result;
        decNumberMultiply(&result.num, &num, &num, &ctx.ctx);
        result.trim();
        return result;
    }

    Number recip() const {
        Number result;
        decNumberDivide(&result.num, Number(1).ptr(), &num, &ctx.ctx);
        result.trim();
        return result;
    }

    Number operator-() const {
        Number result;
        decNumberCopyNegate(&result.num, &num);
        result.trim();
        return result;
    }

    std::string string() const {
        char buf[DECIMAL128_String];
        decNumberToString(&num, buf);
        return buf;
    }

private:
    const decNumber* ptr() const {
        return &num;
    }
};
} // namespace Brisk

template <typename Char>
struct fmt::formatter<Brisk::Number, Char> : fmt::formatter<std::basic_string<Char>, Char> {
    template <typename FormatContext>
    auto format(const Brisk::Number& val, FormatContext& ctx) const {
        return formatter<std::basic_string<Char>, Char>::format(val.string(), ctx);
    }
};

namespace Brisk {

inline Number binary(Number x, AdditiveOperator op, Number y) {
    switch (op) {
    case AdditiveOperator::Add:
        return x + y;
    case AdditiveOperator::Subtract:
        return x - y;
    default:
        BRISK_UNREACHABLE();
    }
}

inline Number binary(Number x, MultiplicativeOperator op, Number y) {
    switch (op) {
    case MultiplicativeOperator::Multiply:
        return x * y;
    case MultiplicativeOperator::Divide:
        return x / y;
    default:
        BRISK_UNREACHABLE();
    }
}

inline Number binary(Number x, ExponentiationOperator op, Number y) {
    switch (op) {
    case ExponentiationOperator::Power:
        return x ^ y;
    default:
        BRISK_UNREACHABLE();
    }
}

inline Number unary(UnaryOperator op, Number x) {
    switch (op) {
    case UnaryOperator::Negate:
        return -x;
    case UnaryOperator::SquareRoot:
        return x.sqrt();
    case UnaryOperator::Reciprocal:
        return Number(1) / x;
    case UnaryOperator::Square:
        return x * x;
    default:
        BRISK_UNREACHABLE();
    }
}

struct Calculator {
    std::optional<std::string> editable;
    std::optional<Number> memory;

    std::optional<std::tuple<Number, AdditiveOperator>> additiveOperation;
    std::optional<std::tuple<Number, MultiplicativeOperator>> multiplicativeOperation;
    std::optional<std::tuple<Number, ExponentiationOperator>> exponentiationOperation;
    Number currentOperand;

    BindingRegistration registration{ this, uiThread };

    std::string output() const {
        if (editable)
            return *editable;
        return currentOperand.string();
    }

    void notify() {
        bindings->notify(this);
    }

    Value<std::string> valOutput() const {
        return Value<std::string>(
            [this]() {
                return output();
            },
            nullptr, toBindingAddress(this));
    }

    void memStore() {
        memory = currentOperand;
        notify();
    }

    void memAdd() {
        if (!memory)
            memory = 0;
        *memory = *memory + currentOperand;
        notify();
    }

    void memSubtract() {
        if (!memory)
            memory = 0;
        *memory = *memory - currentOperand;
        notify();
    }

    void memRecall() {
        if (memory)
            currentOperand = *memory;
        notify();
    }

    void memClear() {
        memory = nullopt;
        notify();
    }

    void changeSign() {
        if (editable) {
            if (editable->front() == '-')
                edit(editable->substr(1));
            else
                edit('-' + *editable);
        } else {
            operation(UnaryOperator::Negate);
        }
    }

    Number exponentiationSolve() const {
        Number result = currentOperand;

        if (exponentiationOperation) {
            auto [left, op] = *exponentiationOperation;
            result          = binary(left, op, result);
        }
        return result;
    }

    Number multiplicativeSolve() const {
        Number result = exponentiationSolve();

        if (multiplicativeOperation) {
            auto [left, op] = *multiplicativeOperation;
            result          = binary(left, op, result);
        }
        return result;
    }

    Number additiveSolve() const {
        Number result = multiplicativeSolve();

        if (additiveOperation) {
            auto [left, op] = *additiveOperation;
            result          = binary(left, op, result);
        }
        return result;
    }

    Number calculate() const {
        return additiveSolve();
    }

    std::string stringify() const {
        std::string result = fmt::to_string(currentOperand);
        if (exponentiationOperation) {
            auto [left, op] = *exponentiationOperation;
            result          = fmt::format("{} {} {}", left, op, result);
        }
        if (multiplicativeOperation) {
            auto [left, op] = *multiplicativeOperation;
            result          = fmt::format("{} {} {}", left, op, result);
        }
        if (additiveOperation) {
            auto [left, op] = *additiveOperation;
            result          = fmt::format("{} {} {}", left, op, result);
        }
        return result;
    }

    void operation(AdditiveOperator op) {
        currentOperand          = multiplicativeSolve();
        additiveOperation       = std::make_tuple(currentOperand, op);
        multiplicativeOperation = std::nullopt;
        exponentiationOperation = std::nullopt;
        editable                = nullopt;
        notify();
    }

    void operation(MultiplicativeOperator op) {
        currentOperand          = exponentiationSolve();
        multiplicativeOperation = std::make_tuple(currentOperand, op);
        exponentiationOperation = std::nullopt;
        editable                = nullopt;
        notify();
    }

    void operation(ExponentiationOperator op) {
        exponentiationOperation = std::make_tuple(currentOperand, op);
        editable                = nullopt;
        notify();
    }

    void operation(UnaryOperator op) {
        currentOperand = unary(op, currentOperand);
        editable       = nullopt;
        notify();
    }

    void solve() {
        currentOperand          = calculate();
        editable                = std::nullopt;
        additiveOperation       = std::nullopt;
        multiplicativeOperation = std::nullopt;
        exponentiationOperation = std::nullopt;
        notify();
    }

    void clear() {
        currentOperand          = 0;
        editable                = std::nullopt;
        additiveOperation       = std::nullopt;
        multiplicativeOperation = std::nullopt;
        exponentiationOperation = std::nullopt;
        notify();
    }

    void constant(Number x) {
        currentOperand = x;
        editable       = std::nullopt;
        notify();
    }

    void edit(std::string s) {
        Number tmp = Number::parse(s);
        if (!tmp.isNan()) {
            editable       = std::move(s);
            currentOperand = tmp;
        }
        notify();
    }

    void backspace() {
        if (editable && editable->size() > 1)
            edit(editable->substr(0, editable->size() - 1));
        else
            clear();
    }

    void digit(uint8_t digit) {
        if (!editable)
            editable = "";
        if (editable->empty() || editable->front() != '0')
            edit(*editable + char('0' + digit));
    }

    void decimalSep() {
        if (!editable)
            editable = "";
        if (editable->empty())
            edit("0.");
        else if (editable->find('.') == std::string::npos)
            edit(*editable + char('.'));
    }
};

} // namespace Brisk
