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
#include <brisk/graphics/Renderer.hpp>
#include "Atlas.hpp"

namespace Brisk {

RenderResources::RenderResources()  = default;
RenderResources::~RenderResources() = default;

namespace Internal {
ImageBackend::~ImageBackend() = default;
}

static RC<RenderDevice> defaultDevice;

#ifdef BRISK_D3D11
expected<RC<RenderDevice>, RenderDeviceError> createRenderDeviceD3D11(
    RendererDeviceSelection deviceSelection);
#endif

#ifdef BRISK_WEBGPU
expected<RC<RenderDevice>, RenderDeviceError> createRenderDeviceWebGPU(
    RendererDeviceSelection deviceSelection);
#endif

expected<RC<RenderDevice>, RenderDeviceError> createRenderDevice(RendererBackend backend,
                                                                 RendererDeviceSelection deviceSelection) {
#ifdef BRISK_D3D11
    if (backend == RendererBackend::D3D11)
        return createRenderDeviceD3D11(deviceSelection);
#endif
#ifdef BRISK_WEBGPU
    return createRenderDeviceWebGPU(deviceSelection);
#else
    return nullptr;
#endif

#if !defined BRISK_D3D11 && !defined BRISK_WEBGPU
#error "Either BRISK_D3D11 or BRISK_WEBGPU must be defined, or both"
#endif
}

static RendererBackend defaultBackend          = RendererBackend::Default;
static RendererDeviceSelection deviceSelection = RendererDeviceSelection::HighPerformance;

void setRenderDeviceSelection(RendererBackend backend, RendererDeviceSelection selection) {
    defaultBackend  = backend;
    deviceSelection = selection;
}

static std::recursive_mutex mutex;

expected<RC<RenderDevice>, RenderDeviceError> getRenderDevice() {
    std::lock_guard lk(mutex);
    if (!defaultDevice) {
        auto device = createRenderDevice(defaultBackend, deviceSelection);
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

RenderPipeline::RenderPipeline(RC<RenderEncoder> encoder, RC<RenderTarget> target,
                               std::optional<ColorF> clear, Rectangle clipRect)
    : m_encoder(std::move(encoder)), m_resources(m_encoder->device()->resources()) {
    m_limits = m_encoder->device()->limits();
    m_encoder->begin(std::move(target), clear);
    m_clipRect = clipRect;
}

bool RenderPipeline::flush() {
    if (m_commands.empty())
        return false;
    BRISK_ASSERT(m_resources.currentCommand > m_resources.firstCommand);
    if (m_resources.currentCommand <= m_resources.firstCommand) {
        return false;
    }
    m_numBatches++;
    if (!m_clipRect.empty())
        m_encoder->batch(m_commands, m_data);

    m_textures.clear();
    m_commands.clear();
    m_data.clear();
    m_resources.firstCommand = m_resources.currentCommand;
    return true;
}

void RenderPipeline::command(RenderStateEx&& cmd, std::span<const float> data) {
    if (cmd.imageHandle) {
        m_encoder->device()->createImageBackend(cmd.imageHandle);
        cmd.imageBackend = Internal::getBackend(cmd.imageHandle);
        BRISK_ASSERT(cmd.imageBackend);
        cmd.textureId = 1; // Tell shader that texture is set (-1 if is not)
        m_textures.push_back(cmd.imageHandle);
    }

    if (m_data.size() + data.size() > m_limits.maxDataSize) {
        flush();
    }

    if (cmd.gradientHandle) {
        cmd.multigradient = m_resources.gradientAtlas->addEntry(cmd.gradientHandle, m_resources.firstCommand,
                                                                m_resources.currentCommand);
        if (cmd.multigradient == gradientNull) {
            flush();
            cmd.multigradient = m_resources.gradientAtlas->addEntry(
                cmd.gradientHandle, m_resources.firstCommand, m_resources.currentCommand);
            BRISK_ASSERT_MSG("Resource is too large for atlas", cmd.multigradient != gradientNull);
        }
    }
    SmallVector<SpriteOffset, 1> spriteIndices(cmd.sprites.size());
    for (size_t i = 0; i < cmd.sprites.size(); ++i) {
        RC<SpriteResource>& sprite = cmd.sprites[i];
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

    cmd.shaderClip = cmd.shaderClip.intersection(m_clipRect);

    m_commands.push_back(static_cast<const RenderState&>(cmd));
    m_data.insert(m_data.end(), data.begin(), data.end());
    // Add padding needed to align m_data to a multiple of 4.
    m_data.resize(alignUp(m_data.size(), 4), 0);

    if (cmd.shader == ShaderType::Text || cmd.shader == ShaderType::Mask ||
        cmd.shader == ShaderType::ColorMask) {
        float* cmdData        = m_data.data() + offs;
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

void RenderPipeline::setClipRect(Rectangle clipRect) {
    m_clipRect = clipRect;
}

void RenderPipeline::blit(RC<Image> image) {
    RenderStateEx style(ShaderType::Rectangles, nullptr);
    RectangleF rect{ {}, image->size() };
    style.textureMatrix = Matrix{};
    style.imageHandle   = std::move(image);
    command(std::move(style), one(GeometryRectangle{ rect, CornersF(0.f) }));
}

Rectangle RenderPipeline::clipRect() const {
    return m_clipRect;
}

void RenderResources::reset() {
    spriteAtlas.reset();
    gradientAtlas.reset();
}
} // namespace Brisk
