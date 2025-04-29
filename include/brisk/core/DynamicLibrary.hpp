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

#include <string>
#include <type_traits>

#include <brisk/core/Rc.hpp>

namespace Brisk {

/**
 * @class DynamicLibrary
 * @brief Provides an interface to dynamically load and access functions from shared libraries.
 */
class DynamicLibrary {
public:
    /**
     * @brief Retrieves a pointer to a function from the dynamic library.
     * @tparam Func The function type to retrieve.
     * @param name The name of the function in the library.
     * @return A pointer to the function if found; otherwise, nullptr.
     */
    template <typename Func>
    Func* func(const std::string& name) const noexcept
        requires std::is_function_v<Func>
    {
        return reinterpret_cast<Func*>(getFunc(name));
    }

    /**
     * @brief Loads a dynamic library by its name.
     * @param name The name of the library to load.
     * @return A smart pointer to the loaded dynamic library.
     */
    static Rc<DynamicLibrary> load(const std::string& name) noexcept;

    /**
     * @brief Destructor for the DynamicLibrary class.
     * @details Cleans up and closes the dynamic library.
     */
    ~DynamicLibrary();

private:
    using FuncPtr = void (*)();

    DynamicLibrary(void* handle) noexcept;
    FuncPtr getFunc(const std::string& name) const noexcept;

    void* m_handle;
};

/**
 * @class DynamicFunc
 * @brief A wrapper for a function from a dynamically loaded library.
 * @tparam Func The type of the function being wrapped.
 */
template <typename Func>
struct DynamicFunc {
    /**
     * @brief Constructs a DynamicFunc object.
     * @param library A smart pointer to the dynamic library.
     * @param name The name of the function in the library.
     */
    DynamicFunc(const Rc<DynamicLibrary>& library, const std::string& name) {
        m_func = library->func<Func*>(name);
    }

    /**
     * @brief Constructs a DynamicFunc object and sets a flag if the function is not found.
     * @param library A smart pointer to the dynamic library.
     * @param name The name of the function in the library.
     * @param flag A boolean flag that is set to false if the function is not found.
     */
    DynamicFunc(const Rc<DynamicLibrary>& library, const std::string& name, bool& flag)
        : DynamicFunc(library, name) {
        m_func = library->func<Func*>(name);
        if (!m_func) {
            flag = false;
        }
    }

    /**
     * @brief Calls the wrapped function with the provided arguments.
     * @tparam Args Types of the arguments to be passed to the function.
     * @param args The arguments to pass to the function.
     * @return The result of the function call.
     * @note This method is `noexcept`.
     */
    template <typename... Args>
    decltype(auto) operator()(Args&&... args) const noexcept {
        return m_func(std::forward<Args>(args)...);
    }

private:
    Func* m_func;
};

} // namespace Brisk
