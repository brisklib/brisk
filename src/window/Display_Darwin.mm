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
#include <AppKit/AppKit.h>
#include <brisk/window/Display.hpp>

#include <shared_mutex>
#include <brisk/graphics/internal/NSTypes.hpp>

#include <IOKit/graphics/IOGraphicsLib.h>
#include <ApplicationServices/ApplicationServices.h>

namespace Brisk {

class DisplayMacos final : public Display {
public:
    Point position() const {
        std::shared_lock lk(m_mutex);
        return m_position;
    }

    Rectangle workarea() const {
        std::shared_lock lk(m_mutex);
        return m_workarea;
    }

    Size resolution() const {
        std::shared_lock lk(m_mutex);
        return m_resolution * m_scale;
    }

    Size nativeResolution() const {
        std::shared_lock lk(m_mutex);
        return m_nativeResolution;
    }

    Size size() const {
        std::shared_lock lk(m_mutex);
        return m_resolution;
    }

    SizeF physicalSize() const {
        // No lock needed
        return m_physSize;
    }

    int dpi() const {
        std::shared_lock lk(m_mutex);
        return 72 * m_scale;
    }

    const std::string& name() const {
        // No lock needed
        return m_name;
    }

    const std::string& id() const {
        // No lock needed
        return m_id;
    }

    const std::string& adapterName() const {
        // No lock needed
        return m_adapterName;
    }

    const std::string& adapterId() const {
        // No lock needed
        return m_adapterId;
    }

    DisplayFlags flags() const {
        std::shared_lock lk(m_mutex);
        return m_flags;
    }

    float contentScale() const {
        std::shared_lock lk(m_mutex);
        return m_scale;
    }

    double refreshRate() const {
        std::shared_lock lk(m_mutex);
        return m_refreshRate;
    }

    Point desktopToMonitor(Point pt) const {
        return pt;
    }

    Point monitorToDesktop(Point pt) const {
        return pt;
    }

    int backingScaleFactor() const {
        std::shared_lock lk(m_mutex);
        return m_backingScaleFactor;
    }

    bool containsWindow(OSWindowHandle handle) const {
        NSScreen* windowScreen = [handle.nsWindow() screen];
        if (!windowScreen) {
            return false;
        }

        // Get the display ID from the screen description
        NSDictionary* screenDescription = [windowScreen deviceDescription];
        NSNumber* screenNumber          = [screenDescription objectForKey:@"NSScreenNumber"];
        if (!screenNumber) {
            return false;
        }

        return (CGDirectDisplayID)[screenNumber unsignedIntValue] == m_dispId;
    }

    OSDisplayHandle getHandle() const {
        return OSDisplayHandle(m_dispId);
    }

private:
    mutable std::shared_mutex m_mutex;
    std::string m_name;
    std::string m_id;
    std::string m_adapterName;
    std::string m_adapterId;
    const uint32_t m_unitNumber{};
    SizeF m_physSize;
    CGDirectDisplayID m_dispId{};
    int m_backingScaleFactor = 1;
    DisplayFlags m_flags{};
    Rectangle m_workarea;
    Point m_position;
    Size m_resolution;
    Size m_nativeResolution;
    double m_refreshRate{};
    float m_scale;
    uint32_t m_counter = 0;
    friend void Internal::updateDisplays();

    void update(CGDirectDisplayID id, NSScreen* screen, DisplayFlags flags, std::string name);

    DisplayMacos(uint32_t unitNumber, SizeF physSize) : m_unitNumber(unitNumber), m_physSize(physSize) {
        m_id = fmt::to_string(m_unitNumber);
    }
};

static std::shared_mutex displayMutex;
static std::map<uint32_t, RC<DisplayMacos>> allDisplays;
static RC<DisplayMacos> primaryDisplay;
static uint32_t counter = 0;

std::vector<RC<Display>> Display::all() {
    std::shared_lock lk(displayMutex);
    std::vector<RC<Display>> result;
    for (auto [k, d] : allDisplays) {
        result.push_back(std::move(d));
    }
    return result;
}

RC<Display> Display::primary() {
    std::shared_lock lk(displayMutex);
    return primaryDisplay;
}

// Get the name of the specified display, or NULL
//
static std::string getMonitorName(CGDirectDisplayID displayID, NSScreen* screen) noexcept {
    // IOKit doesn't work on Apple Silicon anymore
    // Luckily, 10.15 introduced -[NSScreen localizedName].
    // Use it if available, and fall back to IOKit otherwise.
    if (screen) {
        if ([screen respondsToSelector:@selector(localizedName)]) {
            NSString* name = [screen valueForKey:@"localizedName"];
            if (name)
                return fromNSString(name);
        }
    }

    io_iterator_t it;
    io_service_t service;
    CFDictionaryRef info;

    if (IOServiceGetMatchingServices(MACH_PORT_NULL, IOServiceMatching("IODisplayConnect"), &it) != 0) {
        // This may happen if a desktop Mac is running headless
        return "Display";
    }

    while ((service = IOIteratorNext(it)) != 0) {
        info                     = IODisplayCreateInfoDictionary(service, kIODisplayOnlyPreferredName);

        CFNumberRef vendorIDRef  = (CFNumberRef)CFDictionaryGetValue(info, CFSTR(kDisplayVendorID));
        CFNumberRef productIDRef = (CFNumberRef)CFDictionaryGetValue(info, CFSTR(kDisplayProductID));
        if (!vendorIDRef || !productIDRef) {
            CFRelease(info);
            continue;
        }

        unsigned int vendorID, productID;
        CFNumberGetValue(vendorIDRef, kCFNumberIntType, &vendorID);
        CFNumberGetValue(productIDRef, kCFNumberIntType, &productID);

        if (CGDisplayVendorNumber(displayID) == vendorID && CGDisplayModelNumber(displayID) == productID) {
            // Info dictionary is used and freed below
            break;
        }

        CFRelease(info);
    }

    IOObjectRelease(it);

    if (!service)
        return "Display";

    CFDictionaryRef names = (CFDictionaryRef)CFDictionaryGetValue(info, CFSTR(kDisplayProductName));

    CFStringRef nameRef;

    if (!names || !CFDictionaryGetValueIfPresent(names, CFSTR("en_US"), (const void**)&nameRef)) {
        // This may happen if a desktop Mac is running headless
        CFRelease(info);
        return "Display";
    }

    std::string result = fromCFString(nameRef);
    CFRelease(info);
    return result;
}

// Returns the display refresh rate queried from the I/O registry
//
static double getFallbackRefreshRate(CGDirectDisplayID displayID) {
    double refreshRate = 60.0;

    io_iterator_t it;
    io_service_t service;

    if (IOServiceGetMatchingServices(MACH_PORT_NULL, IOServiceMatching("IOFramebuffer"), &it) != 0) {
        return refreshRate;
    }

    while ((service = IOIteratorNext(it)) != 0) {
        const CFNumberRef indexRef = (CFNumberRef)IORegistryEntryCreateCFProperty(
            service, CFSTR("IOFramebufferOpenGLIndex"), kCFAllocatorDefault, kNilOptions);
        if (!indexRef)
            continue;

        uint32_t index = 0;
        CFNumberGetValue(indexRef, kCFNumberIntType, &index);
        CFRelease(indexRef);

        if (CGOpenGLDisplayMaskToDisplayID(1 << index) != displayID)
            continue;

        const CFNumberRef clockRef = (CFNumberRef)IORegistryEntryCreateCFProperty(
            service, CFSTR("IOFBCurrentPixelClock"), kCFAllocatorDefault, kNilOptions);
        const CFNumberRef countRef = (CFNumberRef)IORegistryEntryCreateCFProperty(
            service, CFSTR("IOFBCurrentPixelCount"), kCFAllocatorDefault, kNilOptions);

        uint32_t clock = 0, count = 0;

        if (clockRef) {
            CFNumberGetValue(clockRef, kCFNumberIntType, &clock);
            CFRelease(clockRef);
        }

        if (countRef) {
            CFNumberGetValue(countRef, kCFNumberIntType, &count);
            CFRelease(countRef);
        }

        if (clock > 0 && count > 0)
            refreshRate = clock / (double)count;

        break;
    }

    IOObjectRelease(it);
    return refreshRate;
}

void DisplayMacos::update(CGDirectDisplayID id, NSScreen* screen, DisplayFlags flags, std::string name) {
    m_dispId              = id;
    m_flags               = flags;
    m_name                = std::move(name);

    m_backingScaleFactor  = screen.backingScaleFactor;

    m_workarea            = fromNSRect(screen.visibleFrame);

    const CGRect bounds   = CGDisplayBounds(m_dispId);
    m_position            = fromNSPoint((NSPoint)bounds.origin);

    CGDisplayModeRef mode = CGDisplayCopyDisplayMode(m_dispId);
    m_resolution          = { int(CGDisplayModeGetWidth(mode)), int(CGDisplayModeGetHeight(mode)) };
    m_refreshRate         = CGDisplayModeGetRefreshRate(mode);
    if (m_refreshRate == 0.0)
        m_refreshRate = getFallbackRefreshRate(m_dispId);

    const NSRect points = [screen frame];
    const NSRect pixels = [screen convertRectToBacking:points];

    m_scale             = std::max((float)(pixels.size.width / points.size.width),
                                   (float)(pixels.size.height / points.size.height));

    CFArrayRef modes    = CGDisplayCopyAllDisplayModes(m_dispId, NULL);
    CFIndex n           = CFArrayGetCount(modes);
    Size maxRes{ 0, 0 };
    for (int i = 0; i < n; ++i) {
        CGDisplayModeRef m = (CGDisplayModeRef)CFArrayGetValueAtIndex(modes, i);
        Size res(CGDisplayModeGetPixelWidth(m), CGDisplayModeGetPixelHeight(m));
        maxRes = max(maxRes, res);
    }
    CFRelease(modes);
    m_nativeResolution = maxRes;
}

void Internal::updateDisplays() {
    mustBeMainThread();
    std::unique_lock lk(displayMutex);
    ++counter;
    uint32_t displayCount;
    CGGetOnlineDisplayList(0, nullptr, &displayCount);
    std::vector<CGDirectDisplayID> displays(displayCount);
    CGGetOnlineDisplayList(displayCount, displays.data(), &displayCount);

    primaryDisplay = nullptr;

    for (uint32_t i = 0; i < displayCount; i++) {
        if (CGDisplayIsAsleep(displays[i]))
            continue;

        const uint32_t unitNumber = CGDisplayUnitNumber(displays[i]);
        NSScreen* screen          = nil;

        for (screen in [NSScreen screens]) {
            NSNumber* screenNumber = [screen deviceDescription][@"NSScreenNumber"];

            // HACK: Compare unit numbers instead of display IDs to work around
            //       display replacement on machines with automatic graphics
            //       switching
            if (CGDisplayUnitNumber([screenNumber unsignedIntValue]) == unitNumber)
                break;
        }
        if (!screen)
            continue;

        const CGSize physSize = CGDisplayScreenSize(displays[i]);
        std::string name      = getMonitorName(displays[i], screen);
        if (name.empty())
            continue;

        bool isPrimary        = primaryDisplay == nullptr;
        RC<DisplayMacos>& mon = allDisplays[unitNumber];
        if (mon == nullptr)
            mon = rcnew DisplayMacos(unitNumber, fromNSSize((NSSize)physSize));
        std::unique_lock lk(mon->m_mutex);
        mon->update(displays[i], screen, isPrimary ? DisplayFlags::Primary : DisplayFlags::None,
                    std::move(name));
        mon->m_counter = counter;
        if (isPrimary) {
            primaryDisplay = mon;
        }
    }
    for (auto it = allDisplays.begin(); it != allDisplays.end();) {
        if (it->second->m_counter != counter) {
            it = allDisplays.erase(it);
        } else {
            ++it;
        }
    }
}
} // namespace Brisk
