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
 */                                                                                                          \
#pragma once
#include "Image.hpp"
#include <brisk/core/internal/Expected.hpp>
#include <mutex>
#include "RenderState.hpp"
#include "Color.hpp"
#include "Geometry.hpp"
#include <brisk/graphics/OsWindowHandle.hpp>
#include <brisk/graphics/OsDisplayHandle.hpp>
#include <brisk/core/MetaClass.hpp>

namespace Brisk {

/**
 * @enum RendererBackend
 * @brief Specifies the rendering backends available for the platform.
 */
enum class RendererBackend : int {
#ifdef BRISK_D3D11
    D3d11 = 1, ///< Direct3D 11 backend available on Windows.
#endif
#ifdef BRISK_WEBGPU
    WebGpu = 2, ///< WebGPU backend.
#endif
#ifdef BRISK_D3D11
    Default = D3d11, ///< Default backend option.
#else
    Default = WebGpu,
#endif
};

/**
 * @brief A list of available renderer backends based on platform compilation settings.
 */
constexpr inline std::initializer_list<RendererBackend> rendererBackends{
#ifdef BRISK_D3D11
    RendererBackend::D3d11,
#endif
#ifdef BRISK_WEBGPU
    RendererBackend::WebGpu,
#endif
};

/**
 * @brief Default names for available renderer backends.
 */
template <>
inline constexpr std::initializer_list<NameValuePair<RendererBackend>> defaultNames<RendererBackend>{
#ifdef BRISK_D3D11
    { "D3d11", RendererBackend::D3d11 },
#endif
#ifdef BRISK_WEBGPU
    { "WebGpu", RendererBackend::WebGpu },
#endif
};

/**
 * @def BRISK_SUBPIXEL_DEFAULT
 * @brief Defines the default value for subpixel rendering based on the platform.
 */
#ifdef BRISK_MACOS
#define BRISK_SUBPIXEL_DEFAULT false
#else
#define BRISK_SUBPIXEL_DEFAULT true
#endif

/**
 * @struct VisualSettings
 * @brief Contains settings for visual adjustments during rendering.
 */
struct VisualSettings {
    float blueLightFilter = 0;                      ///< Adjusts blue light filtering. Default is 0.
    float gamma           = 1;                      ///< Controls the gamma correction. Default is 1.
    bool subPixelText     = BRISK_SUBPIXEL_DEFAULT; ///< Enables or disables subpixel text rendering.
};

/**
 * @enum RendererDeviceSelection
 * @brief Specifies the performance level when selecting a rendering device.
 */
enum class RendererDeviceSelection {
    HighPerformance, ///< Selects a high-performance rendering device.
    LowPower,        ///< Selects a low-power rendering device.
    Default,         ///< Selects the default device.
};

/**
 * @struct RenderDeviceInfo
 * @brief Holds information about the rendering device being used.
 */
struct RenderDeviceInfo {
    std::string api;    ///< The name of the rendering API.
    int apiVersion;     ///< The version of the rendering API.
    std::string vendor; ///< The vendor of the rendering device.
    std::string device; ///< The specific device name.

    inline static const std::tuple reflection = {
        ReflectionField{ "api", &RenderDeviceInfo::api },
        ReflectionField{ "apiVersion", &RenderDeviceInfo::apiVersion },
        ReflectionField{ "vendor", &RenderDeviceInfo::vendor },
        ReflectionField{ "device", &RenderDeviceInfo::device },
    };
};

enum class RenderTargetType {
    Window,
    Image,
};

/**
 * @class RenderTarget
 * @brief Abstract base class representing a render target.
 */
class RenderTarget {
public:
    /**
     * @brief Returns the size of the render target.
     * @return The size of the render target.
     */
    virtual Size size() const                      = 0;

    virtual RenderTargetType type() const noexcept = 0;
};

class SpriteAtlas;
class GradientAtlas;

/**
 * @struct RenderResources
 * @brief Manages resources used during rendering.
 */
struct RenderResources {
    RenderResources();  ///< Constructor.
    ~RenderResources(); ///< Destructor.

    std::recursive_mutex mutex;
    std::unique_ptr<SpriteAtlas> spriteAtlas;
    std::unique_ptr<GradientAtlas> gradientAtlas;
    uint64_t firstCommand   = 0;
    uint64_t currentCommand = 0;

    void reset();
};

/**
 * @struct RenderLimits
 * @brief Defines limits on rendering resources.
 */
struct RenderLimits {
    size_t maxDataSize;  ///< Maximum buffer size for rendering data (in floats).
    size_t maxAtlasSize; ///< Maximum size of texture atlases (in bytes).
    size_t maxGradients; ///< Maximum number of gradients allowed.
};

class RenderDevice;

/**
 * @class RenderEncoder
 * @brief Abstract base class representing a rendering encoder.
 */
class RenderEncoder {
public:
    /**
     * @brief Returns the rendering device associated with this encoder.
     * @return Pointer to the RenderDevice.
     */
    virtual RenderDevice* device() const                                                            = 0;

    /**
     * @brief Gets the visual settings for the encoder.
     * @return The visual settings.
     */
    virtual VisualSettings visualSettings() const                                                   = 0;

    /**
     * @brief Sets the visual settings for the encoder.
     * @param visualSettings The visual settings to apply.
     */
    virtual void setVisualSettings(const VisualSettings& visualSettings)                            = 0;

    /**
     * @brief Begins the rendering operation.
     * @param target The render target.
     * @param clear The clear color. std::nullopt means no clear command is issued.
     */
    virtual void begin(Rc<RenderTarget> target, std::optional<ColorF> clear = Palette::transparent) = 0;

    /**
     * @brief Batches rendering commands.
     * @param commands The rendering commands.
     * @param data Associated data.
     */
    virtual void batch(std::span<const RenderState> commands, std::span<const float> data)          = 0;

    /**
     * @brief Ends the rendering operation.
     */
    virtual void end()                                                                              = 0;

    /**
     * @brief Waits for the rendering to finish.
     */
    virtual void wait()                                                                             = 0;

    /**
     * @brief Gets current bound target (the target passed to `begin` function).
     * @return Pointer to the current RenderTarget, or nullptr if `begin` has not been called or `end` has
     * been called.
     */
    virtual Rc<RenderTarget> currentTarget() const                                                  = 0;

    constexpr static uint32_t maxDurations                                                          = 256;

    using DurationCallback = std::function<void(uint64_t, std::span<const std::chrono::nanoseconds>)>;

    virtual void beginFrame(uint64_t frameId)        = 0;
    virtual void endFrame(DurationCallback callback) = 0;
};

/**
 * @class RenderPipeline
 * @brief Represents the rendering pipeline responsible for managing and executing rendering operations.
 */
class RenderPipeline final : public RenderContext {
    BRISK_DYNAMIC_CLASS(RenderPipeline, RenderContext)
public:
    /**
     * @brief Constructs a RenderPipeline with an encoder and target.
     * @param encoder The render encoder.
     * @param target The render target.
     * @param clear Optional clear color for the render target (defaults to transparent); if
     * std::nullopt is passed, the target is not cleared, enabling blending with previous content.
     * @param clipRect The clipping rectangle in screen coordinates; use noClipRect to disable clipping.
     */
    RenderPipeline(Rc<RenderEncoder> encoder, Rc<RenderTarget> target,
                   std::optional<ColorF> clear = Palette::transparent, Rectangle clipRect = noClipRect);

    /**
     * @brief Blits an image to the render target.
     * @param image The image to blit; its size must match the target size.
     */
    void blit(Rc<Image> image);

    /**
     * @brief Destroys the RenderPipeline instance.
     */
    ~RenderPipeline();

    /**
     * @brief Retrieves the current clipping rectangle.
     * @return The current clipping rectangle in screen coordinates.
     */
    Rectangle clipRect() const;

    /**
     * @brief Sets the clipping rectangle for rendering operations.
     * @param clipRect The new clipping rectangle in screen coordinates (origin at top-left).
     */
    void setClipRect(Rectangle clipRect) final;

    using RenderContext::command;

    /**
     * @brief Issues a rendering command.
     * @param cmd The render state command.
     * @param data Optional associated data.
     */
    void command(RenderStateEx&& cmd, std::span<const float> data = {}) final;

    /**
     * @brief Retrieves the number of batches processed.
     * @return The number of batches.
     */
    int numBatches() const final;

    /**
     * @brief Retrieves the render encoder associated with this pipeline.
     * @return A reference-counted pointer to the render encoder.
     */
    Rc<RenderEncoder> encoder() const {
        return m_encoder;
    }

    /**
     * @brief Flushes the pipeline to issue the batched commands.
     * @return True if some commands were sent to underlying device.
     */
    bool flush();

private:
    Rc<RenderEncoder> m_encoder;         ///< The current rendering encoder.
    RenderLimits m_limits;               ///< Resource limits for the pipeline.
    RenderResources& m_resources;        ///< Rendering resources.
    std::vector<RenderState> m_commands; ///< List of rendering commands queued for execution.
    std::vector<float> m_data;           ///< Buffer for associated rendering data.
    std::vector<Rc<Image>> m_textures;   ///< List of textures used in rendering.
    int m_numBatches = 0;                ///< Number of rendering batches.
    Rectangle m_clipRect;                ///< The current clipping rectangle.
};

/**
 * @class WindowRenderTarget
 * @brief Represents a render target for window-based rendering.
 */
class WindowRenderTarget : public RenderTarget {
public:
    RenderTargetType type() const noexcept final {
        return RenderTargetType::Window;
    }

    /**
     * @brief Resizes the backbuffer.
     * @param size The new size of the backbuffer.
     */
    virtual void resizeBackbuffer(Size size)    = 0;

    /**
     * @brief Presents the rendered frame to the window.
     */
    virtual void present()                      = 0;

    /**
     * @brief Gets the VSync interval.
     * @return The VSync interval (0 means no VSync).
     */
    virtual int vsyncInterval() const           = 0;

    /**
     * @brief Sets the VSync interval.
     * @param interval The VSync interval to set (0 means no VSync).
     */
    virtual void setVSyncInterval(int interval) = 0;
};

/**
 * @class ImageRenderTarget
 * @brief Represents a render target for image-based rendering.
 */
class ImageRenderTarget : public RenderTarget {
public:
    RenderTargetType type() const noexcept final {
        return RenderTargetType::Image;
    }

    /**
     * @brief Sets the size of the render target.
     * @param newSize The new size.
     */
    virtual void setSize(Size newSize) = 0;

    /**
     * @brief Returns the rendered image.
     * @return The rendered image.
     */
    virtual Rc<Image> image() const    = 0;
};

/**
 * @enum DepthStencilType
 * @brief Describes the available depth-stencil buffer formats.
 */
enum class DepthStencilType {
    None,  ///< No depth-stencil buffer.
    D24S8, ///< 24-bit depth, 8-bit stencil buffer.
    D32,   ///< 32-bit depth buffer.
};

/**
 * @enum BlendMode
 * @brief Defines different blending modes for combining colors.
 *
 * Blending modes determine how two colors (source and destination) are combined
 * when rendering graphics. The result depends on the mathematical formula associated
 * with each mode.
 *
 * The formulas use the following notation:
 * - Csrc : Source color
 * - Cdst : Destination color
 * - Cout : Output color
 *
 * @note Color values are typically in the range [0,1].
 */
enum class BlendMode {
    /**
     * @brief Normal blending mode.
     *
     * The source color simply replaces the destination color.
     * Cout = Csrc
     */
    Normal,

    /**
     * @brief Multiply blending mode.
     *
     * The source and destination colors are multiplied together,
     * resulting in a darker image.
     * Cout = Csrc × Cdst
     */
    Multiply,

    /**
     * @brief Screen blending mode.
     *
     * The inverse of the multiplied inverse colors, resulting in a lighter image.
     * Cout = 1 - (1 - Csrc) × (1 - Cdst)
     */
    Screen,

    /**
     * @brief Difference blending mode.
     *
     * The absolute difference between the source and destination colors.
     * Cout = |Csrc - Cdst|
     */
    Difference,
};

/**
 * @class OsWindow
 * @brief Represents a platform-specific window handle.
 */
class OsWindow {
public:
    /**
     * @brief Returns the size of the framebuffer.
     * @return The framebuffer size.
     */
    virtual Size framebufferSize() const     = 0;

    /**
     * @brief Gets the native OS window handle.
     */
    virtual OsWindowHandle getHandle() const = 0;
};

/**
 * @enum RenderDeviceError
 * @brief Specifies the possible errors that can occur in a rendering device.
 */
enum class RenderDeviceError {
    ShaderError,   ///< Error related to shader compilation.
    Unsupported,   ///< The requested operation or feature is unsupported.
    InternalError, ///< An internal error occurred in the rendering device.
};

/**
 * @brief Default names for rendering device errors.
 */
template <>
inline constexpr std::initializer_list<NameValuePair<RenderDeviceError>> defaultNames<RenderDeviceError>{
    { "Unsupported", RenderDeviceError::Unsupported },
    { "ShaderError", RenderDeviceError::ShaderError },
    { "InternalError", RenderDeviceError::InternalError },
};

/**
 * @class RenderDevice
 * @brief Abstract base class for a rendering device.
 */
class RenderDevice {
public:
    /**
     * @brief Returns information about the rendering device.
     * @return RenderDeviceInfo object.
     */
    virtual RenderDeviceInfo info() const                              = 0;

    virtual RendererBackend backend() const noexcept                   = 0;

    /**
     * @brief Creates a render target for a window.
     * @param window The window to render to.
     * @param type Pixel type.
     * @param depth Depth-stencil format.
     * @param samples Number of samples for multisampling.
     * @return The window render target.
     */
    virtual Rc<WindowRenderTarget> createWindowTarget(const OsWindow* window,
                                                      PixelType type         = PixelType::U8Gamma,
                                                      DepthStencilType depth = DepthStencilType::None,
                                                      int samples            = 1) = 0;

    /**
     * @brief Creates a render target for off-screen image rendering.
     * @param frameSize The size of the image.
     * @param type Pixel type.
     * @param depth Depth-stencil format.
     * @param samples Number of samples for multisampling.
     * @return The image render target.
     */
    virtual Rc<ImageRenderTarget> createImageTarget(Size frameSize, PixelType type = PixelType::U8Gamma,
                                                    DepthStencilType depth = DepthStencilType::None,
                                                    int samples            = 1)   = 0;

    /**
     * @brief Creates a new render encoder.
     * @return The render encoder.
     */
    virtual Rc<RenderEncoder> createEncoder()                          = 0;

    /**
     * @brief Returns the resources used for rendering.
     * @return Reference to RenderResources.
     */
    virtual RenderResources& resources()                               = 0;

    /**
     * @brief Returns the rendering limits for the device.
     * @return RenderLimits object.
     */
    virtual RenderLimits limits() const                                = 0;

    /**
     * @brief Creates a backend representation of an image.
     * @param image The image to create a backend for.
     */
    virtual void createImageBackend(Rc<Image> image)                   = 0;
};

extern RendererBackend defaultBackend;
extern RendererDeviceSelection deviceSelection;

/**
 * @brief Sets the rendering device selection based on backend and device selection options.
 * @param backend The rendering backend to use.
 * @param deviceSelection The device selection criteria.
 */
void setRenderDeviceSelection(RendererBackend backend, RendererDeviceSelection deviceSelection);

/**
 * @brief Gets the current rendering device, if available.
 * @return Expected object containing the rendering device or an error.
 */
expected<Rc<RenderDevice>, RenderDeviceError> getRenderDevice(OsDisplayHandle display = {});

/**
 * @brief Frees the currently allocated rendering device.
 */
void freeRenderDevice();

/**
 * @brief Creates a new rendering device with specified backend and device selection criteria.
 * @param backend The rendering backend to use.
 * @param deviceSelection The device selection criteria.
 * @return Expected object containing the rendering device or an error.
 */
expected<Rc<RenderDevice>, RenderDeviceError> createRenderDevice(RendererBackend backend,
                                                                 RendererDeviceSelection deviceSelection,
                                                                 OsDisplayHandle display = {});

} // namespace Brisk
