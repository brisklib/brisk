#include <brisk/widgets/WebGPU.hpp>

namespace Brisk {

void WebGPUWidget::paint(Canvas& canvas) const {
    wgpu::Device device;
    wgpu::TextureView backBuffer;
    if (webgpuFromContext(canvas.renderContext(), device, backBuffer)) {
        render(device, backBuffer);
    }
}

WebGPUWidget::WebGPUWidget(Construction construction, ArgumentsView<WebGPUWidget> args)
    : Base(construction, nullptr) {
    args.apply(this);
}
} // namespace Brisk
