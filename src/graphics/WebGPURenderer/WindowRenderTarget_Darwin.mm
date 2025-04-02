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
#include "WindowRenderTarget.hpp"
#include <brisk/graphics/OSWindowHandle.hpp>

#import "QuartzCore/CAMetalLayer.h"

namespace Brisk {

void WindowRenderTargetWebGPU::createSurface(const OSWindow* window) {
    @autoreleasepool {
        OSWindowHandle handle = window->getHandle();
        NSWindow* nsWindow    = handle.nsWindow();
        NSView* view          = [nsWindow contentView];

        [view setWantsLayer:YES];
        [view setLayer:[CAMetalLayer layer]];
        [[view layer] setContentsScale:[nsWindow backingScaleFactor]];

        wgpu::SurfaceDescriptor surfaceDesc;
        wgpu::SurfaceDescriptorFromMetalLayer surfaceDescMac{};
        surfaceDescMac.layer    = (__bridge_retained void*)([view layer]);
        surfaceDesc.nextInChain = &surfaceDescMac;
        m_surface               = m_device->m_instance.CreateSurface(&surfaceDesc);
    }
}

} // namespace Brisk
