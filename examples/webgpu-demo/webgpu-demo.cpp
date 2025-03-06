#include <brisk/core/internal/Initialization.hpp>
#include <brisk/gui/GUIApplication.hpp>
#include <brisk/gui/GUIWindow.hpp>
#include <brisk/widgets/Layouts.hpp>
#include <brisk/widgets/Text.hpp>
#include <brisk/widgets/Button.hpp>
#include <brisk/widgets/Graphene.hpp>
#include <brisk/widgets/WebGPU.hpp>
#include <brisk/gui/Component.hpp>
#include <brisk/graphics/Fonts.hpp>
#include "brisk/gui/Icons.hpp"

namespace Brisk {

class WebGPUCubes final : public WebGPUWidget {
public:
    using WebGPUWidget::WebGPUWidget;

protected:
    void render(wgpu::Device device, wgpu::TextureView backBuffer) const {
        if (!m_device) {
            const_cast<WebGPUCubes*>(this)->setupPipeline(std::move(device));
        }

        // Setup render pass
        wgpu::RenderPassColorAttachment colorAttachment{ .view       = backBuffer,
                                                         .loadOp     = wgpu::LoadOp::Clear,
                                                         .storeOp    = wgpu::StoreOp::Store,
                                                         .clearValue = { 0.0f, 0.0f, 0.0f, 1.0f } };

        wgpu::RenderPassDescriptor renderPassDesc{ .colorAttachmentCount = 1,
                                                   .colorAttachments     = &colorAttachment };

        wgpu::CommandEncoder encoder = m_device.CreateCommandEncoder();

        float rotation               = currentTime();
        m_device.GetQueue().WriteBuffer(m_uniform, 0, &rotation, sizeof(rotation));
        wgpu::RenderPassEncoder renderPass = encoder.BeginRenderPass(&renderPassDesc);
        renderPass.SetPipeline(m_pipeline);
        renderPass.SetBindGroup(0, m_bindGroup);
        renderPass.Draw(3, 1);
        renderPass.End();
        wgpu::CommandBuffer commands = encoder.Finish();
        m_device.GetQueue().Submit(1, &commands);
    }

private:
    wgpu::Device m_device;
    wgpu::RenderPipeline m_pipeline;
    wgpu::Buffer m_uniform;
    wgpu::BindGroupLayout m_bindGroupLayout;
    wgpu::BindGroup m_bindGroup;

    void setupPipeline(wgpu::Device device) {
        m_device                 = std::move(device);
        // Create shader module (hardcoded WGSL)
        const char* shaderSource = R"Wgsl(
        
@group(0) @binding(0) var<uniform> rotation: f32;
        
struct VertexOutput {
    @builtin(position) position: vec4f,
    @location(0) @interpolate(linear) color: vec4f,
};

fn rotate2D(point: vec2<f32>, rotation: f32) -> vec2<f32> {
    let s = sin(rotation);
    let c = cos(rotation);
    let rotationMatrix = mat2x2<f32>(
        c, -s,
        s,  c
    );
    return rotationMatrix * point;
}

@vertex
fn vs_main(
    @builtin(vertex_index) VertexIndex : u32
) -> VertexOutput {
    var pos = array<vec2f, 3>(
        vec2(0.0, 1.0) * 0.75,
        vec2(-0.866, -0.5) * 0.75,
        vec2(0.866, -0.5) * 0.75
    );
    var col = array<vec3f, 3>(
        vec3(1.0, 0.0, 0.0),
        vec3(0.0, 1.0, 0.0),
        vec3(0.0, 0.0, 1.0)
    );
    var output: VertexOutput;
    output.position = vec4f(rotate2D(pos[VertexIndex], rotation), 0.0, 1.0);
    output.color    = vec4f(col[VertexIndex], 1.0);
    return output;
}

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4f {
    return in.color;
}

        )Wgsl";

        wgpu::ShaderModuleWGSLDescriptor wgslDesc{};
        wgslDesc.code = shaderSource;
        wgpu::ShaderModuleDescriptor shaderDesc{ .nextInChain = &wgslDesc };
        wgpu::ShaderModule shaderModule = m_device.CreateShaderModule(&shaderDesc);

        wgpu::BindGroupLayoutEntry bindEntries{
            .binding    = 0,
            .visibility = wgpu::ShaderStage::Vertex,
            .buffer =
                wgpu::BufferBindingLayout{
                    .type           = wgpu::BufferBindingType::Uniform,
                    .minBindingSize = sizeof(float),
                },
        };

        wgpu::BindGroupLayoutDescriptor bindGroupLayoutDesc;
        bindGroupLayoutDesc.entryCount = 1;
        bindGroupLayoutDesc.entries    = &bindEntries;
        m_bindGroupLayout              = m_device.CreateBindGroupLayout(&bindGroupLayoutDesc);

        wgpu::PipelineLayoutDescriptor pipelineLayoutDesc;
        pipelineLayoutDesc.bindGroupLayoutCount = 1;
        pipelineLayoutDesc.bindGroupLayouts     = &m_bindGroupLayout;

        wgpu::PipelineLayout pipelineLayout     = m_device.CreatePipelineLayout(&pipelineLayoutDesc);

        // Create pipeline
        wgpu::RenderPipelineDescriptor pipelineDesc{
            .layout    = pipelineLayout,
            .vertex    = { .module = shaderModule, .entryPoint = "vs_main" },
            .primitive = { .topology = wgpu::PrimitiveTopology::TriangleList,
                           .cullMode = wgpu::CullMode::Back },
        };

        wgpu::ColorTargetState colorTarget{ .format    = wgpu::TextureFormat::BGRA8Unorm,
                                            .writeMask = wgpu::ColorWriteMask::All };

        wgpu::FragmentState fragmentState{
            .module = shaderModule, .entryPoint = "fs_main", .targetCount = 1, .targets = &colorTarget
        };
        pipelineDesc.fragment = &fragmentState;

        m_pipeline            = m_device.CreateRenderPipeline(&pipelineDesc);

        wgpu::BufferDescriptor bufferDesc;
        bufferDesc.size  = sizeof(float);
        bufferDesc.usage = wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst;
        m_uniform        = m_device.CreateBuffer(&bufferDesc);

        wgpu::BindGroupEntry bindGroupEntry{};
        bindGroupEntry.binding = 0;
        bindGroupEntry.buffer  = m_uniform;
        bindGroupEntry.size    = sizeof(float);

        wgpu::BindGroupDescriptor bindGroupDesc{};
        bindGroupDesc.layout     = m_bindGroupLayout;
        bindGroupDesc.entryCount = 1;
        bindGroupDesc.entries    = &bindGroupEntry;
        m_bindGroup              = m_device.CreateBindGroup(&bindGroupDesc);
    }
};

class AppComponent final : public Component {
public:
    explicit AppComponent() {}

    RC<Widget> build() final {
        return rcnew WebGPUCubes{
            stylesheet = Graphene::stylesheet(),
            Graphene::darkColors(),
            layout         = Layout::Vertical,
            alignItems     = Align::Center,
            justifyContent = Justify::Center,
            gapRow         = 8_px,
            rcnew Text{
                "This is a demo showing how to render 3D content using the WebGPU API in Brisk "
                "applications.",
                wordWrap = true,
            },
            rcnew Button{
                rcnew Text{ "Quit" },
                onClick = staticLifetime |
                          []() {
                              guiApplication->quit();
                          },
            },
        };
    }

    void configureWindow(RC<GUIWindow> window) final {
        window->setTitle("WebGPU Demo"_tr);
        window->setSize({ 640, 640 });
        window->windowFit = WindowFit::MinimumSize;
        window->setStyle(WindowStyle::Normal);
    }
};

} // namespace Brisk

using namespace Brisk;

int briskMain() {
    GUIApplication application;
    Internal::bufferedRendering = false;
    setRenderDeviceSelection(RendererBackend::WebGPU, RendererDeviceSelection::HighPerformance);

    return application.run(createComponent<AppComponent>());
}
