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

#include <memory>

namespace Brisk {

/** @brief Alias for a shared pointer type using std::shared_ptr.
 *  @tparam T The type of object managed by the shared pointer.
 */
template <typename T>
using RC = std::shared_ptr<T>;

/** @brief Alias for a weak pointer type using std::weak_ptr.
 *  @tparam T The type of object managed by the weak pointer.
 */
template <typename T>
using WeakRC = std::weak_ptr<T>;

namespace Internal {

/** @brief Utility for creating a shared pointer (RC) from a raw pointer.
 *  The operator `*` is overloaded to construct an `RC` object from a raw pointer.
 *  @details This is used with the `rcnew` macro to allocate objects
 *  and automatically wrap them in an `RC` (std::shared_ptr).
 */
struct RCNew {
    template <typename T>
    RC<T> operator*(T* rawPtr) const {
        return RC<T>(rawPtr);
    }
};

/** @brief Instance of RCNew used for creating reference-counted pointers.
 *  @see @ref RCNew
 */
constexpr inline RCNew rcNew{};

} // namespace Internal

/** @brief Macro to simplify the creation of reference-counted (shared) objects.
 *  @details This macro allows the use of `rcnew` in place of `new`
 *  to directly allocate objects wrapped in `RC` (std::shared_ptr).
 */
#define rcnew ::Brisk::Internal::rcNew* new

/** @brief Wraps a raw pointer in a shared pointer (`RC`) without taking ownership.
 *  @tparam T The type of the object being pointed to.
 *  @param pointer A raw pointer to the object. The pointer is not managed or deleted.
 *  @return A shared pointer (`RC`) that does not manage the lifetime of the object.
 */
template <typename T>
std::shared_ptr<T> notManaged(T* pointer) {
    return std::shared_ptr<T>(pointer, [](T*) {
        /* do nothing */
    });
}

} // namespace Brisk
