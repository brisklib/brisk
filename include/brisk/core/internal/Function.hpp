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

#include <functional>
#include <memory>
#include "Throw.hpp"

namespace Brisk {

template <typename F>
struct function;

namespace Internal {

template <typename R, typename... Args>
struct fn_vtable {
    R (*invoke)(fn_vtable*, Args...);
};

template <typename Fn, typename R, typename... Args>
struct fn_vtable_impl : fn_vtable<R, Args...> {
    Fn fn;

    template <typename F>
    fn_vtable_impl(F&& fn) noexcept : fn(std::forward<F>(fn)) {
        this->invoke = &fn_vtable_impl::invoke_impl;
    }

    static R invoke_impl(fn_vtable<R, Args...>* self, Args... args) {
        return reinterpret_cast<fn_vtable_impl*>(self)->fn(std::forward<Args>(args)...);
    }
};

} // namespace Internal

template <typename R, typename... Args>
struct function<R(Args...)> {
    constexpr function() noexcept = default;

    constexpr function(std::nullptr_t) noexcept {}

    template <typename Fn>
        requires(std::is_invocable_r_v<R, Fn, Args...> && !std::is_same_v<std::decay_t<Fn>, function>)
    function(Fn fn) : impl(new Internal::fn_vtable_impl<std::decay_t<Fn>, R, Args...>(std::move(fn))) {}

    function(const function&) noexcept            = default;

    function(function&&) noexcept                 = default;

    function& operator=(const function&) noexcept = default;

    function& operator=(function&&) noexcept      = default;

    R operator()(Args... args) const {
        auto impl = this->impl.get();
        if (impl) {
            return impl->invoke(impl, std::forward<Args>(args)...);
        }
        throwException(std::bad_function_call());
    }

    [[nodiscard]] explicit operator bool() const noexcept {
        return !!impl;
    }

    [[nodiscard]] bool empty() const noexcept {
        return !impl;
    }

    std::shared_ptr<Internal::fn_vtable<R, Args...>> impl;

    bool operator==(const function& fn) const noexcept {
        return impl == fn.impl;
    }

    bool operator!=(const function& fn) const noexcept {
        return !operator==(fn);
    }
};
} // namespace Brisk
