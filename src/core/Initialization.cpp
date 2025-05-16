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
#include <brisk/core/internal/Initialization.hpp>
#include <brisk/core/Log.hpp>
#include <brisk/core/Settings.hpp>
#include <brisk/core/Threading.hpp>

namespace Brisk {

static int coreInitLevel = 0;

void initializeCommon(InitializationFlags flags) {
    if (++coreInitLevel == 1) {
        initializeLogs();

        if (flags && InitializationFlags::Threading) {
            mainScheduler = rcnew TaskQueue();
            uiScheduler   = rcnew TaskQueue();
        } else {
            mainScheduler = nullptr;
            uiScheduler   = nullptr;
        }
        if (flags && InitializationFlags::Settings) {
            settings = new Settings{};
            settings->load();
        } else {
            settings = nullptr;
        }
    }
}

void finalizeCommon() {
    if (--coreInitLevel == 0) {
        if (settings) {
            settings->save();
            delete settings;
            settings = nullptr;
        }

        mainScheduler = nullptr;
        uiScheduler   = nullptr;
    }
}

} // namespace Brisk
