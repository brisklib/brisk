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
#include <brisk/core/Threading.hpp>
#include <brisk/core/Encoding.hpp>
#include <brisk/core/Utilities.hpp>

#define NOMINMAX 1
#define WIN32_LEAN_AND_MEAN 1
#include <windows.h>
#include <processthreadsapi.h>

namespace Brisk {

void setThreadName(std::string_view name) {
    HRESULT r;
    r = SetThreadDescription(GetCurrentThread(), utf8ToWcs(name).c_str());
    (void)r;
}

void setThreadPriority(ThreadPriority priority) {
    int prio = staticMap(priority,                                           //
                         ThreadPriority::Lowest, THREAD_PRIORITY_LOWEST,     //
                         ThreadPriority::Low, THREAD_PRIORITY_BELOW_NORMAL,  //
                         ThreadPriority::Normal, THREAD_PRIORITY_NORMAL,     //
                         ThreadPriority::High, THREAD_PRIORITY_ABOVE_NORMAL, //
                         ThreadPriority::Highest, THREAD_PRIORITY_HIGHEST,   //
                         THREAD_PRIORITY_NORMAL);
    SetThreadPriority(GetCurrentThread(), prio);
}
} // namespace Brisk
