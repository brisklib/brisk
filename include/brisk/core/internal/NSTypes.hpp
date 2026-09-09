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
 * If you do not wish to be bound by the GPL-2.0+ license, you must purchase a commercial
 * license. For commercial licensing options, please visit: https://brisklib.com
 *
 * You should have received a copy of the GNU General Public License along with this program.
 * If not, see <http://www.gnu.org/licenses/>.
 *
 * For commercial licensing, please visit: https://brisklib.com/
 */
#pragma once

#include <brisk/core/BasicTypes.hpp>
#include <brisk/core/Brisk.h>


#ifdef BRISK_APPLE

#include <Foundation/Foundation.h>

namespace Brisk {

inline NSString* toNSString(std::string_view str) {
    return [NSString.alloc initWithBytes:str.data() length:str.size() encoding:NSUTF8StringEncoding];
}

inline NSString* toNSStringOrNil(std::string_view str) {
    if (str.empty())
        return nil;
    return toNSString(str);
}

inline NSString* toNSStringNoCopy(std::string_view str) {
    return [[NSString alloc] initWithBytesNoCopy:const_cast<char*>(str.data())
                                          length:str.size()
                                        encoding:NSUTF8StringEncoding
                                    freeWhenDone:NO];
}

inline NSData* toNSDataNoCopy(BytesView bytes) {
    return
        [NSData dataWithBytesNoCopy:const_cast<std::byte*>(bytes.data()) length:bytes.size() freeWhenDone:NO];
}

inline std::string fromNSString(NSString* string) {
    if (string == nil)
        return {};
    const char* utf8 = [string UTF8String];
    if (utf8 == nullptr)
        return {};
    const NSUInteger length = [string lengthOfBytesUsingEncoding:NSUTF8StringEncoding];
    return std::string(utf8, length);
}

inline std::string fromCFString(CFStringRef string) {
    if (string == nullptr)
        return {};
    return fromNSString((__bridge NSString*)string);
}
} // namespace Brisk

#endif
