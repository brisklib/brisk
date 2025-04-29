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
#pragma once

#include <brisk/graphics/OsWindowHandle.hpp>
#include <brisk/core/Binding.hpp>
#include <brisk/graphics/Geometry.hpp>
#include <brisk/core/Rc.hpp>
#include <brisk/graphics/OsDisplayHandle.hpp>

namespace Brisk {

enum class DisplayFlags {
    None    = 0,
    Primary = 1,
};

template <>
constexpr inline bool isBitFlags<DisplayFlags> = true;

/**
 * @class Display
 * @brief A class that provides information about a monitor (display) device, including content scale and
 * coordinate conversion.
 *
 * The Display class is an abstract base class that represents a monitor. It provides methods to retrieve
 * various attributes of the display such as position, size, resolution, DPI, and name. It also offers methods
 * for coordinate conversion between desktop and monitor coordinates.
 *
 * @remark All methods are thread-safe.
 */
class Display : public Object {
public:
    /**
     * @brief Get the position of the display in desktop coordinates.
     * @return The top-left position of the display.
     */
    virtual Point position() const                           = 0;

    /**
     * @brief Get the size of the display in desktop coordinates.
     * @return The size of the display in pixels.
     */
    virtual Size size() const                                = 0;

    /**
     * @brief Get the work area of the display.
     * @return The work area of the display excluding taskbars, docks, and other furniture.
     */
    virtual Rectangle workarea() const                       = 0;

    /**
     * @brief Get the current resolution of the display.
     * @return The current resolution of the display in pixels.
     * @remark On macOS this returns virtual resolution before scaling down to the display's native
     * resolution. Same as resolution of screenshots.
     */
    virtual Size resolution() const                          = 0;

    /**
     * @brief Get the native resolution of the display.
     * @return The native resolution of the display in pixels.
     * @remark On macOS this returns actual resolution selected for the display.
     */
    virtual Size nativeResolution() const                    = 0;

    /**
     * @brief Get the physical size of the display.
     * @return The physical size of the display in millimeters.
     */
    virtual SizeF physicalSize() const                       = 0;

    /**
     * @brief Get the DPI (dots per inch) of the display.
     * @return The DPI of the display.
     */
    virtual int dpi() const                                  = 0;

    /**
     * @brief Get the name of the display.
     * @return The name of the display as a string.
     */
    virtual const std::string& name() const                  = 0;

    /**
     * @brief Get the unique identifier of the display.
     * @return The unique identifier of the display as a string.
     */
    virtual const std::string& id() const                    = 0;

    /**
     * @brief Get the name of the adapter associated with the display.
     * @return The adapter name as a string.
     * @remark May return an empty string if the adapter name is not available
     */
    virtual const std::string& adapterName() const           = 0;

    /**
     * @brief Get the unique identifier of the adapter associated with the display.
     * @return The adapter identifier as a string.
     * @remark May return an empty string if the adapter identifier is not available
     */
    virtual const std::string& adapterId() const             = 0;

    /**
     * @brief Get the display flags.
     * @return The flags associated with the display.
     */
    virtual DisplayFlags flags() const                       = 0;

    /**
     * @brief Get the content scale factor of the display.
     * @return The content scale factor of the display.
     */
    virtual float contentScale() const                       = 0;

    /**
     * @brief Get the refresh rate of the display.
     * @return The refresh rate of the display in hertz.
     */
    virtual double refreshRate() const                       = 0;

    /**
     * @brief Convert a point from desktop coordinates to monitor coordinates.
     * @param pt The point in desktop coordinates.
     * @return The point in monitor coordinates.
     */
    virtual Point desktopToMonitor(Point pt) const           = 0;

    /**
     * @brief Convert a point from monitor coordinates to desktop coordinates.
     * @param pt The point in monitor coordinates.
     * @return The point in desktop coordinates.
     */
    virtual Point monitorToDesktop(Point pt) const           = 0;

    /**
     * @brief Get the backing scale factor of the display.
     * @return The backing scale factor of the display.
     * @remark macOS specific
     */
    virtual int backingScaleFactor() const                   = 0;

    virtual bool containsWindow(OsWindowHandle handle) const = 0;

    virtual OsDisplayHandle getHandle() const                = 0;

    /**
     * @brief Get all connected displays.
     * @return A vector of RC-pointers to all connected displays.
     */
    static std::vector<Rc<Display>> all();

    /**
     * @brief Get the primary display.
     * @return An RC-pointer to the primary display.
     */
    static Rc<Display> primary();
};

namespace Internal {
// Internal functions. Must be called from the main thread only
void updateDisplays();
} // namespace Internal

} // namespace Brisk
