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

#include <brisk/core/Utilities.hpp>
#include <brisk/core/Reflection.hpp>
#include <catch2/catch_all.hpp>
#include "Catch2Utils.hpp"
#include "VisualTests.hpp"
#include <brisk/core/Time.hpp>
#include <brisk/core/Text.hpp>
#include <brisk/graphics/Image.hpp>
#include <brisk/graphics/Canvas.hpp>

#include <brisk/graphics/NativeWindowHandle.hpp>
#include <brisk/graphics/Palette.hpp>
#ifdef BRISK_WEBGPU
#include <brisk/graphics/WebGpu.hpp>
#endif

namespace Brisk {

TEST_CASE("Renderer Info", "[gpu]") {
    expected<Rc<RenderDevice>, RenderDeviceError> device_ = getRenderDevice();
    REQUIRE(device_.has_value());
    Rc<RenderDevice> device = *device_;
    RenderDeviceInfo info   = device->info();
#ifdef BRISK_DEBUG_GPU
    fmt::print("#########################################################\n");
    fmt::print("{}\n", info);
    fmt::print("#########################################################\n");
#endif
    freeRenderDevice();
}

TEST_CASE("Renderer devices", "[gpu]") {
    expected<Rc<RenderDevice>, RenderDeviceError> d;
#ifdef BRISK_D3D11
    d = createRenderDevice(RendererBackend::D3d11, RendererDeviceSelection::HighPerformance);
    REQUIRE(d.has_value());
    fmt::print("[D3d11] HighPerformance: {}\n", (*d)->info().device);
    d = createRenderDevice(RendererBackend::D3d11, RendererDeviceSelection::LowPower);
    REQUIRE(d.has_value());
    fmt::print("[D3d11] LowPower: {}\n", (*d)->info().device);
    d = createRenderDevice(RendererBackend::D3d11, RendererDeviceSelection::Default);
    REQUIRE(d.has_value());
    fmt::print("[D3d11] Default: {}\n", (*d)->info().device);
#endif

#ifdef BRISK_WEBGPU
    d = createRenderDevice(RendererBackend::WebGpu, RendererDeviceSelection::HighPerformance);
    REQUIRE(d.has_value());
    fmt::print("[WebGpu] HighPerformance: {}\n", (*d)->info().device);
    d = createRenderDevice(RendererBackend::WebGpu, RendererDeviceSelection::LowPower);
    REQUIRE(d.has_value());
    fmt::print("[WebGpu] LowPower: {}\n", (*d)->info().device);
    d = createRenderDevice(RendererBackend::WebGpu, RendererDeviceSelection::Default);
    REQUIRE(d.has_value());
    fmt::print("[WebGpu] Default: {}\n", (*d)->info().device);
#endif
}

TEST_CASE("Renderer - fonts") {
    auto ttf = readBytes(fs::path(PROJECT_SOURCE_DIR) / "resources" / "fonts" / "Lato-Medium.ttf");
    REQUIRE(ttf.has_value());
    fonts->addFont("Lato", FontStyle::Normal, FontWeight::Regular, *ttf, true, FontFlags::Default);
    auto ttf2 = readBytes(fs::path(PROJECT_SOURCE_DIR) / "resources" / "fonts" / "Lato-Heavy.ttf");
    REQUIRE(ttf2.has_value());
    fonts->addFont("Lato", FontStyle::Normal, FontWeight::Bold, *ttf2, true, FontFlags::Default);

    for (bool subpixel : { false, true }) {
        renderTest(
            "rr-fonts{}"_fmt(subpixel ? "-subpixel" : ""), { 1200, 600 },
            [&](RenderContext& context) {
                Canvas canvas(context, CanvasFlags::Sdf);
                canvas.setSubpixelTextRendering(subpixel);

                Rectangle rect;
                ColorF c;
                canvas.setFont(Font{ "Lato", 27.f });
                for (int i = 0; i < 10; ++i) {
                    c    = ColorOf<float, ColorGamma::sRGB>(i / 9.f);
                    rect = Rectangle{ 0, i * 60, 600, (i + 1) * 60 };
                    canvas.setFillColor(c);
                    canvas.fillRect(rect);
                    canvas.setFillColor(Palette::white);
                    canvas.fillText("The quick brown fox jumps over the lazy dog", rect, PointF(0.5f, 0.5f));
                    c    = ColorOf<float, ColorGamma::sRGB>(1.f - i / 9.f);
                    rect = Rectangle{ 600, i * 60, 1200, (i + 1) * 60 };
                    canvas.setFillColor(c);
                    canvas.fillRect(rect);
                    canvas.setFillColor(Palette::black);
                    canvas.fillText("The quick brown fox jumps over the lazy dog", rect, PointF(0.5f, 0.5f));
                }
                CHECK(canvas.rasterizedPaths() == 0);
            },
            ColorF{ 1.f, 1.f });
    }
}

TEST_CASE("Html text") {
    renderTest("html-text", Size{ 300, 150 }, [](RenderContext& context) {
        Canvas canvas(context);
        canvas.setFillColor(Palette::white);
        canvas.fillRect({ 0, 0, 300, 150 });
        canvas.setFillColor(Palette::black);
        canvas.setFont(Font{ "Lato", 25.f });
        canvas.fillText(
            TextWithOptions("The <b>quick</b> <font color=\"brown\">brown</font> <u>fox<br/>jumps</u> over "
                            "the <small>lazy</small> dog",
                            TextOptions::Html),
            Rectangle{ 30, 30, 270, 120 });
    });
}

TEST_CASE("Renderer", "[gpu]") {
    const Rectangle frameBounds = Rectangle{ 0, 0, 480, 320 };
    RectangleF rect             = frameBounds.withPadding(10);
    float radius                = frameBounds.shortestSide() * 0.2f;
    float strokeWidth           = frameBounds.shortestSide() * 0.05f;

    SECTION("Canvas-SDF") {
        renderTest(
            "rr-ll", frameBounds.size(),
            [&](RenderContext& context) {
                Canvas canvas(context, CanvasFlags::Sdf);
                canvas.setFillPaint(LinearGradient(frameBounds.at(0.1f, 0.1f), frameBounds.at(0.9f, 0.9f),
                                                   Palette::Standard::green, Palette::Standard::red));
                canvas.setStrokeColor(Palette::black);
                canvas.setStrokeWidth(strokeWidth);
                canvas.drawRect(rect, radius);
                CHECK(canvas.rasterizedPaths() == 0);
            },
            ColorF{ 0.5f, 0.5f, 0.5f, 1.f });
    }

    SECTION("Canvas") {
        renderTest(
            "rr", frameBounds.size(),
            [&](RenderContext& context) {
                Canvas canvas(context, CanvasFlags::None);
                Path path;
                path.addRoundRect(rect, radius);
                canvas.setStrokeWidth(strokeWidth);
                canvas.setStrokeColor(Palette::black);
                Gradient grad(GradientType::Linear, frameBounds.at(0.1f, 0.1f), frameBounds.at(0.9f, 0.9f));
                grad.addStop(0.f, Palette::Standard::green);
                grad.addStop(1.f, Palette::Standard::red);
                canvas.setFillPaint(grad);
                canvas.fillPath(path);
                canvas.strokePath(path);
            },
            ColorF{ 0.5f, 0.5f, 0.5f, 1.f });
    }
}

TEST_CASE("Atlas overflow", "[gpu]") {
    constexpr Size size{ 2048, 2048 };
    renderTest("overflow-lines", size, [&](RenderContext& context) {
        Canvas canvas(context, CanvasFlags::None);
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
    SECTION("sRGB") {
        renderTest(s + "_sRGB", size, fn, Palette::transparent, 0.06f);
    }
    linearColor = true;
    SECTION("Linear") {
        renderTest(s + "_Linear", size, fn, Palette::transparent, 0.06f);
    }
    linearColor = false;
}

TEST_CASE("Blending", "[gpu]") {
    constexpr Size canvasSize{ 1200, 1200 };
    constexpr int rowHeight = 100;
    blendingTest("blending1", canvasSize, [&](RenderContext& context) {
        Canvas canvas(context);
        auto bands = [&canvas BRISK_IF_GCC(, canvasSize)](int index, int count, Color background,
                                                          Color foreground) {
            canvas.setFillColor(background);
            canvas.fillRect(RectangleF(Point(0, index * rowHeight), Size(canvasSize.width, rowHeight)));
            for (int i = 0; i <= count; ++i) {
                canvas.setFillColor(foreground.multiplyAlpha(static_cast<float>(i) / count));
                canvas.fillRect(RectangleF(i * canvasSize.width / (count + 1), index * rowHeight,
                                           (i + 1) * canvasSize.width / (count + 1),
                                           (index + 1) * rowHeight));
            }
        };
        auto gradient = [&canvas BRISK_IF_GCC(, canvasSize)](int index, Color background, Color start,
                                                             Color end) {
            canvas.setFillColor(background);
            canvas.fillRect(RectangleF(Point(0, index * rowHeight), Size(canvasSize.width, rowHeight)));
            canvas.setFillPaint(LinearGradient(Point{ 0, 0 }, Point{ canvasSize.width, 0 }, start, end));
            canvas.fillRect(RectangleF(Point(0, index * rowHeight), Size(canvasSize.width, rowHeight)));
        };
        bands(0, 10, Palette::black, Palette::white);
        bands(1, 50, Palette::black, Palette::white);
        gradient(2, Palette::black, Palette::transparent, Palette::white);
        gradient(3, Palette::black, Palette::black, Palette::white);
        bands(4, 10, Palette::red, Palette::green);
        bands(5, 50, Palette::red, Palette::green);
        gradient(6, Palette::red, Palette::transparent, Palette::green);
        gradient(7, Palette::red, Palette::red, Palette::green);
        bands(8, 10, Palette::cyan, Palette::red);
        bands(9, 50, Palette::cyan, Palette::red);
        gradient(10, Palette::cyan, Palette::transparent, Palette::red);
        gradient(11, Palette::cyan, Palette::cyan, Palette::red);
        CHECK(canvas.rasterizedPaths() == 0);
    });
}

TEST_CASE("Gradients", "[gpu]") {
    constexpr Size canvasSize{ 1000, 100 };
    blendingTest("gradients1", canvasSize, [&](RenderContext& context) {
        Canvas canvas(context);
        Gradient grad{ GradientType::Linear, PointF{ 0, 0 }, PointF{ 1000, 0 } };
        grad.addStop(0.000f, Palette::black);
        grad.addStop(0.333f, Palette::white);
        grad.addStop(0.667f, Palette::black);
        grad.addStop(1.000f, Palette::white);
        canvas.setFillPaint(std::move(grad));
        canvas.fillRect(RectangleF{ 0, 0, 1000.f, 50.f });
        grad = Gradient{ GradientType::Linear, PointF{ 0, 0 }, PointF{ 1000, 0 } };
        grad.addStop(0.000f, Palette::red);
        grad.addStop(0.333f, Palette::green);
        grad.addStop(0.667f, Palette::red);
        grad.addStop(1.000f, Palette::green);
        canvas.setFillPaint(std::move(grad));
        canvas.fillRect(RectangleF{ 0, 50.f, 1000.f, 100.f });
    });
}

TEST_CASE("TextureFill", "[gpu]") {
    constexpr Size canvasSize{ 400, 400 };
    blendingTest("texturefill", canvasSize, [&](RenderContext& context) {
        Rc<Image> checkerboard = rcnew Image({ 20, 20 }, ImageFormat::RGBA);
        {
            auto wr = checkerboard->mapWrite<ImageFormat::RGBA>();
            wr.forPixels([](int32_t x, int32_t y, PixelRGBA8& pix) {
                Color c = x < 10 != y < 10 ? 0x592d07_rgb : 0xf0bf7f_rgb;
                colorToPixel(pix, c);
            });
        }

        Canvas canvas(context);
        canvas.setFillPaint(Texture{ checkerboard });
        canvas.fillRect(RectangleF{ 0, 0, 400, 200 });
        canvas.setFillPaint(Texture{ checkerboard, Matrix::rotation(45.f) });
        canvas.fillRect(RectangleF{ 0, 200, 400, 400 });
    });
}

TEST_CASE("Canvas::drawImage", "[gpu]") {
    renderTest("rotate-texture", Size{ 300, 300 }, [](RenderContext& context) {
        Canvas canvas(context);
        auto bytes = readBytes(fs::path(PROJECT_SOURCE_DIR) / "src/testdata/16616460-rgba.png");
        REQUIRE(bytes.has_value());
        auto image = pngDecode(*bytes, ImageFormat::RGBA, true);
        REQUIRE(image.has_value());
        canvas.drawImage({ 100, 100, 200, 200 }, *image, Matrix{}.rotate(15, 50.f, 50.f));
    });
    renderTest("rotate-texture-rect", Size{ 300, 300 }, [](RenderContext& context) {
        Canvas canvas(context);
        auto bytes = readBytes(fs::path(PROJECT_SOURCE_DIR) / "src/testdata/16616460-rgba.png");
        REQUIRE(bytes.has_value());
        auto image = pngDecode(*bytes, ImageFormat::RGBA, true);
        REQUIRE(image.has_value());
        canvas.setTransform(Matrix{}.rotate(15, 150.f, 150.f));
        canvas.drawImage({ 100, 100, 200, 200 }, *image);
    });
    renderTest("rotate-rect", Size{ 300, 300 }, [](RenderContext& context) {
        Canvas canvas(context);
        auto bytes = readBytes(fs::path(PROJECT_SOURCE_DIR) / "src/testdata/16616460-rgba.png");
        REQUIRE(bytes.has_value());
        auto image = pngDecode(*bytes, ImageFormat::RGBA, true);
        REQUIRE(image.has_value());
        canvas.setTransform(Matrix{}.rotate(15, 150.f, 150.f));
        canvas.setFillColor(Palette::Standard::green);
        canvas.fillRect({ 100, 100, 200, 200 });
    });
}

TEST_CASE("Emoji") {
    auto ttf = readBytes(fs::path(PROJECT_SOURCE_DIR) / "resources" / "fonts" / "NotoColorEmoji-SVG.otf");
    REQUIRE(ttf.has_value());
    fonts->addFont("Noto Emoji", FontStyle::Normal, FontWeight::Regular, *ttf, true, FontFlags::EnableColor);
    auto ttf2 = readBytes(fs::path(PROJECT_SOURCE_DIR) / "resources" / "fonts" / "Lato-Medium.ttf");
    REQUIRE(ttf2.has_value());
    fonts->addFont("Lato", FontStyle::Normal, FontWeight::Regular, *ttf2, true, FontFlags::Default);

    const Size size{ 1200, 200 };
    renderTest("emoji-only", size, [&](RenderContext& context) {
        Canvas canvas(context);
        Rectangle rect({}, size);
        canvas.setFont({ "Noto Emoji", 60.f });
        canvas.setFillColor(Palette::black);
        canvas.fillText("🐢👑🌟🧿📸🚨🏡🕊️🏆😻✌️🍀🎨🌴🍜", rect, PointF(0.5f, 0.5f));
    });

    renderTest(
        "emoji-text", size,
        [&](RenderContext& context) {
            Canvas canvas(context);
            Rectangle rect({}, size);
            canvas.setFont({ "Lato,Noto Emoji", 72.f });
            canvas.setFillColor(Palette::black);
            canvas.fillText("Crown: 👑, Star: 🌟 Camera: 📸", rect, PointF(0.5f, 0.5f));
        },
        ColorF(0.5f));
}

TEST_CASE("SetClipRect") {
    renderTest("SetClipRect0", Size{ 256, 256 }, [&](RenderContext& context) {
        Canvas canvas(context);
        Rectangle rect({}, Size{ 256, 256 });
        canvas.setFillPaint(LinearGradient({ 0, 0 }, { 256, 256 }, Palette::cyan, Palette::magenta));
        canvas.fillRect(rect);
    });
    renderTest("SetClipRect1", Size{ 256, 256 }, [&](RenderContext& context) {
        Canvas canvas(context);
        Rectangle rect({}, Size{ 256, 256 });
        context.setClipRect(Rectangle{ 10, 20, 100, 200 });
        canvas.setFillPaint(LinearGradient({ 0, 0 }, { 256, 256 }, Palette::cyan, Palette::magenta));
        canvas.fillRect(rect);
    });
}

TEST_CASE("Multi-pass render") {
    renderTest<true, true>(
        "MultiPass1", Size{ 256, 256 }, [&](Rc<RenderEncoder> encoder, Rc<ImageRenderTarget> target) {
            {
                RenderPipeline pipeline(encoder, target, Palette::transparent);
                Canvas canvas(pipeline);
                Rectangle rect({}, Size{ 256, 256 });
                canvas.setFillPaint(LinearGradient({ 0, 0 }, { 0, 256 }, Palette::red, Palette::transparent));
                canvas.fillRect(rect);
            }
            {
                RenderPipeline pipeline(encoder, target, std::nullopt);
                Canvas canvas(pipeline);
                Rectangle rect({}, Size{ 256, 256 });
                canvas.setFillPaint(
                    LinearGradient({ 0, 0 }, { 256, 0 }, Palette::blue, Palette::transparent));
                canvas.fillRect(rect);
            }
        });
}

TEST_CASE("Shadow") {
    renderTest(
        "shadows", Size{ 1536, 256 },
        [&](RenderContext& context) {
            Canvas canvas(context);
            for (int i = 0; i < 6; ++i) {
                RectangleF box{ 256.f * i, 0, 256.f * i + 256, 256 };
                context.setClipRect(box);
                float shadowSize = 2 << i;
                canvas.setFillColor(Palette::black);
                canvas.blurRect(box.withPadding(64.f), shadowSize);
            }
        },
        Palette::white);

    renderTest(
        "shadows-rounded", Size{ 1536, 256 },
        [&](RenderContext& context) {
            Canvas canvas(context);
            for (int i = 0; i < 6; ++i) {
                RectangleF box{ 256.f * i, 0, 256.f * i + 256, 256 };
                context.setClipRect(box);
                float boxRadius = 2 << i;
                canvas.setFillColor(Palette::black);
                canvas.blurRect(box.withPadding(64.f), 16.f, boxRadius);
            }
        },
        Palette::white);
    renderTest(
        "shadows-rounded2", Size{ 1536, 256 },
        [&](RenderContext& context) {
            Canvas canvas(context);
            for (int i = 0; i < 6; ++i) {
                RectangleF box{ 256.f * i, 0, 256.f * i + 256, 256 };
                context.setClipRect(box);
                float shadowSize = 1 << i;
                canvas.setFillColor(Palette::black);
                canvas.blurRect(box.withPadding(64.f), shadowSize, { 0.f, 32.f, 8.f, 0.f });
            }
        },
        Palette::white);
}

TEST_CASE("Canvas opacity") {
    renderTest("canvas-opacity", Size{ 256, 192 }, [&](RenderContext& context) {
        Canvas canvas(context);
        canvas.setOpacity(0.5f);
        canvas.setFillColor(Palette::black);
        canvas.fillRect({ 0, 0, 256, 64 });
        Gradient gradient(GradientType::Linear);
        gradient.setStartPoint({ 0, 0 });
        gradient.setEndPoint({ 256, 0 });
        gradient.addStop(0.f, Palette::green);
        gradient.addStop(1.f, Palette::red);
        canvas.setFillPaint(gradient);
        canvas.fillRect({ 0, 64, 256, 128 });
        Rc<Image> image = rcnew Image(Size{ 4, 4 });
        {
            auto wr = image->mapWrite();
            wr.clear(Palette::blue);
        }
        canvas.setFillPaint(Texture{ std::move(image), {}, SamplerMode::Clamp });
        canvas.fillRect({ 0, 128, 256, 192 });
    });
}

enum class TestMode {
    Fill   = 1,
    Stroke = 2,
    Draw   = 3,
};
template <>
inline constexpr std::initializer_list<NameValuePair<TestMode>> defaultNames<TestMode>{
    { "fill", TestMode::Fill },
    { "stroke", TestMode::Stroke },
    { "draw", TestMode::Draw },
};

template <>
inline constexpr std::initializer_list<NameValuePair<CapStyle>> defaultNames<CapStyle>{
    { "flat", CapStyle::Flat },
    { "square", CapStyle::Square },
    { "round", CapStyle::Round },
};

template <>
inline constexpr std::initializer_list<NameValuePair<JoinStyle>> defaultNames<JoinStyle>{
    { "bevel", JoinStyle::Bevel },
    { "miter", JoinStyle::Miter },
    { "round", JoinStyle::Round },
};

template <>
inline constexpr std::initializer_list<NameValuePair<GradientType>> defaultNames<GradientType>{
    { "linear", GradientType::Linear },
    { "radial", GradientType::Radial },
    { "angle", GradientType::Angle },
    { "reflected", GradientType::Reflected },
};
using enum TestMode;

static void drawRect(Canvas& canvas, TestMode mode, RectangleF r) {
    switch (mode) {
    case Fill:
        canvas.fillRect(r);
        break;
    case Stroke:
        canvas.strokeRect(r);
        break;
    case Draw:
        canvas.drawRect(r);
        break;
    }
}

static void drawPath(Canvas& canvas, TestMode mode, const Path& p) {
    switch (mode) {
    case Fill:
        canvas.fillPath(p);
        break;
    case Stroke:
        canvas.strokePath(p);
        break;
    case Draw:
        canvas.drawPath(p);
        break;
    }
}

TEST_CASE("Canvas optimization") {
    constexpr CanvasFlags flags = CanvasFlags::Sdf;
    for (TestMode mode : { Fill, Stroke, Draw }) {
        renderTest(
            "canvas-sdf1-{}"_fmt(mode), Size{ 100, 100 },
            [&](RenderContext& context) {
                Canvas canvas(context, flags);
                canvas.setJoinStyle(JoinStyle::Round);
                canvas.setFillColor(Palette::Standard::cyan);
                drawRect(canvas, mode, { 20, 20, 80, 80 });
                CHECK(canvas.rasterizedPaths() == 0);
            },
            defaultBackColor, 0.075f);
        renderTest(
            "canvas-sdf2-{}"_fmt(mode), Size{ 100, 100 },
            [&](RenderContext& context) {
                Canvas canvas(context, flags);
                canvas.setJoinStyle(JoinStyle::Round);
                canvas.setFillColor(Palette::Standard::cyan);
                canvas.setTransform(Matrix::translation(+4.5f, -3.f));
                drawRect(canvas, mode, { 20, 20, 80, 80 });
                CHECK(canvas.rasterizedPaths() == 0);
            },
            defaultBackColor, 0.075f);
        renderTest(
            "canvas-sdf3-{}"_fmt(mode), Size{ 100, 100 },
            [&](RenderContext& context) {
                Canvas canvas(context, flags);
                canvas.setJoinStyle(JoinStyle::Round);
                canvas.setFillPaint(
                    LinearGradient({ 20, 20 }, { 80, 80 }, Palette::Standard::cyan, Palette::Standard::pink));
                canvas.setTransform(Matrix().scale(0.75f, 0.75f, 50.f, 50.f));
                drawRect(canvas, mode, { 20, 20, 80, 80 });
                CHECK(canvas.rasterizedPaths() == 0);
            },
            defaultBackColor, 0.075f);
        renderTest(
            "canvas-sdf4-{}"_fmt(mode), Size{ 100, 100 },
            [&](RenderContext& context) {
                Canvas canvas(context, flags);
                canvas.setJoinStyle(JoinStyle::Round);
                canvas.setFillPaint(
                    RadialGradient({ 20, 20 }, 84.85, Palette::Standard::cyan, Palette::Standard::pink));
                canvas.setTransform(Matrix().rotate(60.f, 50.f, 50.f));
                drawRect(canvas, mode, { 20, 20, 80, 80 });
                CHECK(canvas.rasterizedPaths() == 0);
            },
            defaultBackColor, 0.075f);
        renderTest(
            "canvas-sdf5-{}"_fmt(mode), Size{ 100, 100 },
            [&](RenderContext& context) {
                Canvas canvas(context, flags);
                canvas.setJoinStyle(JoinStyle::Round);
                canvas.setStrokeWidth(0.15f);
                canvas.setFillPaint(
                    RadialGradient({ 20, 20 }, 84.85, Palette::Standard::cyan, Palette::Standard::pink));
                canvas.setTransform(Matrix::scaling(100.f));
                drawRect(canvas, mode, { 0.2f, 0.2f, 0.8f, 0.8f });
                CHECK(canvas.rasterizedPaths() == 0);
            },
            defaultBackColor, 0.075f);
        renderTest(
            "canvas-sdf6-{}"_fmt(mode), Size{ 100, 100 },
            [&](RenderContext& context) {
                Canvas canvas(context, flags);
                canvas.setJoinStyle(JoinStyle::Round);
                canvas.setFillPaint(
                    RadialGradient({ 20, 20 }, 84.85, Palette::Standard::cyan, Palette::Standard::pink));
                canvas.setTransform(Matrix::scaling(10.f));
                drawRect(canvas, mode, { 2, 2, 8, 8 });
                CHECK(canvas.rasterizedPaths() == 0);
            },
            defaultBackColor, 0.075f);
        renderTest(
            "canvas-sdf7-{}"_fmt(mode), Size{ 100, 100 },
            [&](RenderContext& context) {
                Canvas canvas(context, flags);
                canvas.setJoinStyle(JoinStyle::Miter);
                canvas.setStrokeWidth(15.f);
                canvas.setFillPaint(
                    RadialGradient({ 20, 20 }, 84.85, Palette::Standard::cyan, Palette::Standard::pink));
                drawRect(canvas, mode, { 20, 20, 80, 80 });
                CHECK(canvas.rasterizedPaths() == 0);
            },
            defaultBackColor, 0.075f);
        renderTest(
            "canvas-sdf8-{}"_fmt(mode), Size{ 100, 100 },
            [&](RenderContext& context) {
                Canvas canvas(context, flags);
                canvas.setJoinStyle(JoinStyle::Round);
                canvas.setFillColor(Palette::Standard::orange);
                Path path;
                path.addRoundRect({ 20, 20, 80, 80 }, 10.f);
                drawPath(canvas, mode, path);
                CHECK(canvas.rasterizedPaths() == 0);
            },
            defaultBackColor, 0.075f);
    }
    for (CapStyle style : { CapStyle::Flat, CapStyle::Round, CapStyle::Square }) {
        renderTest(
            "canvas-sdf8-{}"_fmt(style), Size{ 100, 100 },
            [&](RenderContext& context) {
                Canvas canvas(context, flags);
                canvas.setCapStyle(style);
                canvas.setStrokeWidth(15.f);
                canvas.setStrokeColor(Palette::Standard::amber);
                canvas.strokeLine({ 20, 20 }, { 80, 80 });
                CHECK(canvas.rasterizedPaths() == 0);
            },
            defaultBackColor, 0.075f);
    }
}

TEST_CASE("Canvas scissors") {
    renderTest("canvas-scissors1", Size{ 256, 256 }, [](RenderContext& context) {
        Canvas canvas(context);
        canvas.setStrokeColor(Palette::black);
        canvas.setStrokeWidth(2.f);
        canvas.setFillColor(Palette::Standard::lime);
        canvas.setClipRect(RectangleF{ 0, 0, 128, 128 });
        canvas.drawRect(Rectangle{ 10, 10, 246, 246 });
    });
}

TEST_CASE("Canvas transform") {
    renderTest("canvas-transform", Size{ 128, 64 }, [](RenderContext& context) {
        Canvas canvas(context, CanvasFlags::Sdf);
        canvas.setFillColor(Palette::Standard::green);
        canvas.setStrokeColor(Palette::Standard::pink);
        canvas.setStrokeWidth(4.f);
        canvas.drawRect(Rectangle{ 10, 10, 54, 54 });
        canvas.transform(Matrix().rotate(10.f, { 32.f, 32.f }));
        canvas.transform(Matrix::translation(64.f, 0));
        canvas.setFillColor(Palette::Standard::pink);
        canvas.setStrokeColor(Palette::Standard::green);
        canvas.drawRect(Rectangle{ 10, 10, 54, 54 });
    });
    renderTest("canvas-transform2", Size{ 10, 30 }, [](RenderContext& context) {
        Canvas canvas(context, CanvasFlags::Sdf);
        canvas.setStrokeColor(Palette::black);
        canvas.setStrokeWidth(1.f);
        canvas.strokeLine({ 1.f, 1.f }, { 9.f, 9.f });
        canvas.transform(Matrix::translation(0, 10));
        canvas.strokeLine({ 1.f, 1.f }, { 9.f, 9.f });
        canvas.transform(Matrix::translation(0, 10));
        canvas.strokeLine({ 1.f, 1.f }, { 9.f, 9.f });
    });
}

TEST_CASE("Semitransparent fill and stroke") {
    for (CanvasFlags flags : { CanvasFlags::None, CanvasFlags::Sdf }) {
        renderTest("canvas-semitransparent-fs", Size{ 64, 64 }, [flags](RenderContext& context) {
            Canvas canvas(context, flags);
            canvas.setFillColor(Palette::white.multiplyAlpha(0.5f));
            canvas.setStrokeColor(Palette::black.multiplyAlpha(0.5f));
            canvas.setStrokeWidth(8);
            canvas.drawRect(Rectangle{ 10, 10, 54, 54 });
        });
        renderTest("canvas-semitransparent2-s", Size{ 64, 64 }, [flags](RenderContext& context) {
            Canvas canvas(context, flags);
            canvas.setFillColor(Palette::white);
            canvas.setStrokeColor(Palette::black.multiplyAlpha(0.5f));
            canvas.setStrokeWidth(8);
            canvas.drawRect(Rectangle{ 10, 10, 54, 54 });
        });
        renderTest("canvas-semitransparent-f", Size{ 64, 64 }, [flags](RenderContext& context) {
            Canvas canvas(context, flags);
            canvas.setFillColor(Palette::white.multiplyAlpha(0.5f));
            canvas.setStrokeColor(Palette::black);
            canvas.setStrokeWidth(8);
            canvas.drawRect(Rectangle{ 10, 10, 54, 54 });
        });
    }
}

TEST_CASE("Canvas blur") {
    renderTest<true>("canvas-blur0", Size{ 320, 213 }, [](RenderContext& context) {
        Canvas canvas(context);
        auto bytes = readBytes(fs::path(PROJECT_SOURCE_DIR) / "src/testdata/16616460-rgb.png");
        REQUIRE(bytes.has_value());
        auto image = pngDecode(*bytes, ImageFormat::RGBA);
        REQUIRE(image.has_value());

        canvas.drawImage({ 0, 0, 320, 213 }, *image, Matrix{}, SamplerMode::Clamp, 0.f);
        static_cast<RenderPipeline&>(canvas.renderContext()).flush();
        canvas.drawImage({ 0, 0, 320, 213 }, *image, Matrix{}, SamplerMode::Clamp, 0.f);
        static_cast<RenderPipeline&>(canvas.renderContext()).flush();
        canvas.drawImage({ 0, 0, 320, 213 }, *image, Matrix{}, SamplerMode::Clamp, 0.f);
        static_cast<RenderPipeline&>(canvas.renderContext()).flush();
        canvas.drawImage({ 0, 0, 320, 213 }, *image, Matrix{}, SamplerMode::Clamp, 0.f);
    });
    renderTest<true>("canvas-blur1", Size{ 512, 512 }, [](RenderContext& context) {
        Canvas canvas(context);
        auto bytes = readBytes(fs::path(PROJECT_SOURCE_DIR) / "src/testdata/suprematism.png");
        REQUIRE(bytes.has_value());
        auto image = pngDecode(*bytes, ImageFormat::RGBA);
        REQUIRE(image.has_value());
        canvas.drawImage({ 0, 0, 512, 512 }, *image, Matrix{}, SamplerMode::Clamp, 1.f);
    });
    renderTest<true>("canvas-blur2", Size{ 512, 512 }, [](RenderContext& context) {
        Canvas canvas(context);
        auto bytes = readBytes(fs::path(PROJECT_SOURCE_DIR) / "src/testdata/suprematism.png");
        REQUIRE(bytes.has_value());
        auto image = pngDecode(*bytes, ImageFormat::RGBA);
        REQUIRE(image.has_value());
        canvas.drawImage({ 0, 0, 512, 512 }, *image, Matrix{}, SamplerMode::Clamp, 13.f);
    });
    renderTest<true>("canvas-blur3", Size{ 320, 213 }, [](RenderContext& context) {
        Canvas canvas(context);
        auto bytes = readBytes(fs::path(PROJECT_SOURCE_DIR) / "src/testdata/16616460-rgb.png");
        REQUIRE(bytes.has_value());
        auto image = pngDecode(*bytes, ImageFormat::RGBA);
        REQUIRE(image.has_value());
        canvas.drawImage({ 0, 0, 320, 213 }, *image, Matrix{}, SamplerMode::Clamp, 7.f);
    });
}

TEST_CASE("CapStyle") {
    for (CapStyle capStyle : { CapStyle::Flat, CapStyle::Square, CapStyle::Round }) {

        renderTest("canvas-capStyle-" + fmt::to_string(capStyle), Size{ 320, 160 },
                   [capStyle](RenderContext& context) {
                       PointF pt1{ 40, 80 };
                       PointF pt2{ 280, 80 };
                       Canvas canvas(context);
                       canvas.setStrokeColor(Palette::Standard::green);
                       canvas.setCapStyle(capStyle);
                       canvas.setStrokeWidth(40.f);
                       canvas.strokeLine(pt1, pt2);
                       canvas.setFillColor(Palette::black);
                       for (PointF p : { pt1, pt2 })
                           canvas.fillEllipse(p.alignedRect({ 6.f, 6.f }, { 0.5f, 0.5f }));
                       canvas.setStrokeColor(Palette::black);
                       canvas.setStrokeWidth(2.f);
                       canvas.strokeLine(pt1, pt2);
                   });
    }
}

TEST_CASE("JoinStyle") {
    for (JoinStyle joinStyle : { JoinStyle::Miter, JoinStyle::Bevel, JoinStyle::Round }) {

        renderTest("canvas-joinStyle-" + fmt::to_string(joinStyle), Size{ 320, 160 },
                   [joinStyle](RenderContext& context) {
                       PointF pt1{ 50, 40 };
                       PointF pt2{ 50, 120 };
                       PointF pt3{ 240, 120 };
                       PointF pt4{ 120, 40 };
                       Canvas canvas(context);
                       canvas.setStrokeColor(Palette::Standard::cyan);
                       canvas.setJoinStyle(joinStyle);

                       Path path;
                       path.moveTo(pt1);
                       path.lineTo(pt2);
                       path.lineTo(pt3);
                       path.lineTo(pt4);
                       canvas.strokePath(path);

                       canvas.setStrokeWidth(40.f);
                       canvas.strokePath(path);
                       canvas.setFillColor(Palette::black);
                       for (PointF p : { pt1, pt2, pt3, pt4 })
                           canvas.fillEllipse(p.alignedRect({ 6.f, 6.f }, { 0.5f, 0.5f }));
                       canvas.setStrokeColor(Palette::black);
                       canvas.setStrokeWidth(2.f);
                       canvas.strokePath(path);
                   });
    }
}

TEST_CASE("GradientType") {
    for (GradientType gradientType :
         { GradientType::Linear, GradientType::Radial, GradientType::Angle, GradientType::Reflected }) {

        renderTest("canvas-gradientType-" + fmt::to_string(gradientType), Size{ 320, 320 },
                   [gradientType](RenderContext& context) {
                       PointF pt1{ 160, 160 };
                       PointF pt2{ 260, 260 };
                       Canvas canvas(context);
                       canvas.setFillPaint(Gradient(gradientType, pt1, pt2, Palette::Standard::yellow,
                                                    Palette::Standard::fuchsia));

                       canvas.fillRect({ 0, 0, 320, 320 });
                       canvas.setFillColor(Palette::black);
                       for (PointF p : { pt1, pt2 })
                           canvas.fillEllipse(p.alignedRect({ 6.f, 6.f }, { 0.5f, 0.5f }));
                   });
    }
}

#ifdef BRISK_WEBGPU

static void triangle(wgpu::Device device, wgpu::CommandEncoder encoder, wgpu::TextureView backBuffer,
                     float rotation = 0.f) {
    wgpu::RenderPipeline pipeline;
    wgpu::Buffer uniformBuffer;
    wgpu::BindGroupLayout bindGroupLayout;
    wgpu::BindGroup bindGroup;

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
    wgpu::ShaderModule shaderModule = device.CreateShaderModule(&shaderDesc);

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
    bindGroupLayout                = device.CreateBindGroupLayout(&bindGroupLayoutDesc);

    wgpu::PipelineLayoutDescriptor pipelineLayoutDesc;
    pipelineLayoutDesc.bindGroupLayoutCount = 1;
    pipelineLayoutDesc.bindGroupLayouts     = &bindGroupLayout;

    wgpu::PipelineLayout pipelineLayout     = device.CreatePipelineLayout(&pipelineLayoutDesc);

    // Create pipeline
    wgpu::RenderPipelineDescriptor pipelineDesc{
        .layout    = pipelineLayout,
        .vertex    = { .module = shaderModule, .entryPoint = "vs_main" },
        .primitive = { .topology = wgpu::PrimitiveTopology::TriangleList, .cullMode = wgpu::CullMode::Back },
    };

    wgpu::ColorTargetState colorTarget{ .format    = wgpu::TextureFormat::BGRA8Unorm,
                                        .writeMask = wgpu::ColorWriteMask::All };

    wgpu::FragmentState fragmentState{
        .module = shaderModule, .entryPoint = "fs_main", .targetCount = 1, .targets = &colorTarget
    };
    pipelineDesc.fragment = &fragmentState;

    pipeline              = device.CreateRenderPipeline(&pipelineDesc);

    wgpu::BufferDescriptor bufferDesc;
    bufferDesc.size  = sizeof(float);
    bufferDesc.usage = wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst;
    uniformBuffer    = device.CreateBuffer(&bufferDesc);

    wgpu::BindGroupEntry bindGroupEntry{};
    bindGroupEntry.binding = 0;
    bindGroupEntry.buffer  = uniformBuffer;
    bindGroupEntry.size    = sizeof(float);

    wgpu::BindGroupDescriptor bindGroupDesc{};
    bindGroupDesc.layout     = bindGroupLayout;
    bindGroupDesc.entryCount = 1;
    bindGroupDesc.entries    = &bindGroupEntry;
    bindGroup                = device.CreateBindGroup(&bindGroupDesc);

    // Setup render pass
    wgpu::RenderPassColorAttachment colorAttachment{ .view       = backBuffer,
                                                     .loadOp     = wgpu::LoadOp::Clear,
                                                     .storeOp    = wgpu::StoreOp::Store,
                                                     .clearValue = { 0.0f, 0.0f, 0.0f, 1.0f } };

    wgpu::RenderPassDescriptor renderPassDesc{ .colorAttachmentCount = 1,
                                               .colorAttachments     = &colorAttachment };

    device.GetQueue().WriteBuffer(uniformBuffer, 0, &rotation, sizeof(rotation));

    wgpu::RenderPassEncoder renderPass = encoder.BeginRenderPass(&renderPassDesc);

    renderPass.SetPipeline(pipeline);
    renderPass.SetBindGroup(0, bindGroup);
    renderPass.Draw(3, 1);
    renderPass.End();
}

TEST_CASE("WebGPU") {
    renderTest("webgpu", Size{ 256, 256 },
               [&](RenderContext& context) {
                   Canvas canvas(context);
                   wgpu::Device device;
                   wgpu::TextureView backBuffer;
                   bool ok = webgpuFromContext(canvas.renderContext(), device, backBuffer);
                   REQUIRE(ok);
                   wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
                   triangle(device, encoder, backBuffer, 0.f);
                   wgpu::CommandBuffer commands = encoder.Finish();
                   device.GetQueue().Submit(1, &commands);
               },
               defaultBackColor, defaultMaximumDiff, { RendererBackend::WebGpu });
}
#endif

TEST_CASE("SDF-Border") {
    for (int i = 128; i <= 4096; i *= 2) {
        renderTest<true>("sdf-border-" + std::to_string(i), Size{ i, i }, [i](RenderContext& context) {
            Canvas canvas(context);
            canvas.setFillColor(Palette::transparent);
            canvas.setStrokePaint(LinearGradient({ 0, 0 }, PointF(i, i), Palette::blue, Palette::green));
            canvas.drawRect({ 0.5f, 0.5f, i - 0.5f, i - 0.5f });
            REQUIRE(canvas.rasterizedPaths() == 0);
        });
    }
    for (int i = 128; i <= 4096; i *= 2) {
        renderTest<true>("raster-border-" + std::to_string(i), Size{ i, i }, [i](RenderContext& context) {
            auto dur1 = Internal::performancePathScanline;
            auto dur2 = Internal::performancePathRasterization;
            Canvas canvas(context, CanvasFlags::None);
            canvas.setFillColor(Palette::transparent);
            canvas.setStrokePaint(LinearGradient({ 0, 0 }, PointF(i, i), Palette::blue, Palette::green));
            canvas.drawRect({ 0.5f, 0.5f, i - 0.5f, i - 0.5f });
            fmt::println("{} CPU rasterization times: {} {}\n", "raster-border-" + std::to_string(i),
                         std::chrono::duration_cast<std::chrono::microseconds>(
                             Internal::performancePathScanline - dur1),
                         std::chrono::duration_cast<std::chrono::microseconds>(
                             Internal::performancePathRasterization - dur2));
            REQUIRE(canvas.rasterizedPaths() > 0);
        });
    }
}

} // namespace Brisk
