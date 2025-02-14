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
#pragma once

#include <brisk/core/BasicTypes.hpp>

#ifdef BRISK_APPLE

#include <Foundation/Foundation.h>

namespace Brisk {

inline NSString* toNSString(std::string_view str) {
    return [NSString.alloc initWithBytes:(const UInt8*)str.data()
                                  length:str.size()
                                encoding:NSUTF8StringEncoding];
}

inline NSString* toNSStringOrNil(std::string_view str) {
    if (str.empty())
        return NULL;
    return toNSString(str);
}

inline NSString* toNSStringNoCopy(std::string_view str) {
    return [[NSString alloc] initWithBytesNoCopy:(void*)str.data()
                                          length:str.size()
                                        encoding:NSUTF8StringEncoding
                                    freeWhenDone:NO];
}

inline NSData* toNSDataNoCopy(BytesView bytes) {
    return [NSData dataWithBytesNoCopy:(void*)bytes.data() length:bytes.size() freeWhenDone:NO];
}

inline std::string fromNSString(NSString* string) {
    if (string == nil)
        return {};
    std::string result;
    size_t len = [string lengthOfBytesUsingEncoding:NSUTF8StringEncoding];
    result.resize(len);
    memcpy(result.data(), [string UTF8String], len);
    return result;
}

inline std::string fromCFString(CFStringRef string) {
    return fromNSString((__bridge NSString*)string);
}
} // namespace Brisk

#endif
