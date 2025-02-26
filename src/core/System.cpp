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
#include <brisk/core/Utilities.hpp>
#include <brisk/core/System.hpp>

#include <uv.h>

namespace Brisk {

OSUname osUname() {
    uv_utsname_t buf;
    if (uv_os_uname(&buf) < 0)
        return {};
    return { buf.sysname, buf.release, buf.version, buf.machine };
}

CpuInfo cpuInfo() {
    uv_cpu_info_t* buf = nullptr;
    int count          = 0;
    if (uv_cpu_info(&buf, &count) < 0)
        return {};
    SCOPE_EXIT {
        uv_free_cpu_info(buf, count);
    };
    return { buf[0].model, buf[0].speed };
}

MemoryInfo memoryInfo() {
    uv_rusage_t usage;
    if (uv_getrusage(&usage) < 0) {
        return {};
    }
    return {
        .maxrss  = usage.ru_maxrss,
        .majflt  = usage.ru_majflt,
        .inblock = usage.ru_inblock,
        .oublock = usage.ru_oublock,
    };
}

} // namespace Brisk
