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

#include <brisk/core/Utilities.hpp>
#include <brisk/core/Reflection.hpp>
#include <catch2/catch_all.hpp>
#include "Catch2Utils.hpp"
#include "VisualTests.hpp"
#include <brisk/core/Time.hpp>
#include <brisk/graphics/Image.hpp>
#include <brisk/graphics/RawCanvas.hpp>
#include <brisk/graphics/Canvas.hpp>

#include <brisk/graphics/OSWindowHandle.hpp>
#include <brisk/graphics/Palette.hpp>

#ifdef HAVE_GLFW3
#include <GLFW/glfw3.h>
#ifdef _WIN32
#define GLFW_EXPOSE_NATIVE_WIN32
#endif
#ifdef __APPLE__
#define GLFW_EXPOSE_NATIVE_COCOA
extern "C" id objc_retain(id value);
#endif
#ifdef __linux__
#define GLFW_EXPOSE_NATIVE_X11
#endif
#include <GLFW/glfw3native.h>
#endif

namespace Brisk {

TEST_CASE("Renderer Info", "[gpu]") {
    expected<RC<RenderDevice>, RenderDeviceError> device_ = getRenderDevice();
    REQUIRE(device_.has_value());
    RC<RenderDevice> device = *device_;
    RenderDeviceInfo info   = device->info();
#ifdef BRISK_DEBUG_GPU
    fmt::print("#########################################################\n");
    fmt::print("{}\n", info);
    fmt::print("#########################################################\n");
#endif
}

template <typename Fn>
static void renderTest(const std::string& referenceImageName, Size size, Fn&& fn,
                       ColorF backColor = Palette::transparent, float minimumPSNR = 40.f) {

    for (RendererBackend bk : rendererBackends) {
        INFO(fmt::to_string(bk));
        expected<RC<RenderDevice>, RenderDeviceError> device_ =
            createRenderDevice(bk, RendererDeviceSelection::Default);
        REQUIRE(device_.has_value());
        RC<RenderDevice> device = *device_;
        auto info               = device->info();
        REQUIRE(!info.api.empty());
        REQUIRE(!info.vendor.empty());
        REQUIRE(!info.device.empty());

        RC<ImageRenderTarget> target = device->createImageTarget(size, PixelType::U8Gamma);

        REQUIRE(!!target.get());
        REQUIRE(target->size() == size);

        RC<RenderEncoder> encoder = device->createEncoder();
        encoder->setVisualSettings(VisualSettings{ .blueLightFilter = 0, .gamma = 1, .subPixelText = false });

        visualTest(
            referenceImageName, size,
            [&](RC<ImageRGBA> image) {
                {
                    RenderPipeline pipeline(encoder, target, backColor);
                    fn(static_cast<RenderContext&>(pipeline));
                }
                encoder->wait();
                auto out = target->imageAs<PixelType::U8Gamma>();
                image->copyFrom(out);
            },
            minimumPSNR);
    }
}

TEST_CASE("Renderer devices", "[gpu]") {
    expected<RC<RenderDevice>, RenderDeviceError> d;
#ifdef BRISK_WINDOWS
    d = createRenderDevice(RendererBackend::D3D11, RendererDeviceSelection::HighPerformance);
    REQUIRE(d.has_value());
    fmt::print("[D3D11] HighPerformance: {}\n", (*d)->info().device);
    d = createRenderDevice(RendererBackend::D3D11, RendererDeviceSelection::LowPower);
    REQUIRE(d.has_value());
    fmt::print("[D3D11] LowPower: {}\n", (*d)->info().device);
    d = createRenderDevice(RendererBackend::D3D11, RendererDeviceSelection::Default);
    REQUIRE(d.has_value());
    fmt::print("[D3D11] Default: {}\n", (*d)->info().device);
#endif

#ifdef BRISK_WEBGPU
    d = createRenderDevice(RendererBackend::WebGPU, RendererDeviceSelection::HighPerformance);
    REQUIRE(d.has_value());
    fmt::print("[WebGPU] HighPerformance: {}\n", (*d)->info().device);
    d = createRenderDevice(RendererBackend::WebGPU, RendererDeviceSelection::LowPower);
    REQUIRE(d.has_value());
    fmt::print("[WebGPU] LowPower: {}\n", (*d)->info().device);
    d = createRenderDevice(RendererBackend::WebGPU, RendererDeviceSelection::Default);
    REQUIRE(d.has_value());
    fmt::print("[WebGPU] Default: {}\n", (*d)->info().device);
#endif
}

TEST_CASE("Renderer - fonts") {
    auto ttf = readBytes(fs::path(PROJECT_SOURCE_DIR) / "resources" / "fonts" / "Lato-Medium.ttf");
    REQUIRE(ttf.has_value());
    fonts->addFont(FontFamily(44), FontStyle::Normal, FontWeight::Regular, *ttf, true, FontFlags::Default);

    renderTest(
        "rr-fonts", { 1200, 600 },
        [&](RenderContext& context) {
            RawCanvas canvas(context);

            Rectangle rect;
            ColorF c;
            for (int i = 0; i < 10; ++i) {
                c    = ColorOf<float, ColorGamma::sRGB>(i / 9.f);
                rect = Rectangle{ 0, i * 60, 600, (i + 1) * 60 };
                canvas.drawRectangle(rect, 0.f, 0.f, fillColor = c, strokeWidth = 0);
                canvas.drawText(rect, 0.5f, 0.5f, "The quick brown fox jumps over the lazy dog",
                                Font{ FontFamily(44), 27.f }, Palette::white);
                c    = ColorOf<float, ColorGamma::sRGB>(1.f - i / 9.f);
                rect = Rectangle{ 600, i * 60, 1200, (i + 1) * 60 };
                canvas.drawRectangle(rect, 0.f, 0.f, fillColor = c, strokeWidth = 0);
                canvas.drawText(rect, 0.5f, 0.5f, "The quick brown fox jumps over the lazy dog",
                                Font{ FontFamily(44), 27.f }, Palette::black);
            }
        },
        ColorF{ 1.f, 1.f });
}

TEST_CASE("Renderer", "[gpu]") {
    const Rectangle frameBounds = Rectangle{ 0, 0, 480, 320 };
    RectangleF rect             = frameBounds.withPadding(10);
    float radius                = frameBounds.shortestSide() * 0.2f;
    float strokeWidth           = frameBounds.shortestSide() * 0.05f;

    SECTION("RawCanvas") {
        renderTest(
            "rr-ll", frameBounds.size(),
            [&](RenderContext& context) {
                RawCanvas canvas(context);
                canvas.drawRectangle(
                    rect, radius, 0.f,
                    linearGradient = { frameBounds.at(0.1f, 0.1f), frameBounds.at(0.9f, 0.9f) },
                    fillColors     = { Palette::Standard::green, Palette::Standard::red },
                    strokeColor = Palette::black, Arg::strokeWidth = strokeWidth);
            },
            ColorF{ 0.5f, 0.5f, 0.5f, 1.f });
    }

    SECTION("Canvas") {
        renderTest(
            "rr", frameBounds.size(),
            [&](RenderContext& context) {
                Canvas canvas(context);
                Path path;
                path.addRoundRect(rect, radius);
                canvas.setStrokeWidth(strokeWidth);
                canvas.setStrokeColor(Palette::black);
                InplacePtr<Gradient> grad(GradientType::Linear, frameBounds.at(0.1f, 0.1f),
                                          frameBounds.at(0.9f, 0.9f));
                grad->addStop(0.f, Palette::Standard::green);
                grad->addStop(1.f, Palette::Standard::red);
                canvas.setFillPaint(grad);
                canvas.fillPath(path);
                canvas.strokePath(path);
            },
            ColorF{ 0.5f, 0.5f, 0.5f, 1.f });
    }
}

#if defined HAVE_GLFW3 && defined BRISK_INTERACTIVE_TESTS
class OSWindowGLFW final : public OSWindow {
public:
    Size framebufferSize() const final {
        Size size;
        glfwGetFramebufferSize(win, &size.x, &size.y);
        return size;
    }

    void getHandle(OSWindowHandle& handle) const final {
#ifdef BRISK_WINDOWS
        handle.window = glfwGetWin32Window(win);
#endif
#ifdef BRISK_MACOS
        handle.window = objc_retain(glfwGetCocoaWindow(win));
#endif
#ifdef BRISK_LINUX
        handle.window  = glfwGetX11Window(win);
        handle.display = glfwGetX11Display();
#endif
    }

    OSWindowGLFW() = default;

    explicit OSWindowGLFW(GLFWwindow* win) : win(win) {}

private:
    GLFWwindow* win = nullptr;
};

static void errorfun(int error_code, const char* description) {
    INFO(description);
    CHECK(false);
}

TEST_CASE("Renderer: window", "[gpu]") {

    constexpr int numWindows = 1;

    struct OneWindow {
        GLFWwindow* win;
        OSWindowGLFW osWin;
        RC<WindowRenderTarget> target;
        RC<RenderEncoder> encoder;
        double previousFrameTime = -1;
        double waitTime;
        double frameInterval;
    };

    std::array<OneWindow, numWindows> windows;

    auto ttf = readBytes(fs::path(PROJECT_SOURCE_DIR) / "resources" / "fonts" / "Lato-Medium.ttf");
    REQUIRE(ttf.has_value());
    fonts->addFont(FontFamily::Default, FontStyle::Normal, FontWeight::Regular, *ttf);

    expected<RC<RenderDevice>, RenderDeviceError> device = getRenderDevice();
    REQUIRE(device.has_value());

    glfwSetErrorCallback(&errorfun);

    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    for (int i = 0; i < numWindows; ++i) {
        windows[i].win = glfwCreateWindow(500, 500, "test", nullptr, nullptr);
        REQUIRE(windows[i].win != nullptr);
        windows[i].osWin = OSWindowGLFW(windows[i].win);
    }
    SCOPE_EXIT {
        for (int i = 0; i < numWindows; ++i)
            glfwDestroyWindow(windows[i].win);
        glfwTerminate();
    };
    for (int i = 0; i < numWindows; ++i) {
        windows[i].target = (*device)->createWindowTarget(&windows[i].osWin, PixelType::U8);
        windows[i].target->setVSyncInterval(1);
    }

    RC<RenderEncoder> encoder = (*device)->createEncoder();
    double sumWaitTimeR       = -1;

    while (!glfwWindowShouldClose(windows[0].win)) {
        glfwPollEvents();
        double sumWaitTime = 0;
        for (int i = 0; i < numWindows; ++i) {
            sumWaitTime += windows[i].frameInterval;
        }
        sumWaitTime /= numWindows;
        if (sumWaitTimeR >= 0)
            sumWaitTime = mix(0.9, sumWaitTime, sumWaitTimeR);
        sumWaitTimeR = sumWaitTime;
        for (int i = 0; i < numWindows; ++i) {
            Size winSize;
            glfwGetFramebufferSize(windows[i].win, &winSize.width, &winSize.height);
            Rectangle bounds = Rectangle{ Point{ 0, 0 }, winSize };
            Rectangle inner  = bounds.withPadding(40);
            {
                RenderPipeline pipeline(encoder, windows[i].target, 0x222426_rgb);
                static int frame = 0;
                ++frame;
                RawCanvas canvas(pipeline);
                canvas.drawRectangle(inner, inner.shortestSide() * 0.5f, frame * 0.02f,
                                     linearGradient = { inner.at(0.f, 0.f), inner.at(1.f, 1.f) },
                                     fillColors     = { Palette::Standard::green, Palette::Standard::red },
                                     strokeColor = Palette::black, strokeWidth = 16.f);
                canvas.drawText(inner, 0.5f, 0.5f,
                                fmt::format(R"({}x{}
wait = {:.1f}ms
total = {:.1f}ms 
rate = {:.1f}fps)",
                                            winSize.x, winSize.y, 1000 * windows[i].waitTime,
                                            1000 * sumWaitTime, 1.0 / sumWaitTime),
                                Font{ FontFamily::Default, 40.f }, Palette::white);
                canvas.drawRectangle(Rectangle{ Point{ frame % winSize.x, 0 }, Size{ 5, winSize.y } }, 0.f,
                                     0.f, strokeWidth = 0, fillColor = Palette::black);
            }
        }
        for (int i = 0; i < numWindows; ++i) {
            double beforeFrameTime = currentTime();
            windows[i].target->present();
            double frameTime             = currentTime();
            double previousFrameTime     = windows[i].previousFrameTime;
            windows[i].previousFrameTime = frameTime;
            windows[i].frameInterval     = frameTime - previousFrameTime;
            windows[i].waitTime          = frameTime - beforeFrameTime;
        }
    }
}
#endif

TEST_CASE("Atlas overflow", "[gpu]") {
    constexpr Size size{ 2048, 2048 };
    renderTest("overflow-lines", size, [&](RenderContext& context) {
        Canvas canvas(context);
        canvas.setFillColor(Palette::white);
        canvas.fillRect(RectangleF(PointF{}, size));
        for (int i = 0; i < 200; ++i) {
            Path path;
            canvas.setFillColor(Palette::Standard::index(i));
            path.addRect(RectangleF(0.f, 2 * i, size.width, 2 * i + 1));
            path.addRect(RectangleF(2 * i, 0.f, 2 * i + 1, size.height));
            canvas.fillPath(path);
        }
        CHECK(context.numBatches() > 1);
    });
}

template <typename Fn>
static void blendingTest(std::string s, Size size, Fn&& fn) {
    linearColor = false;
    renderTest(s + "_sRGB", size, fn);
    linearColor = true;
    renderTest(s + "_Linear", size, fn);
    linearColor = false;
}

TEST_CASE("Blending", "[gpu]") {
    constexpr Size canvasSize{ 1000, 1000 };
    constexpr int rowHeight = 100;
    blendingTest("blending1", canvasSize, [&](RenderContext& context) {
        RawCanvas canvas(context);
        auto bands = [&canvas](int index, int count, Color background, Color foreground) {
            canvas.drawRectangle(RectangleF(Point(0, index * rowHeight), Size(canvasSize.width, rowHeight)),
                                 0.f, 0.f, fillColor = background, strokeWidth = 0);
            for (int i = 0; i <= count; ++i) {
                canvas.drawRectangle(
                    RectangleF(i * canvasSize.width / (count + 1), index * rowHeight,
                               (i + 1) * canvasSize.width / (count + 1), (index + 1) * rowHeight),
                    0.f, 0.f, fillColor = foreground.multiplyAlpha(static_cast<float>(i) / count),
                    strokeWidth = 0);
            }
        };
        auto gradient = [&canvas](int index, Color background, Color start, Color end) {
            canvas.drawRectangle(RectangleF(Point(0, index * rowHeight), Size(canvasSize.width, rowHeight)),
                                 0.f, 0.f, fillColor = background, strokeWidth = 0);
            canvas.drawRectangle(RectangleF(Point(0, index * rowHeight), Size(canvasSize.width, rowHeight)),
                                 0.f, 0.f, linearGradient = { Point{ 0, 0 }, Point{ canvasSize.width, 0 } },
                                 fillColors = { start, end }, strokeWidth = 0);
        };
        bands(0, 10, Palette::black, Palette::white);
        bands(1, 50, Palette::black, Palette::white);
        gradient(2, Palette::black, Palette::transparent, Palette::white);
        gradient(3, Palette::black, Palette::black, Palette::white);
        bands(4, 10, Palette::red, Palette::green);
        bands(5, 50, Palette::red, Palette::green);
        gradient(6, Palette::red, Palette::transparent, Palette::green);
        gradient(7, Palette::red, Palette::red, Palette::green);
    });
}

TEST_CASE("Gradients", "[gpu]") {
    constexpr Size canvasSize{ 1000, 100 };
    blendingTest("gradients1", canvasSize, [&](RenderContext& context) {
        Canvas canvas(context);
        auto grad = rcnew Gradient{ GradientType::Linear, PointF{ 0, 0 }, PointF{ 1000, 0 } };
        grad->addStop(0.000f, Palette::black);
        grad->addStop(0.333f, Palette::white);
        grad->addStop(0.667f, Palette::black);
        grad->addStop(1.000f, Palette::white);
        canvas.setFillPaint(grad);
        canvas.fillRect(RectangleF{ 0, 0, 1000.f, 50.f });
        grad = rcnew Gradient{ GradientType::Linear, PointF{ 0, 0 }, PointF{ 1000, 0 } };
        grad->addStop(0.000f, Palette::red);
        grad->addStop(0.333f, Palette::green);
        grad->addStop(0.667f, Palette::red);
        grad->addStop(1.000f, Palette::green);
        canvas.setFillPaint(grad);
        canvas.fillRect(RectangleF{ 0, 50.f, 1000.f, 100.f });
    });
}

TEST_CASE("TextureFill", "[gpu]") {
    constexpr Size canvasSize{ 400, 400 };
    blendingTest("texturefill", canvasSize, [&](RenderContext& context) {
        RC<ImageRGBA> checkerboard = rcnew ImageRGBA({ 20, 20 });
        {
            auto wr = checkerboard->mapWrite();
            wr.forPixels([](int32_t x, int32_t y, PixelRGBA8& pix) {
                Color c = x < 10 != y < 10 ? 0x592d07_rgb : 0xf0bf7f_rgb;
                pix     = colorToPixel<PixelType::U8Gamma, PixelFormat::RGBA>(c);
            });
        }

        Canvas canvas(context);
        canvas.setFillPaint(Texture{ checkerboard });
        canvas.fillRect(RectangleF{ 0, 0, 400, 200 });
        canvas.setFillPaint(Texture{ checkerboard, Matrix2D::rotation(45.f) });
        canvas.fillRect(RectangleF{ 0, 200, 400, 400 });
    });
}

TEST_CASE("Canvas::drawImage", "[gpu]") {
    renderTest("rotate-texture", Size{ 300, 300 }, [](RenderContext& context) {
        Canvas canvas(context);
        auto bytes = readBytes(fs::path(PROJECT_SOURCE_DIR) / "src/graphics/testdata/16616460-rgba.png");
        REQUIRE(bytes.has_value());
        auto image = pngDecode(*bytes, PixelFormat::RGBA);
        REQUIRE(image.has_value());
        canvas.drawImage({ 100, 100, 200, 200 }, *image, Matrix2D{}.rotate(15, 150.f, 150.f));
    });
    renderTest("rotate-texture-rect", Size{ 300, 300 }, [](RenderContext& context) {
        Canvas canvas(context);
        auto bytes = readBytes(fs::path(PROJECT_SOURCE_DIR) / "src/graphics/testdata/16616460-rgba.png");
        REQUIRE(bytes.has_value());
        auto image = pngDecode(*bytes, PixelFormat::RGBA);
        REQUIRE(image.has_value());
        canvas.setTransform(Matrix2D{}.rotate(15, 150.f, 150.f));
        canvas.drawImage({ 100, 100, 200, 200 }, *image);
    });
    renderTest("rotate-rect", Size{ 300, 300 }, [](RenderContext& context) {
        Canvas canvas(context);
        auto bytes = readBytes(fs::path(PROJECT_SOURCE_DIR) / "src/graphics/testdata/16616460-rgba.png");
        REQUIRE(bytes.has_value());
        auto image = pngDecode(*bytes, PixelFormat::RGBA);
        REQUIRE(image.has_value());
        canvas.setTransform(Matrix2D{}.rotate(15, 150.f, 150.f));
        canvas.setFillColor(Palette::Standard::green);
        canvas.fillRect({ 100, 100, 200, 200 });
    });
}

} // namespace Brisk
