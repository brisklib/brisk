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

/**
 * @brief Represents metadata for a class.
 *
 * This struct stores the base class pointer and class name, forming the foundation for type identification
 * and casting in a hierarchy. It is designed to be immutable and non-copyable.
 */
struct MetaClass {
    const MetaClass* classBase; ///< Pointer to the base class's MetaClass, or nullptr if no base.
    std::string_view className; ///< Name of the class as a string view.

    /**
     * @brief Constructs a MetaClass with a base class and name.
     * @param base Pointer to the base class's MetaClass (nullptr if root).
     * @param className Name of the class.
     * @note This constructor is evaluated at compile-time.
     */
    consteval MetaClass(const MetaClass* base, std::string_view className) noexcept
        : classBase(base), className(className) {}

    MetaClass(MetaClass&&) noexcept                 = delete; ///< Move constructor is deleted.
    MetaClass(const MetaClass&) noexcept            = delete; ///< Copy constructor is deleted.
    MetaClass& operator=(MetaClass&&) noexcept      = delete; ///< Move assignment is deleted.
    MetaClass& operator=(const MetaClass&) noexcept = delete; ///< Copy assignment is deleted.
};

/**
 * @brief Root class metadata for classes with no base class in the RTTI system.
 *
 * Inherits from MetaClass and sets the base class to nullptr.
 */
struct MetaClassRoot : public MetaClass {
    /**
     * @brief Constructs a root MetaClass with no base class.
     * @param className Name of the root class.
     */
    consteval MetaClassRoot(std::string_view className) noexcept : MetaClass{ nullptr, className } {}
};

/**
 * @brief Template struct for derived class metadata in the RTTI system.
 *
 * Inherits from MetaClass and links to the base class's metadata.
 *
 * @tparam T The derived class type.
 * @tparam Base The base class type.
 */
template <typename T, typename Base>
struct MetaClassImpl : public MetaClass {
    /**
     * @brief Constructs metadata for a derived class.
     * @param className Name of the derived class.
     */
    consteval MetaClassImpl(std::string_view className) noexcept
        : MetaClass{ Base::staticMetaClass(), className } {}
};

/**
 * @brief Macro to define RTTI metadata for a root class.
 *
 * Adds static and virtual methods for type identification.
 * @param ClassName The name of the class.
 */
#define BRISK_DYNAMIC_CLASS_ROOT(ClassName)                                                                  \
public:                                                                                                      \
    static constinit inline MetaClassRoot metaClass{ #ClassName };                                           \
    /**                                                                                                      \
     * @brief Returns a pointer to the static metadata of this class.                                        \
     * @return Const pointer to the MetaClass instance.                                                      \
     */                                                                                                      \
    static constexpr const MetaClass* staticMetaClass() noexcept {                                           \
        return &metaClass;                                                                                   \
    }                                                                                                        \
    /**                                                                                                      \
     * @brief Returns a pointer to the dynamic metadata of this instance.                                    \
     * @return Const pointer to the MetaClass instance.                                                      \
     */                                                                                                      \
    virtual const MetaClass* dynamicMetaClass() const noexcept {                                             \
        return &metaClass;                                                                                   \
    }

/**
 * @brief Macro to define RTTI metadata for a derived class.
 *
 * Adds static and virtual methods for type identification, linking to a base class.
 * @param ClassName The name of the class.
 * @param BaseClass The name of the base class.
 */
#define BRISK_DYNAMIC_CLASS(ClassName, BaseClass)                                                            \
private:                                                                                                     \
    static constinit inline MetaClassImpl<ClassName, BaseClass> metaClass{ #ClassName };                     \
                                                                                                             \
public:                                                                                                      \
    /**                                                                                                      \
     * @brief Returns a pointer to the static metadata of this class.                                        \
     * @return Const pointer to the MetaClass instance.                                                      \
     */                                                                                                      \
    static constexpr const MetaClass* staticMetaClass() noexcept {                                           \
        return &metaClass;                                                                                   \
    }                                                                                                        \
    /**                                                                                                      \
     * @brief Returns a pointer to the dynamic metadata of this instance.                                    \
     * @return Const pointer to the MetaClass instance.                                                      \
     */                                                                                                      \
    const MetaClass* dynamicMetaClass() const noexcept override {                                            \
        return &metaClass;                                                                                   \
    }

namespace Internal {
/**
 * @brief Checks if one class is the same as or a base of another class.
 * @param baseClass Pointer to the base class's MetaClass.
 * @param instanceClass Pointer to the instance's MetaClass.
 * @return True if baseClass is in the inheritance chain of instanceClass, false otherwise.
 */
bool isClassOrBase(const MetaClass* baseClass, const MetaClass* instanceClass) noexcept;
} // namespace Internal

/**
 * @brief Performs a dynamic cast between pointer types using custom RTTI.
 *
 * @tparam To The target pointer type.
 * @tparam From The source pointer type.
 * @param ptr Pointer to the source object.
 * @return Pointer of type To if the cast is valid, nullptr otherwise.
 */
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

/**
 * @brief Checks if an object is of a specific class type or derived from it.
 *
 * @tparam Class The class type to check against.
 * @tparam From The type of the pointer.
 * @param ptr Pointer to the object.
 * @return True if the object is of type Class or a derived type, false otherwise.
 */
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

/**
 * @brief Performs a dynamic cast on a shared_ptr using custom RTTI.
 *
 * @tparam T The target type of the shared_ptr.
 * @tparam U The source type of the shared_ptr.
 * @param r The source shared_ptr.
 * @return A shared_ptr of type T if the cast is valid, an empty shared_ptr otherwise.
 */
template <typename T, typename U>
std::shared_ptr<T> dynamicPointerCast(const std::shared_ptr<U>& r) noexcept {
    if (auto p = dynamicCast<typename std::shared_ptr<T>::element_type*>(r.get()))
        return std::shared_ptr<T>{ r, p };
    else
        return std::shared_ptr<T>{};
}

} // namespace Brisk
