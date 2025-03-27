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
 * If you do not wish to be bound by the GPL-2.0+ license, you must purchase a commercial
 * license. For commercial licensing options, please visit: https://brisklib.com
 *
 * You should have received a copy of the GNU General Public License along with this program.
 * If not, see <http://www.gnu.org/licenses/>.
 *
 * For commercial licensing, please visit: https://brisklib.com/
 */

#include <vector>
#include <string>
#include <map>
#include <brisk/core/Encoding.hpp>
#include <brisk/core/App.hpp>
#include <brisk/core/Log.hpp>
#include "Metadata.Defines.hpp"
#include <brisk/core/Text.hpp>
#include <brisk/core/internal/Initialization.hpp>

extern "C" char** environ;

int briskMain();

namespace Brisk {

std::vector<std::string> args;
std::map<std::string, std::string, std::less<>> environment;
std::vector<std::string> environmentPath;
} // namespace Brisk

using namespace Brisk;

static void parseCommandLine(int argc, char** argv) {
    args.resize(argc);
    for (int i = 0; i < argc; i++) {
        args[i] = argv[i] ? argv[i] : std::string();
    }
}

static void collectEnvironment() {
    char** env = environ;
    while (*env) {
        std::string_view pair = env[0];
        std::string_view key, value;
        split(pair, "=", key, value);
        environment.insert_or_assign(std::string(key), std::string(value));
        ++env;
    }
}

static void setup() {
    setMetadata(appMetadata);
    initializeCommon();
}

static void shutdown() {
    finalizeCommon();
}

int main(int argc, char** argv) {
    setup();
    parseCommandLine(argc, argv);
    collectEnvironment();
    int ret = 0;
#ifdef BRISK_EXCEPTIONS
    try {
#endif
        ret = briskMain();
#ifdef BRISK_EXCEPTIONS
    } catch (std::exception& exc) {
        LOG_DEBUG(application, "Exception occurred: {}", exc.what());
    } catch (...) {
        LOG_DEBUG(application, "Unknown exception occurred");
    }
#endif
    shutdown();
    return ret;
}
