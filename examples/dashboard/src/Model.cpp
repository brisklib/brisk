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
#include "Model.hpp"
#include "brisk/core/Time.hpp"
#include <brisk/core/System.hpp>
#include <fmt/core.h>
#include <brisk/core/Log.hpp>

namespace App {
using namespace Brisk;

class DataSourceModelCpu final : public DataSourceModel {
public:
    DataSourceModelCpu() {
        m_previous = cpuUsage();
    }

    std::string caption() const {
        return "CPU load per core";
    }

    int count() const {
        return m_previous.usage.size();
    }

    void update() {
        auto newUsage = cpuUsage();
        m_delta       = newUsage - m_previous;
        m_previous    = std::move(newUsage);
    }

    std::string cap() const {
        return "100%";
    }

    std::string label(int index) const {
        return fmt::format("Core #{}", index + 1);
    }

    Normalized get(int index) const {
        double sum = m_delta.usage[index].sum();
        return (sum - m_delta.usage[index].idle) / sum;
    }

private:
    CpuUsage m_previous;
    CpuUsage m_delta;
};

std::shared_ptr<DataSourceModel> dataCpuUsage() {
    return std::make_shared<DataSourceModelCpu>();
}

} // namespace App
