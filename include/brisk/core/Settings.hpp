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

#include "Io.hpp"
#include "BasicTypes.hpp"
#include "internal/Generation.hpp"
#include "Binding.hpp"
#include <set>
#include <shared_mutex>

namespace Brisk {

class Settings {
public:
    static fs::path path();
    Json data(std::string_view path, const Json& val = nullptr) const;

    template <typename T>
    T dataAs(std::string_view path, const T& fallbackValue = nullptr) const {
        return data(path, nullptr).template to<T>().value_or(fallbackValue);
    }

    void setData(std::string_view path, Json json, bool notify = true, bool save = false);

    template <typename Fn>
    void update(std::string_view path, Fn&& fn, const Json& val = nullptr, bool notify = true,
                bool save = false) {
        using R = typename InternalHelper::LambdaArgument<Fn>::type;
        static_assert(std::is_reference_v<R>);
        using T = std::remove_reference_t<R>;
        T value;
        dataGetTo<T>(value, path, val);
        fn(value);
        setData(path, value, notify, save);
    }

    template <typename T>
    Value<T> value(std::string_view path, T fallbackValue) {
        return Value<T>{
            [path, this, fallbackValue = std::move(fallbackValue)]() -> T {
                return dataAs<T>(path, fallbackValue);
            },
            [path, this](T val) {
                setData(path, std::move(val));
            },
            { toBindingAddress(&m_data) },
            toBindingAddress(&m_data),
        };
    }

    void save();
    void load();

    void mock(Json json);

    bool isMocked() const noexcept;

    Settings();

    template <typename T>
    bool dataGetTo(T& value, std::string_view path, const Json& val = nullptr) const {
        return data(path, val).to(value);
    }

private:
    void internalSave();
    Json m_data = JsonObject();
    mutable std::shared_mutex m_mutex;
    bool m_mocked = false;
    Trigger<> m_changed;
    BindingRegistration m_lt{ this, mainScheduler };

public:

    static const auto& properties() noexcept {
        static constexpr tuplet::tuple props{
            /* 0 */ Internal::PropField{ &Settings::m_changed, "changed" },
        };
        return props;
    }

public:
    BRISK_PROPERTIES_BEGIN
    Property<Settings, Trigger<>, 0> changed;
    BRISK_PROPERTIES_END
};

extern Nullable<Settings> settings;
} // namespace Brisk
