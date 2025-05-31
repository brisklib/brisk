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

#include <memory>
#include <string>

namespace App {

// Value normalized to [0..1] range
using Normalized = double;

// Data source (Model)
class DataSourceModel {
public:
    virtual ~DataSourceModel()                 = default;

    virtual std::string caption() const        = 0;

    virtual std::string cap() const            = 0;

    virtual std::string label(int index) const = 0;

    // Returns the number of measurements (cpu cores etc)
    virtual int count() const                  = 0;

    // Fetches new data
    virtual void update()                      = 0;

    // Gets data fetched by `update`
    virtual Normalized get(int index) const    = 0;
};

std::shared_ptr<DataSourceModel> dataCpuUsage();

std::shared_ptr<DataSourceModel> dataMemoryUsage();

} // namespace App
