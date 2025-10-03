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
#include <brisk/core/Settings.hpp>
#include <brisk/core/Log.hpp>
#include <brisk/core/Reflection.hpp>

#include <brisk/core/App.hpp>

namespace Brisk {

Nullable<Settings> settings;

Settings::Settings() {}

fs::path Settings::path() {
    const fs::path configfolder = defaultFolder(DefaultFolder::AppUserData) / "config";
    fs::create_directories(configfolder);
    return configfolder / "config.json";
}

Json Settings::data(std::string_view path, const Json& val) const {
    std::shared_lock lk(m_mutex);
    return m_data.itemByPath(path).value_or(val);
}

void Settings::setData(std::string_view path, Json json, bool notify, bool save) {
    {
        std::unique_lock lk(m_mutex);
        m_data.setItemByPath(path, std::move(json));
        if (save) {
            internalSave();
        }
    }
    if (notify) {
        bindings->notify(&m_changed);
    }
}

void Settings::internalSave() {
    if (m_mocked)
        return;
    const fs::path p = path();
    if (auto e = writeJson(p, m_data, 4); !e.has_value()) {
        BRISK_LOG_ERROR("Can't write settings, code = {}", e.error());
    }
}

void Settings::save() {
    std::shared_lock lk(m_mutex);
    internalSave();
}

void Settings::load() {
    std::unique_lock lk(m_mutex);
    if (auto e = readJson(path())) {
        m_data = *e;
    } else {
        BRISK_LOG_ERROR("Can't read settings, e = {}", e.error());
    }
    if (m_data.type() != JsonType::Object)
        m_data = JsonObject();
    bindings->notify(&m_changed);
}

bool Settings::isMocked() const noexcept {
    return m_mocked;
}

void Settings::mock(Json json) {
    m_data   = std::move(json);
    m_mocked = true;
}
} // namespace Brisk
