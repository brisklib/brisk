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

#ifndef BRISK_WEBGPU
#error "Brisk was compiled without WebGPU support"
#endif

#include <brisk/gui/Gui.hpp>
#include <brisk/graphics/WebGpu.hpp>

namespace Brisk {

class WIDGET WebGpuWidget : public Widget {
    BRISK_DYNAMIC_CLASS(WebGpuWidget, Widget)
public:
    using Base                                   = Widget;
    constexpr static std::string_view widgetType = "webgpu";

    template <WidgetArgument... Args>
    explicit WebGpuWidget(const Args&... args)
        : WebGpuWidget{ Construction{ widgetType }, std::tuple{ args... } } {
        endConstruction();
    }

protected:
    explicit WebGpuWidget(Construction construction, ArgumentsView<WebGpuWidget> args);

    virtual void render(wgpu::Device device, wgpu::TextureView backBuffer) const = 0;

    void paint(Canvas& canvas) const override;
};
} // namespace Brisk
