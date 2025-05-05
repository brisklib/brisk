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
#include "ViewModel.hpp"

namespace App {

DataSourceViewModel::DataSourceViewModel(Rc<DataSourceModel> model, Value<int> updateTrigger)
    : m_model(std::move(model)) {
    m_values.resize(m_model->count());
    m_labels.resize(m_model->count());

    bindings->listen(
        updateTrigger, lifetime() | [this]() {
            update();
        });
}

int DataSourceViewModel::count() const {
    return m_model->count();
}

std::string DataSourceViewModel::caption() const {
    return m_model->caption();
}

std::string DataSourceViewModel::cap() const {
    return m_model->cap();
}

Brisk::Value<Normalized> DataSourceViewModel::value(int index) const {
    return Value{ &m_values[index] };
}

Brisk::Value<std::string> DataSourceViewModel::label(int index) const {
    return Value{ &m_labels[index] };
}

void DataSourceViewModel::update() {
    m_model->update();
    BRISK_ASSERT(m_values.size() == m_model->count());
    for (int i = 0; i < m_values.size(); ++i) {
        bindings->assign(m_values[i]) = m_model->get(i);
        bindings->assign(m_labels[i]) = m_model->label(i);
    }
    m_updated.trigger();
}

Json DataSourceViewModel::json() const {
    JsonObject result;
    result["data"]     = caption();
    result["readings"] = JsonArray(m_values.begin(), m_values.end());

    return result;
}

Brisk::Value<Trigger<>> DataSourceViewModel::updated() const {
    return Value{ &m_updated };
}
} // namespace App
