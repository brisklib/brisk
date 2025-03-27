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
#define NOMINMAX 1
#include <windows.h>
#include <vector>
#include <string>
#include <brisk/core/Encoding.hpp>
#include <brisk/core/App.hpp>
#include <brisk/core/Log.hpp>
#include "Metadata.Defines.hpp"
#include <brisk/core/Text.hpp>
#include <brisk/core/Utilities.hpp>
#include <brisk/core/internal/Initialization.hpp>
#include <brisk/core/platform/SystemWindows.hpp>

LPWSTR winCmdLine = nullptr;

namespace Brisk {
std::vector<std::string> args;
std::map<std::string, std::string> environment;
std::vector<std::string> environmentPath;
} // namespace Brisk

using namespace Brisk;

int briskMain();

static void parseCommandLine() {
    int argc       = 0;
    wchar_t** argv = CommandLineToArgvW(winCmdLine, &argc);
    args.resize(argc);
    for (size_t i = 0; i < argc; ++i) {
        args[i] = Brisk::wcsToUtf8(std::wstring_view(argv[i]));
    }
    LocalFree(argv);
}

static std::wstring getEnvLine(wchar_t*& envPack) {
    wchar_t* start = envPack;
    while (*envPack != '\0') {
        ++envPack;
    }
    return std::wstring(start, envPack);
}

static void collectEnvironment() {
    wchar_t* envPack = GetEnvironmentStringsW();
    if (envPack == nullptr)
        return;
    SCOPE_EXIT {
        FreeEnvironmentStringsW(envPack);
    };
    wchar_t* envPtr = envPack;
    for (;;) {
        std::wstring wline = getEnvLine(envPtr);
        if (wline.empty())
            break;
        ++envPtr;
        std::string line = wcsToUtf8(wline);
        std::string name, value;
        split(line, "=", name, value);
        if (lowerCase(name) == "path") {
            environmentPath = toStrings(split(value, ";"));
        }
        environment.insert_or_assign(std::move(name), std::move(value));
    }
}

static void setup() {
    SetConsoleOutputCP(CP_UTF8);
    setMetadata(appMetadata);
    initializeCommon();
}

static void shutdown() {
    finalizeCommon();
}

static int mainFun() {
    setup();
    parseCommandLine();
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

int wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nShowCmd) {
    winInstance = hInstance;
    winCmdLine  = lpCmdLine;
    return mainFun();
}

int main(int, char**) {
    GetModuleHandleExA(0, nullptr, &winInstance);
    winCmdLine = GetCommandLineW();
    return mainFun();
}
