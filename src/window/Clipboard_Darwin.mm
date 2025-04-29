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
#include <brisk/window/Clipboard.hpp>
#include <brisk/core/Encoding.hpp>
#include <brisk/core/Utilities.hpp>

#include <brisk/core/internal/NSTypes.hpp>

#include <CoreFoundation/CoreFoundation.h>
#include <Foundation/Foundation.h>
#include <AppKit/AppKit.h>

namespace Brisk {

Clipboard::Format Clipboard::textFormat = "__text";

namespace {

bool copyText(NSPasteboard* pasteboard, const std::string& content) {

    NSString* string = [[NSString alloc] initWithBytesNoCopy:(void*)content.data()
                                                      length:content.size()
                                                    encoding:NSUTF8StringEncoding
                                                freeWhenDone:NO];
    return [pasteboard setString:string forType:NSPasteboardTypeString];
}

struct FmtData {
    NSString* fmt;
    NSData* data;
};

FmtData prepareData(const Clipboard::Format& format, const Bytes& bytes) {
    NSString* fmt = toNSStringNoCopy(format);
    NSData* data  = toNSDataNoCopy(bytes);
    return { fmt, data };
}

// [pasteboard setData:data forType:fmt];
} // namespace

bool Clipboard::setContent(const Content& content) {
    @autoreleasepool {
        NSPasteboard* pasteboard = [NSPasteboard generalPasteboard];
        [pasteboard clearContents];
        NSMutableArray* fmtArray = [NSMutableArray.alloc init];
        std::vector<FmtData> fmtData;
        if (content.text) {
            [fmtArray addObject:NSPasteboardTypeString];
        }
        for (auto& [fmt, data] : content.formats) {
            FmtData p = prepareData(fmt, data);
            fmtData.push_back(p);
            [fmtArray addObject:p.fmt];
        }
        [pasteboard addTypes:fmtArray owner:nil];
        if (content.text) {
            if (!copyText(pasteboard, *content.text))
                return false;
        }
        for (auto& d : fmtData) {
            if (![pasteboard setData:d.data forType:d.fmt])
                return false;
        }
    }
    return true;
}

static bool pasteboardHasText(NSPasteboard* pasteboard) {
    return [pasteboard availableTypeFromArray:[NSArray arrayWithObject:NSPasteboardTypeString]];
}

auto Clipboard::getContent(std::initializer_list<Format> formats) -> Content {
    Content result;
    @autoreleasepool {
        NSPasteboard* pasteboard = [NSPasteboard generalPasteboard];

        for (Format format : formats) {
            if (format == textFormat) {
                if (pasteboardHasText(pasteboard)) {
                    result.text = fromNSString([pasteboard stringForType:NSPasteboardTypeString]);
                }
            } else {
                NSString* fmt = toNSStringNoCopy(format);
                if ([pasteboard availableTypeFromArray:[NSArray arrayWithObject:fmt]]) {
                    NSData* nsData = [pasteboard dataForType:fmt];
                    if (nsData) {
                        Bytes data(nsData.length);
                        [nsData getBytes:data.data() length:data.size()];
                        result.formats[format] = std::move(data);
                    }
                }
            }
        }
    }
    return result;
}

bool Clipboard::hasFormat(Format format) {
    @autoreleasepool {
        NSPasteboard* pasteboard = [NSPasteboard generalPasteboard];
        if (format == textFormat) {
            return pasteboardHasText(pasteboard);
        } else {
            NSString* fmt = toNSStringNoCopy(format);
            return [pasteboard availableTypeFromArray:[NSArray arrayWithObject:fmt]];
        }
    }
    return false;
}

auto Clipboard::registerFormat(std::string_view formatId) -> Format {
    return std::string(formatId);
}

} // namespace Brisk
