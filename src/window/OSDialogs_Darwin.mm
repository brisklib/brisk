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
#define BRISK_ALLOW_OS_HEADERS 1
#include <brisk/window/WindowApplication.hpp>
#if !__has_feature(objc_arc)
#error ARC is reqired
#endif

#include <bit>
#include <brisk/window/OSDialogs.hpp>

#include <AppKit/AppKit.h>
#include <ApplicationServices/ApplicationServices.h>
#include <CoreFoundation/CFBundle.h>
#include <CoreFoundation/CoreFoundation.h>

#include <brisk/window/Window.hpp>
#include <brisk/core/Threading.hpp>
#include <brisk/graphics/OSWindowHandle.hpp>
#include <brisk/core/internal/NSTypes.hpp>
#include <brisk/core/Utilities.hpp>
#include <brisk/core/Localization.hpp>
#include <UniformTypeIdentifiers/UniformTypeIdentifiers.h>

namespace Brisk {

void Shell::openURLInBrowser(std::string_view url_str) {
    mustBeMainThread();
    CFURLRef url = CFURLCreateWithBytes(NULL,                   // allocator
                                        (UInt8*)url_str.data(), // URLBytes
                                        url_str.size(),         // length
                                        kCFStringEncodingUTF8,  // encoding
                                        NULL                    // baseURL
    );
    LSOpenCFURLRef(url, 0);
    CFRelease(url);
}

void Shell::openFolder(const fs::path& path) {
    return openURLInBrowser("file://" + path.string());
}

void Shell::openFileInDefaultApp(const fs::path& path) {
    return openURLInBrowser("file://" + path.string());
}

static std::string buttonName(DialogButtons btn) {
    switch (btn) {
    case DialogButtons::OK:
        return "OK"_tr;
    case DialogButtons::Cancel:
        return "Cancel"_tr;
    case DialogButtons::Yes:
        return "Yes"_tr;
    case DialogButtons::No:
        return "No"_tr;
    case DialogButtons::Close:
        return "Close"_tr;
    case DialogButtons::Retry:
        return "Retry"_tr;
    default:
        return {};
    }
}

static DialogButtons nextButton(DialogButtons& buttons) {
    size_t idx = std::countr_zero(static_cast<uint32_t>(buttons));
    if (idx >= 32)
        return {};
    buttons &= ~static_cast<DialogButtons>(1 << idx);
    return static_cast<DialogButtons>(1 << idx);
}

static DialogResult showChildDialog(OSWindow* window, std::string_view title, std::string_view message,
                                    DialogButtons buttons, MessageBoxType type) {
    mustBeMainThread();
    DialogResult result;
    CFOptionFlags alertResult;
    CFOptionFlags alertLevel = kCFUserNotificationPlainAlertLevel;
    switch (type) {
    case MessageBoxType::Error:
        alertLevel = kCFUserNotificationStopAlertLevel;
        break;
    case MessageBoxType::Warning:
        alertLevel = kCFUserNotificationCautionAlertLevel;
        break;
    case MessageBoxType::Security:
    case MessageBoxType::None:
    default:
        break;
    }

    NSString* nsTitle   = toNSString(title);
    NSString* nsMessage = toNSString(message);

    DialogButtons btn1  = nextButton(buttons);
    DialogButtons btn2  = nextButton(buttons);
    DialogButtons btn3  = nextButton(buttons);

    NSString* nsBtn1    = toNSStringOrNil(buttonName(btn1));
    NSString* nsBtn2    = toNSStringOrNil(buttonName(btn2));
    NSString* nsBtn3    = toNSStringOrNil(buttonName(btn3));

    int32_t ret         = CFUserNotificationDisplayAlert(
        0, alertLevel, NULL, NULL, NULL, (__bridge CFStringRef)nsTitle, (__bridge CFStringRef)nsMessage,
        (__bridge CFStringRef)nsBtn1, (__bridge CFStringRef)nsBtn2, (__bridge CFStringRef)nsBtn3,
        &alertResult);

    switch (alertResult) {
    case kCFUserNotificationDefaultResponse:
        result = static_cast<DialogResult>(btn1);
        break;
    case kCFUserNotificationAlternateResponse:
        result = static_cast<DialogResult>(btn2);
        break;
    case kCFUserNotificationOtherResponse:
        result = static_cast<DialogResult>(btn3);
        break;
    default:
        result = DialogResult::Other;
        break;
    }
    return result;
}

DialogResult Shell::showDialog(std::string_view title, std::string_view message, DialogButtons buttons,
                               MessageBoxType type) {
    DialogResult result;
    windowApplication->systemModal([&](OSWindow* window) {
        result = showChildDialog(window, title, message, buttons, type);
    });
    return result;
}

static void addFilterListToDialog(NSSavePanel* dialog, std::span<const Shell::FileDialogFilter> filters) {
    mustBeMainThread();
    if (filters.empty())
        return;

    NSMutableArray* filterList = [[NSMutableArray alloc] init];
    std::set<std::string_view> exts;
    for (const Shell::FileDialogFilter& ff : filters) {
        for (const std::string& f : ff.filters) {
            if (f == "*" || f == "*.*")
                return;
            if (f.starts_with("*."))
                exts.insert(f.substr(2));
            else if (f.starts_with("."))
                exts.insert(f.substr(1));
            else
                exts.insert(f);
        }
    }
    for (std::string_view s : exts) {
        UTType* type = [UTType typeWithFilenameExtension:toNSString(s) conformingToType:UTTypeData];
        if (type) {
            [filterList addObject:type];
        }
    }
    if (filterList.count == 0)
        return;

    [dialog setAllowedContentTypes:filterList];
}

static std::optional<fs::path> showChildOpenDialog(OSWindow* window,
                                                   std::span<const Shell::FileDialogFilter> filters,
                                                   const fs::path& defaultPath) {
    mustBeMainThread();
    @autoreleasepool {
        @try {
            NSWindow* keyWindow = [[NSApplication sharedApplication] keyWindow];
            SCOPE_EXIT {
                [keyWindow makeKeyAndOrderFront:nil];
            };

            NSOpenPanel* dialog = [NSOpenPanel openPanel];
            [dialog setAllowsMultipleSelection:NO];

            addFilterListToDialog(dialog, filters);

            [dialog setDirectoryURL:[NSURL fileURLWithPath:toNSString(defaultPath.native()) isDirectory:YES]];

            if ([dialog runModal] == NSModalResponseOK) {
                NSURL* url    = [dialog URL];
                fs::path path = fromNSString([url path]);
                return path;
            }
        } @catch (NSException* exception) {
            LOG_ERROR(window, "showOpenDialog: {} {}", fromNSString(exception.name),
                      fromNSString(exception.reason));
            return {};
        }
    }
    return std::nullopt;
}

std::optional<fs::path> Shell::showOpenDialog(std::span<const FileDialogFilter> filters,
                                              const fs::path& defaultPath) {
    std::optional<fs::path> result;
    windowApplication->systemModal([&](OSWindow* window) {
        result = showChildOpenDialog(window, filters, defaultPath);
    });
    return result;
}

static std::optional<fs::path> showChildSaveDialog(OSWindow* window,
                                                   std::span<const Shell::FileDialogFilter> filters,
                                                   const fs::path& defaultPath) {
    mustBeMainThread();
    @autoreleasepool {
        NSWindow* keyWindow = [[NSApplication sharedApplication] keyWindow];
        SCOPE_EXIT {
            [keyWindow makeKeyAndOrderFront:nil];
        };

        NSSavePanel* dialog;
        @try {
            dialog = [NSSavePanel savePanel];
        } @catch (NSException* exception) {
            LOG_ERROR(window, "showSaveDialog: {} {}", fromNSString(exception.name),
                      fromNSString(exception.reason));
            return {};
        }
        [dialog setExtensionHidden:NO];

        addFilterListToDialog(dialog, filters);

        [dialog setDirectoryURL:[NSURL fileURLWithPath:toNSString(defaultPath.native()) isDirectory:YES]];

        if ([dialog runModal] == NSModalResponseOK) {
            NSURL* url    = [dialog URL];
            fs::path path = fromNSString([url path]);
            return path;
        }
    }
    return std::nullopt;
}

std::optional<fs::path> Shell::showSaveDialog(std::span<const FileDialogFilter> filters,
                                              const fs::path& defaultPath) {
    std::optional<fs::path> result;
    windowApplication->systemModal([&](OSWindow* window) {
        result = showChildSaveDialog(window, filters, defaultPath);
    });
    return result;
}

static std::vector<fs::path> showChildOpenDialogMulti(OSWindow* window,
                                                      std::span<const Shell::FileDialogFilter> filters,
                                                      const fs::path& defaultPath) {
    mustBeMainThread();
    std::vector<fs::path> paths;
    @autoreleasepool {
        NSWindow* keyWindow = [[NSApplication sharedApplication] keyWindow];
        SCOPE_EXIT {
            [keyWindow makeKeyAndOrderFront:nil];
        };

        NSOpenPanel* dialog;
        @try {
            dialog = [NSOpenPanel openPanel];
        } @catch (NSException* exception) {
            LOG_ERROR(window, "showOpenDialogMulti: {} {}", fromNSString(exception.name),
                      fromNSString(exception.reason));
            return {};
        }
        [dialog setAllowsMultipleSelection:YES];

        addFilterListToDialog(dialog, filters);

        [dialog setDirectoryURL:[NSURL fileURLWithPath:toNSString(defaultPath.native()) isDirectory:YES]];

        if ([dialog runModal] == NSModalResponseOK) {
            NSArray* urls = [dialog URLs];
            for (NSURL* url in urls) {
                if (url) {
                    paths.push_back(fromNSString([url path]));
                }
            }
        }
    }
    return paths;
}

std::vector<fs::path> Shell::showOpenDialogMulti(std::span<const FileDialogFilter> filters,
                                                 const fs::path& defaultPath) {
    std::vector<fs::path> result;
    windowApplication->systemModal([&](OSWindow* window) {
        result = showChildOpenDialogMulti(window, filters, defaultPath);
    });
    return result;
}

static std::optional<fs::path> showChildFolderDialog(OSWindow* window, const fs::path& defaultPath) {
    mustBeMainThread();
    @autoreleasepool {
        @try {
            NSWindow* keyWindow = [[NSApplication sharedApplication] keyWindow];

            SCOPE_EXIT {
                [keyWindow makeKeyAndOrderFront:nil];
            };

            NSOpenPanel* dialog = [NSOpenPanel openPanel];
            [dialog setAllowsMultipleSelection:NO];
            [dialog setCanChooseDirectories:YES];
            [dialog setCanCreateDirectories:YES];
            [dialog setCanChooseFiles:NO];

            [dialog setDirectoryURL:[NSURL fileURLWithPath:toNSString(defaultPath.native()) isDirectory:YES]];

            if ([dialog runModal] == NSModalResponseOK) {
                NSURL* url    = [dialog URL];
                fs::path path = fromNSString([url path]);
                return path;
            }
        } @catch (NSException* exception) {
            LOG_ERROR(window, "showFolderDialog: {} {}", fromNSString(exception.name),
                      fromNSString(exception.reason));
            return std::nullopt;
        }
    }
    return std::nullopt;
}

std::optional<fs::path> Shell::showFolderDialog(const fs::path& defaultPath) {
    std::optional<fs::path> result;
    windowApplication->systemModal([&](OSWindow* window) {
        result = showChildFolderDialog(window, defaultPath);
    });
    return result;
}

} // namespace Brisk
