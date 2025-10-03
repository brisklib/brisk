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
 * You should have received a copy of the GNU General Public License along with this program.
 * If not, see <http://www.gnu.org/licenses/>.
 *
 * If you do not wish to be bound by the GPL-2.0+ license, you must purchase a commercial
 * license. For commercial licensing options, please visit: https://brisklib.com
 */
#include <brisk/graphics/Renderer.hpp>
#include "Atlas.hpp"

namespace Brisk {

RenderResources::RenderResources()  = default;
RenderResources::~RenderResources() = default;

namespace Internal {
ImageBackend::~ImageBackend() = default;
}

static Rc<RenderDevice> defaultDevice;

#ifdef BRISK_D3D11
expected<Rc<RenderDevice>, RenderDeviceError> createRenderDeviceD3d11(RendererDeviceSelection deviceSelection,
                                                                      NativeDisplayHandle display);
#endif

#ifdef BRISK_WEBGPU
expected<Rc<RenderDevice>, RenderDeviceError> createRenderDeviceWebGpu(
    RendererDeviceSelection deviceSelection, NativeDisplayHandle display);
#endif

expected<Rc<RenderDevice>, RenderDeviceError> createRenderDevice(RendererBackend backend,
                                                                 RendererDeviceSelection deviceSelection,
                                                                 NativeDisplayHandle display) {
#ifdef BRISK_D3D11
    if (backend == RendererBackend::D3d11)
        return createRenderDeviceD3d11(deviceSelection, display);
#endif
#ifdef BRISK_WEBGPU
    return createRenderDeviceWebGpu(deviceSelection, display);
#else
    return nullptr;
#endif

#if !defined BRISK_D3D11 && !defined BRISK_WEBGPU
#error "Either BRISK_D3D11 or BRISK_WEBGPU must be defined, or both"
#endif
}

RendererBackend defaultBackend          = RendererBackend::Default;
RendererDeviceSelection deviceSelection = RendererDeviceSelection::HighPerformance;

void setRenderDeviceSelection(RendererBackend backend, RendererDeviceSelection selection) {
    defaultBackend  = backend;
    deviceSelection = selection;
}

static std::recursive_mutex mutex;

expected<Rc<RenderDevice>, RenderDeviceError> getRenderDevice(NativeDisplayHandle display) {
    std::lock_guard lk(mutex);
    if (!defaultDevice) {
        auto device = createRenderDevice(defaultBackend, deviceSelection, display);
        if (!device)
            return device;
        return defaultDevice = *device;
    }
    return defaultDevice;
}

void freeRenderDevice() {
    std::lock_guard lk(mutex);
    defaultDevice.reset();
}

RenderPipeline::RenderPipeline(Rc<RenderEncoder> encoder, Rc<RenderTarget> target,
                               std::optional<ColorF> clear, Rectangle clipRect)
    : m_encoder(std::move(encoder)), m_resources(m_encoder->device()->resources()) {
    m_limits = m_encoder->device()->limits();
    m_encoder->begin(std::move(target), clear);
    m_globalScissor = clipRect;
}

bool RenderPipeline::flush() {
    if (m_commands.empty())
        return false;
    BRISK_ASSERT(m_resources.currentCommand > m_resources.firstCommand);
    if (m_resources.currentCommand <= m_resources.firstCommand) {
        return false;
    }
    m_numBatches++;
    if (!m_globalScissor.empty()) {
        m_encoder->batch(m_commands, m_data);
    }

    m_textures.clear();
    m_commands.clear();
    m_data.clear();
    m_resources.firstCommand = m_resources.currentCommand;
    return true;
}

void RenderPipeline::command(RenderStateEx&& cmd, std::span<const uint32_t> data) {
    cmd.scissor = cmd.scissor.intersection(m_globalScissor);
    if (cmd.scissor.empty())
        return;

    if (cmd.sourceImageHandle) {
        m_encoder->device()->createImageBackend(cmd.sourceImageHandle);
        cmd.sourceImage = Internal::getBackend(cmd.sourceImageHandle);
        BRISK_ASSERT(cmd.sourceImage);
        cmd.hasTexture = true; // Tell shader that texture is set (-1 if is not)
        m_textures.push_back(cmd.sourceImageHandle);
    }
    if (cmd.backImageHandle) {
        m_encoder->device()->createImageBackend(cmd.backImageHandle);
        cmd.backImage = Internal::getBackend(cmd.backImageHandle);
        BRISK_ASSERT(cmd.backImage);
        m_textures.push_back(cmd.backImageHandle);
    }

    if (m_data.size() + data.size() > m_limits.maxDataSize) {
        flush();
    }

    if (!m_encoder->visualSettings().subPixelText) {
        cmd.subpixelMode = SubpixelMode::Off;
    }

    if (cmd.gradientHandle) {
        cmd.gradientIndex = m_resources.gradientAtlas->addEntry(cmd.gradientHandle, m_resources.firstCommand,
                                                                m_resources.currentCommand);
        if (cmd.gradientIndex == gradientNull) {
            flush();
            cmd.gradientIndex = m_resources.gradientAtlas->addEntry(
                cmd.gradientHandle, m_resources.firstCommand, m_resources.currentCommand);
            BRISK_ASSERT_MSG("Resource is too large for atlas", cmd.gradientIndex != gradientNull);
        }
    }
    SmallVector<SpriteOffset, 1> spriteIndices(cmd.sprites.size());
    for (size_t i = 0; i < cmd.sprites.size(); ++i) {
        Rc<SpriteResource>& sprite = cmd.sprites[i];
        spriteIndices[i] =
            m_resources.spriteAtlas->addEntry(sprite, m_resources.firstCommand, m_resources.currentCommand);
        if (spriteIndices[i] == spriteNull) {
            flush();
            spriteIndices[i] = m_resources.spriteAtlas->addEntry(sprite, m_resources.firstCommand,
                                                                 m_resources.currentCommand);
            BRISK_ASSERT_MSG("Resource is too large for atlas", spriteIndices[i] != spriteNull);
            BRISK_ASSERT(spriteIndices[i] <= 16777216);
        }
        sprite.reset(); // Free memory
    }

    size_t offs    = m_data.size();

    cmd.dataOffset = offs / 4;
    cmd.dataSize   = data.size();

    m_commands.push_back(static_cast<const RenderState&>(cmd));
    m_data.insert(m_data.end(), data.begin(), data.end());
    // Add padding needed to align m_data to a multiple of 4.
    m_data.resize(alignUp(m_data.size(), 4), 0);

    if (cmd.shader == ShaderType::Text || cmd.shader == ShaderType::ColorMask) {
        uint32_t* cmdData     = m_data.data() + offs;
        GeometryGlyph* glyphs = reinterpret_cast<GeometryGlyph*>(cmdData);
        for (size_t i = 0; i < data.size_bytes() / sizeof(GeometryGlyph); ++i) {
            int32_t idx = static_cast<int>(glyphs[i].sprite);
            BRISK_ASSERT(idx < spriteIndices.size());
            glyphs[i].sprite = static_cast<float>(spriteIndices[idx]);
        }
    }

    ++m_resources.currentCommand;
}

RenderPipeline::~RenderPipeline() {
    if (std::uncaught_exceptions() == 0) {
        flush();
        m_encoder->end();
    }
    m_textures.clear();
    m_commands.clear();
    m_data.clear();
}

int RenderPipeline::numBatches() const {
    return m_numBatches;
}

void RenderPipeline::setGlobalScissor(Rectangle clipRect) {
    m_globalScissor = clipRect;
}

void RenderPipeline::blit(Rc<Image> image) {
    RenderStateEx style(ShaderType::Blit, nullptr);
    style.sourceImageHandle = std::move(image);
    command(std::move(style), {});
}

Rectangle RenderPipeline::globalScissor() const {
    return m_globalScissor;
}

void RenderResources::reset() {
    spriteAtlas.reset();
    gradientAtlas.reset();
}
} // namespace Brisk
