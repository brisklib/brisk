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

#include <functional>
#include <memory>
#include "Throw.hpp"

namespace Brisk {

template <typename F>
struct function;

template <typename R, typename... Args>
struct fn_base {
    virtual ~fn_base() {}

    virtual R operator()(Args... args) = 0;
#ifdef BRISK_RTTI
    virtual const std::type_info& target_type() const noexcept = 0;
    virtual void* target() noexcept                            = 0;
#endif
};

template <typename Fn, typename R, typename... Args>
struct fn_impl : public fn_base<R, Args...> {
    template <typename Fn_>
    fn_impl(Fn_ fn) : fn(std::forward<Fn_>(fn)) {}

    ~fn_impl() override {}

    R operator()(Args... args) override {
        return fn(std::forward<Args>(args)...);
    }

#ifdef BRISK_RTTI
    const std::type_info& target_type() const noexcept override {
        return typeid(Fn);
    }

    void* target() noexcept override {
        return &fn;
    }
#endif

    Fn fn;
};

template <typename R, typename... Args>
struct function<R(Args...)> {
    function() noexcept = default;

    function(std::nullptr_t) noexcept {}

    template <typename Fn>
        requires(std::is_invocable_r_v<R, Fn, Args...> && !std::is_same_v<std::decay_t<Fn>, function>)
    function(Fn fn) : impl(new fn_impl<std::decay_t<Fn>, R, Args...>(std::move(fn))) {}

    function(const function&)                = default;

    function(function&&) noexcept            = default;

    function& operator=(const function&)     = default;

    function& operator=(function&&) noexcept = default;

    R operator()(Args... args) const {
        if (impl) {
            return impl->operator()(std::forward<Args>(args)...);
        }
        throwException(std::bad_function_call());
    }

    [[nodiscard]] explicit operator bool() const {
        return !!impl;
    }

    [[nodiscard]] bool empty() const {
        return !impl;
    }

    std::shared_ptr<fn_base<R, Args...>> impl;

#ifdef BRISK_RTTI
    const std::type_info& target_type() const noexcept {
        return impl->target_type();
    }

    template <typename T>
    T* target() const noexcept {
        if (impl->target_type() == typeid(T)) {
            return reinterpret_cast<T*>(impl->target());
        } else {
            return nullptr;
        }
    }
#endif

    bool operator==(const function& fn) const noexcept {
        return impl == fn.impl;
    }

    bool operator!=(const function& fn) const noexcept {
        return !operator==(fn);
    }
};
} // namespace Brisk
