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

#include "Model/Model.hpp"
#include <brisk/core/Rc.hpp>
#include <brisk/window/WindowApplication.hpp>

namespace App {

using namespace Brisk;

class DataSourceViewModel : public BindableObject<DataSourceViewModel, &uiScheduler> {
public:
    DataSourceViewModel(Rc<DataSourceModel> model, Value<int> updateTrigger);

    std::string caption() const;

    int count() const;

    std::string cap() const;

    Value<Normalized> value(int index) const;

    Value<Trigger<>> updated() const;

    Value<std::string> label(int index) const;

    Json json() const;

protected:
    Rc<DataSourceModel> m_model;
    BindableList<Normalized> m_values;
    BindableList<std::string> m_labels;
    Trigger<> m_updated;
    void update();
};

} // namespace App
