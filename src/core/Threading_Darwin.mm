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
#include <brisk/core/Threading.hpp>
#include <brisk/core/Encoding.hpp>
#include <brisk/core/Utilities.hpp>
#include <brisk/core/internal/NSTypes.hpp>

#include <pthread.h>
#import <Foundation/Foundation.h>

namespace Brisk {

void setThreadName(std::string_view name) {
    [[NSThread currentThread] setName:toNSString(name)];
}

void setThreadPriority(ThreadPriority priority) {
    double prio = staticMap(priority,                     //
                            ThreadPriority::Lowest, 0.1,  //
                            ThreadPriority::Low, 0.25,    //
                            ThreadPriority::Normal, 0.5,  //
                            ThreadPriority::High, 0.75,   //
                            ThreadPriority::Highest, 0.9, //
                            0.5);
    [NSThread setThreadPriority:prio];
}
} // namespace Brisk
