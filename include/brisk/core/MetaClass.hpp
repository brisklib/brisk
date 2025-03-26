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

#include <type_traits>
#include <string_view>
#include <memory>

namespace Brisk {

// Example usage:
struct MetaClass {
    const MetaClass* classBase;
    std::string_view className;

    consteval MetaClass(const MetaClass* base, std::string_view className) noexcept
        : classBase(base), className(className) {}

    MetaClass(MetaClass&&) noexcept                 = delete;
    MetaClass(const MetaClass&) noexcept            = delete;
    MetaClass& operator=(MetaClass&&) noexcept      = delete;
    MetaClass& operator=(const MetaClass&) noexcept = delete;
};

struct MetaClassRoot : public MetaClass {
    consteval MetaClassRoot(std::string_view className) noexcept : MetaClass{ nullptr, className } {}
};

template <typename T, typename Base>
struct MetaClassImpl : public MetaClass {
    consteval MetaClassImpl(std::string_view className) noexcept
        : MetaClass{ Base::staticMetaClass(), className } {}
};

#define BRISK_DYNAMIC_CLASS_ROOT(ClassName)                                                                  \
public:                                                                                                      \
    static constinit inline MetaClassRoot metaClass{ #ClassName };                                           \
    static constexpr const MetaClass* staticMetaClass() noexcept {                                           \
        return &metaClass;                                                                                   \
    }                                                                                                        \
    virtual const MetaClass* dynamicMetaClass() const noexcept {                                             \
        return &metaClass;                                                                                   \
    }

#define BRISK_DYNAMIC_CLASS(ClassName, BaseClass)                                                            \
private:                                                                                                     \
    static constinit inline MetaClassImpl<ClassName, BaseClass> metaClass{ #ClassName };                     \
                                                                                                             \
public:                                                                                                      \
    static constexpr const MetaClass* staticMetaClass() noexcept {                                           \
        return &metaClass;                                                                                   \
    }                                                                                                        \
    const MetaClass* dynamicMetaClass() const noexcept override {                                            \
        return &metaClass;                                                                                   \
    }

namespace Internal {
bool isClassOrBase(const MetaClass* baseClass, const MetaClass* instanceClass) noexcept;
} // namespace Internal

template <typename To, typename From>
inline To dynamicCast(From* ptr) noexcept {
    static_assert(requires { static_cast<To>(ptr); });
    if (!ptr) {
        return nullptr;
    }

    static_assert(std::is_pointer_v<To>, "dynamicCast target must be a pointer type");
    static_assert(std::is_class_v<std::remove_pointer_t<To>>, "dynamicCast target must be a class type");

    const MetaClass* fromMeta = ptr->dynamicMetaClass();
    const MetaClass* toMeta   = std::remove_pointer_t<To>::staticMetaClass();

    if (Internal::isClassOrBase(toMeta, fromMeta)) {
        return static_cast<To>(ptr);
    } else {
        return nullptr;
    }
}

template <typename Class, typename From>
inline bool isOf(From* ptr) noexcept {
    if constexpr (!requires { static_cast<Class*>(static_cast<std::remove_cv_t<From>*>(nullptr)); }) {
        return false;
    }
    if (!ptr) {
        return false;
    }
    static_assert(std::is_class_v<Class>, "isOf type must be a class type");

    const MetaClass* fromMeta = ptr->dynamicMetaClass();
    const MetaClass* toMeta   = Class::staticMetaClass();

    return Internal::isClassOrBase(toMeta, fromMeta);
}

template <typename T, typename U>
std::shared_ptr<T> dynamicPointerCast(const std::shared_ptr<U>& r) noexcept {
    if (auto p = dynamicCast<typename std::shared_ptr<T>::element_type*>(r.get()))
        return std::shared_ptr<T>{ r, p };
    else
        return std::shared_ptr<T>{};
}

} // namespace Brisk
