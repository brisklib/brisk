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
#include <brisk/network/UserAgent.hpp>
#include <brisk/core/Brisk.h>

#include <brisk/core/App.hpp>

namespace Brisk {

std::optional<std::string> httpUserAgent;

std::string defaultHttpUserAgent() {
    return std::string("Mozilla/5.0 (") + std::string(appMetadata.vendor) + " " +
           std::string(appMetadata.name) + " " + appMetadata.version.string() + "; " BRISK_OS_NAME ")";
}
} // namespace Brisk
