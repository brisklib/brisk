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
#include <brisk/graphics/Canvas.hpp>

namespace Brisk {

Gradient::Gradient(GradientType type) : m_type(type) {}

Gradient::Gradient(GradientType type, PointF startPoint, PointF endPoint)
    : m_type(type), m_startPoint(startPoint), m_endPoint(endPoint) {}

PointF Gradient::getStartPoint() const {
    return m_startPoint;
}

void Gradient::setStartPoint(PointF pt) {
    m_startPoint = pt;
}

PointF Gradient::getEndPoint() const {
    return m_endPoint;
}

void Gradient::setEndPoint(PointF pt) {
    m_endPoint = pt;
}

void Gradient::addStop(float position, ColorF color) {
    m_colorStops.push_back({ position, color });
}

const ColorStopArray& Gradient::colorStops() const {
    return m_colorStops;
}

const Canvas::State Canvas::defaultState{
    noClipRect,             /* clipRect */
    Matrix{},               /* transform */
    ColorF(Palette::black), /* strokePaint */
    ColorF(Palette::white), /* fillPaint */
    StrokeParams{},         /* dashArray */
    1.f,                    /* opacity */
    FillParams{},           /* fillRule */
};

Canvas::Canvas(RenderContext& context) : RawCanvas(context), m_state(defaultState) {}

Canvas::Canvas(RawCanvas& canvas) : RawCanvas(canvas), m_state(defaultState) {}

const Paint& Canvas::getStrokePaint() const {
    return m_state.strokePaint;
}

void Canvas::setStrokePaint(Paint paint) {
    m_state.strokePaint = std::move(paint);
}

const Paint& Canvas::getFillPaint() const {
    return m_state.fillPaint;
}

void Canvas::setFillPaint(Paint paint) {
    m_state.fillPaint = std::move(paint);
}

float Canvas::getStrokeWidth() const {
    return m_state.strokeParams.strokeWidth;
}

void Canvas::setStrokeWidth(float width) {
    m_state.strokeParams.strokeWidth = width;
}

float Canvas::getOpacity() const {
    return m_state.opacity;
}

void Canvas::setOpacity(float opacity) {
    m_state.opacity = opacity;
}

ColorF Canvas::getStrokeColor() const {
    if (auto* c = get_if<ColorF>(&m_state.strokePaint))
        return *c;
    return Palette::transparent;
}

void Canvas::setStrokeColor(ColorF color) {
    m_state.strokePaint = color;
}

ColorF Canvas::getFillColor() const {
    if (auto* c = get_if<ColorF>(&m_state.fillPaint))
        return *c;
    return Palette::transparent;
}

void Canvas::setFillColor(ColorF color) {
    m_state.fillPaint = color;
}

float Canvas::getMiterLimit() const {
    return m_state.strokeParams.miterLimit;
}

void Canvas::setMiterLimit(float limit) {
    m_state.strokeParams.miterLimit = limit;
}

FillRule Canvas::getFillRule() const {
    return m_state.fillParams.fillRule;
}

void Canvas::setFillRule(FillRule fillRule) {
    m_state.fillParams.fillRule = fillRule;
}

JoinStyle Canvas::getJoinStyle() const {
    return m_state.strokeParams.joinStyle;
}

void Canvas::setJoinStyle(JoinStyle joinStyle) {
    m_state.strokeParams.joinStyle = joinStyle;
}

CapStyle Canvas::getCapStyle() const {
    return m_state.strokeParams.capStyle;
}

void Canvas::setCapStyle(CapStyle capStyle) {
    m_state.strokeParams.capStyle = capStyle;
}

float Canvas::getDashOffset() const {
    return m_state.strokeParams.dashOffset;
}

void Canvas::setDashOffset(float offset) {
    m_state.strokeParams.dashOffset = offset;
}

const DashArray& Canvas::getDashArray() const {
    return m_state.strokeParams.dashArray;
}

void Canvas::setDashArray(const DashArray& array) {
    m_state.strokeParams.dashArray = array;
}

void Canvas::strokeRect(RectangleF rect) {
    Path path;
    path.addRect(rect);
    strokePath(path);
}

void Canvas::fillRect(RectangleF rect) {
    Path path;
    path.addRect(rect);
    fillPath(path);
}

void Canvas::strokeEllipse(RectangleF rect) {
    Path path;
    path.addEllipse(rect);
    strokePath(path);
}

void Canvas::fillEllipse(RectangleF rect) {
    Path path;
    path.addEllipse(rect);
    fillPath(path);
}

void Canvas::strokeLine(PointF pt1, PointF pt2) {
    Path path;
    path.moveTo(pt1);
    path.lineTo(pt2);
    strokePath(path);
}

void Canvas::strokePolygon(std::span<const PointF> points, bool close) {
    if (points.empty())
        return;
    Path path;
    path.moveTo(points.front());
    for (size_t i = 1; i < points.size(); ++i) {
        path.lineTo(points[i]);
    }
    if (close)
        path.close();
    strokePath(path);
}

void Canvas::fillPolygon(std::span<const PointF> points, bool close) {
    if (points.empty())
        return;
    Path path;
    path.moveTo(points.front());
    for (size_t i = 1; i < points.size(); ++i) {
        path.lineTo(points[i]);
    }
    if (close)
        path.close();
    fillPath(path);
}

Font Canvas::getFont() const {
    return m_state.font;
}

void Canvas::setFont(const Font& font) {
    m_state.font = font;
}

void Canvas::reset() {
    m_state = defaultState;
}

void Canvas::save() {
    m_stack.push_back(m_state);
}

void Canvas::restore() {
    if (m_stack.empty())
        return;
    m_state = std::move(m_stack.back());
    m_stack.pop_back();
}

void Canvas::restoreNoPop() {
    if (m_stack.empty())
        return;
    m_state = m_stack.back();
}

Matrix Canvas::getTransform() const {
    return Matrix(m_state.transform);
}

void Canvas::setTransform(const Matrix& matrix) {
    m_state.transform = matrix;
}

void Canvas::transform(const Matrix& matrix) {
    m_state.transform = Matrix(m_state.transform) * matrix;
}

std::optional<Rectangle> Canvas::getClipRect() const {
    if (m_state.clipRect == noClipRect)
        return std::nullopt;
    return m_state.clipRect;
}

void Canvas::setClipRect(Rectangle rect) {
    m_state.clipRect = rect;
}

void Canvas::resetClipRect() {
    m_state.clipRect = noClipRect;
}

void Canvas::setPaint(RenderStateEx& renderState, const Paint& paint) {
    switch (paint.index()) {
    case 0: { // Color
        renderState.fillColor1 = renderState.fillColor2 = ColorF(get<ColorF>(paint));
        break;
    }
    case 1: { // Gradient
        const Gradient& gradient = *get<RC<Gradient>>(paint);
        if (gradient.m_colorStops.empty()) {
            break;
        }
        renderState.gradientPoint1 = m_state.transform.transform(gradient.m_startPoint);
        renderState.gradientPoint2 = m_state.transform.transform(gradient.m_endPoint);
        renderState.gradient       = gradient.m_type;
        renderState.opacity        = m_state.opacity;
        if (gradient.m_colorStops.size() == 1) {
            renderState.fillColor1 = renderState.fillColor2 = ColorF(gradient.m_colorStops.front().color);
        } else if (gradient.m_colorStops.size() == 2) {
            renderState.fillColor1 = ColorF(gradient.m_colorStops.front().color);
            renderState.fillColor2 = ColorF(gradient.m_colorStops.back().color);
        } else {
            renderState.gradientHandle = gradient.rasterize();
        }

        break;
    }
    case 2: { // Texture
        const Texture& texture    = std::get<Texture>(paint);
        renderState.textureMatrix = texture.matrix.invert().value_or(Matrix{});
        renderState.imageHandle   = texture.image;
        renderState.samplerMode   = texture.mode;
        break;
    }
    }
}

void Canvas::drawPath(const RasterizedPath& path, const Paint& paint) {
    RenderStateEx renderState(ShaderType::Mask, 1, nullptr);
    prepareStateInplace(renderState);
    setPaint(renderState, paint);
    GeometryGlyphs data = Internal::pathLayout(renderState.sprites, path);
    if (!data.empty()) {
        m_context.command(std::move(renderState), std::span{ data });
    }
}

Rectangle Canvas::transformedClipRect(const Matrix& matrix, RectangleF clipRect) {
    return horizontalMax(clipRect.p1.v) <= -16777216 && horizontalMin(clipRect.p2.v) >= 16777216
               ? noClipRect
               : Rectangle(matrix.transform(clipRect));
}

void Canvas::strokePath(Path path) {
    strokePath(std::move(path), m_state.strokePaint, m_state.strokeParams, m_state.transform,
               m_state.clipRect);
}

void Canvas::fillPath(Path path) {
    fillPath(std::move(path), m_state.fillPaint, m_state.fillParams, m_state.transform, m_state.clipRect);
}

void Canvas::strokePath(Path path, const Paint& strokePaint, const StrokeParams& params, const Matrix& matrix,
                        RectangleF clipRect) {
    if (!params.dashArray.empty()) {
        path = path.dashed(params.dashArray, params.dashOffset);
    }
    path        = std::move(path).transformed(matrix);
    float scale = matrix.estimateScale();
    drawPath(path.rasterize(params.scale(scale), transformedClipRect(matrix, clipRect)), strokePaint);
}

void Canvas::drawPath(Path path, const Paint& strokePaint, const StrokeParams& strokeParams,
                      const Paint& fillPaint, const FillParams& fillParams, const Matrix& matrix,
                      RectangleF clipRect) {
    fillPath(path, fillPaint, fillParams, matrix, clipRect);
    strokePath(std::move(path), strokePaint, strokeParams, matrix, clipRect);
}

void Canvas::fillPath(Path path, const Paint& fillPaint, const FillParams& fillParams, const Matrix& matrix,
                      RectangleF clipRect) {
    path = std::move(path).transformed(matrix);
    drawPath(path.rasterize(fillParams, transformedClipRect(matrix, clipRect)), fillPaint);
}

static void applier(RenderState* target, Matrix* matrix) {
    target->coordMatrix       = *matrix;
    target->clipInScreenspace = 1;
}

void Canvas::drawImage(RectangleF rect, RC<Image> image, Matrix matrix, SamplerMode samplerMode) {
    drawTexture(rect, image, matrix, &m_state.transform, Arg::samplerMode = samplerMode);
}

void Canvas::fillText(PointF position, const PreparedText& text) {
    fillText(position, { 0, 0 }, text);
}

void Canvas::fillText(PointF position, PointF alignment, const PreparedText& text) {
    if (alignment != PointF{}) {
        position -= text.bounds().size() * alignment;
    }
    drawText(position, text, std::pair{ this, &m_state.fillPaint }, &m_state.transform);
}

void Canvas::fillText(std::string_view text, PointF position, PointF alignment) {
    PreparedText prepared = fonts->prepare(m_state.font, text);
    PointF offset         = prepared.alignLines(alignment.x, alignment.y);
    return fillText(position + offset, prepared);
}

void Canvas::fillText(std::string_view text, RectangleF position, PointF alignment) {
    PreparedText prepared = fonts->prepare(m_state.font, text);
    PointF offset         = prepared.alignLines(alignment.x, alignment.y);
    return fillText(position.at(alignment) + offset, prepared);
}

void applier(RenderStateEx* target, const std::pair<Canvas*, Paint*> canvasAndPaint) {
    canvasAndPaint.first->setPaint(*target, *canvasAndPaint.second);
}

} // namespace Brisk
