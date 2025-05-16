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
#include <brisk/gui/Properties.hpp>
#include <brisk/gui/Gui.hpp>
#include <brisk/core/Hash.hpp>
#include <brisk/gui/Styles.hpp>
#include <brisk/gui/Icons.hpp>
#include <brisk/graphics/Palette.hpp>
#include <yoga/node/Node.h>
#include <yoga/style/Style.h>
#include <yoga/algorithm/CalculateLayout.h>
#include <yoga/algorithm/BoundAxis.h>
#include <brisk/gui/WidgetTree.hpp>
#include <brisk/core/Resources.hpp>

namespace Brisk {

using Internal::PropState;

namespace yoga = facebook::yoga;

namespace Internal {

class LayoutEngine final : public yoga::Node, public yoga::Style {
public:
    friend class Brisk::Widget;

    LayoutEngine(const LayoutEngine&) noexcept = default;

    LayoutEngine(Widget* widget) noexcept : m_widget(widget) {}

    yoga::LayoutResults m_layoutResults{};
    int16_t m_layoutLineIndex = 0;
    bool m_hasNewLayout : 1   = false;
    bool m_layoutDirty : 1    = false;
    bool m_customMeasure : 1  = false;
    std::array<yoga::Style::Length, 2> m_resolvedDimensions{ { yoga::value::undefined(),
                                                               yoga::value::undefined() } };

    // Node

    bool alwaysFormsContainingBlock() const final {
        return true;
    }

    yoga::NodeType getNodeType() const final {
        return yoga::NodeType::Default;
    }

    yoga::LayoutResults& getLayout() final {
        return m_layoutResults;
    }

    void zeroOutLayoutRecursively() final {
        m_widget->layoutResetRecursively();
    }

    void zeroOutLayout() {
        m_layoutResults = {};
        m_layoutResults.setDimension(yoga::Dimension::Width, 0);
        m_layoutResults.setDimension(yoga::Dimension::Height, 0);
        m_hasNewLayout = true;
        m_layoutDirty  = false;
    }

    const yoga::LayoutResults& getLayout() const final {
        return m_layoutResults;
    }

    const yoga::Style& style() const final {
        return *this;
    }

    bool hasBaselineFunc() const noexcept final {
        return false;
    }

    float baseline(float width, float height) const final {
        return 0.f;
    }

    yoga::Node* getChild(size_t index) const final {
        return m_widget->m_widgets[index]->m_layoutEngine.get();
    }

    size_t getChildCount() const final {
        return m_widget->m_widgets.size();
    }

    size_t getLineIndex() const final {
        return m_layoutLineIndex;
    }

    void setLineIndex(size_t lineIndex) final {
        m_layoutLineIndex = lineIndex;
    }

    bool isReferenceBaseline() const final {
        return false;
    }

    void setHasNewLayout() final {
        m_hasNewLayout = true;
        m_widget->layoutSet();
    }

    bool isRoot() const final {
        return m_widget->m_parent == nullptr;
    }

    yoga::Style::Length getResolvedDimension(yoga::Dimension dimension) const final {
        return m_resolvedDimensions[static_cast<size_t>(dimension)];
    }

    void resolveDimension() final {
        for (auto dim : { yoga::Dimension::Width, yoga::Dimension::Height }) {
            auto& resolvedDim = m_resolvedDimensions[yoga::to_underlying(dim)];
            if (maxDimension(dim).isDefined() && yoga::inexactEquals(maxDimension(dim), minDimension(dim))) {
                resolvedDim = maxDimension(dim);
            } else {
                resolvedDim = dimension(dim);
            }

            const bool contentBox = dim == yoga::Dimension::Width
                                        ? m_widget->m_boxSizing && BoxSizingPerAxis::ContentBoxX
                                        : m_widget->m_boxSizing && BoxSizingPerAxis::ContentBoxY;

            if (contentBox && resolvedDim.unit() == yoga::Unit::Point) {
                resolvedDim =
                    yoga::value::points(resolvedDim.value().unwrap() +
                                        paddingAndBorderForAxis(this, yoga::FlexDirection::Column, 0.f));
            }
        }
    }

    void setDirty(bool isDirty) final {
        m_layoutDirty = isDirty;
    }

    bool isDirty() const final {
        return m_layoutDirty;
    }

    void markDirtyAndPropagate() final {
        Widget* self = m_widget;
        do {
            self->m_layoutEngine->m_layoutDirty = true;
            self                                = self->parent();
        } while (self);
    }

    yoga::Size measure(float width, yoga::MeasureMode widthMode, float height,
                       yoga::MeasureMode heightMode) final {
        SizeF size = m_widget->measure(
            AvailableSize{ AvailableLength{ width, static_cast<MeasureMode>(widthMode) },
                           AvailableLength{ height, static_cast<MeasureMode>(heightMode) } });
        return yoga::Size{ size.width, size.height };
    }

    bool hasMeasureFunc() const noexcept final {
        return m_customMeasure;
    }

    // Style

    yoga::Direction direction() const final {
        return yoga::Direction::LTR;
    }

    yoga::FlexDirection flexDirection() const final {
        switch (m_widget->m_layout) {
        case Layout::Horizontal:
            if (m_widget->m_layoutOrder == LayoutOrder::Direct)
                return yoga::FlexDirection::Row;
            else
                return yoga::FlexDirection::RowReverse;
        case Layout::Vertical:
            if (m_widget->m_layoutOrder == LayoutOrder::Direct)
                return yoga::FlexDirection::Column;
            else
                return yoga::FlexDirection::ColumnReverse;
        default:
            BRISK_UNREACHABLE();
        }
    }

    yoga::Justify justifyContent() const final {
        return static_cast<yoga::Justify>(m_widget->m_justifyContent);
    }

    yoga::Align alignContent() const final {
        return static_cast<yoga::Align>(m_widget->m_alignContent);
    }

    yoga::Align alignItems() const final {
        return static_cast<yoga::Align>(m_widget->m_alignItems);
    }

    yoga::Align alignSelf() const final {
        return static_cast<yoga::Align>(m_widget->m_alignSelf);
    }

    yoga::PositionType positionType() const final {
        if (m_widget->m_placement == Placement::Normal)
            return yoga::PositionType::Static;
        else
            return yoga::PositionType::Absolute;
    }

    yoga::Wrap flexWrap() const final {
        return static_cast<yoga::Wrap>(m_widget->m_flexWrap);
    }

    yoga::Overflow overflow() const final {
        yoga::Overflow result = yoga::Overflow::None;
        if (m_widget->m_contentOverflow.x == ContentOverflow::Allow)
            result |= yoga::Overflow::AllowRow;
        if (m_widget->m_contentOverflow.y == ContentOverflow::Allow)
            result |= yoga::Overflow::AllowColumn;
        return result;
    }

    yoga::Display display() const final {
        if (m_widget->m_visible)
            return yoga::Display::Flex;
        else
            return yoga::Display::None;
    }

    yoga::FloatOptional flex() const final {
        return yoga::FloatOptional{};
    }

    yoga::FloatOptional flexGrow() const final {
        return opt(m_widget->m_flexGrow);
    }

    yoga::FloatOptional flexShrink() const final {
        return opt(m_widget->m_flexShrink);
    }

    yoga::Style::Length flexBasis() const final {
        return len(m_widget->m_flexBasis);
    }

    yoga::Style::Length position(yoga::Edge edge) const final {
        return yoga::Style::Length::undefined();
    }

    yoga::Style::Length margin(yoga::Edge edge) const final {
        return fromBorder(m_widget->m_margin, edge);
    }

    yoga::Style::Length padding(yoga::Edge edge) const final {
        return fromBorder(m_widget->m_padding, edge);
    }

    yoga::Style::Length border(yoga::Edge edge) const final {
        return fromBorder(m_widget->m_borderWidth, edge);
    }

    yoga::Style::Length gap(yoga::Gutter gutter) const final {
        switch (gutter) {
        case yoga::Gutter::Column:
            return len(m_widget->m_gap.x);
        case yoga::Gutter::Row:
            return len(m_widget->m_gap.y);
        default: // All
            return yoga::Style::Length::undefined();
        }
    }

    yoga::Style::Length dimension(yoga::Dimension axis) const final {
        return fromSize(m_widget->m_dimensions, axis);
    }

    yoga::Style::Length minDimension(yoga::Dimension axis) const final {
        return fromSize(m_widget->m_minDimensions, axis);
    }

    yoga::Style::Length maxDimension(yoga::Dimension axis) const final {
        yoga::Style::Length result = fromSize(m_widget->m_maxDimensions, axis);
        if (m_widget->m_alignToViewport &&
            (axis == yoga::Dimension::Width ? AlignToViewport::X : AlignToViewport::Y)) {
            Size size      = m_widget->viewportSize();
            float newValue = axis == yoga::Dimension::Width ? size.width : size.height;
            if (result.isDefined() && result.unit() == yoga::Unit::Point) {
                result = Length::pointsUnsafe(std::min(result.value().unwrap(), newValue));
            } else {
                result = Length::pointsUnsafe(newValue);
            }
        }
        return result;
    }

    yoga::FloatOptional aspectRatio() const final {
        return m_widget->m_aspect.hasValue() ? yoga::FloatOptional(m_widget->m_aspect.value())
                                             : yoga::FloatOptional{};
    }

    using EdgeFunction = float (yoga::LayoutResults::*)(yoga::PhysicalEdge) const;

    EdgesF computedBorder() const noexcept {
        return computedEdges(&yoga::LayoutResults::border);
    }

    EdgesF computedMargin() const noexcept {
        return computedEdges(&yoga::LayoutResults::margin);
    }

    EdgesF computedPadding() const noexcept {
        return computedEdges(&yoga::LayoutResults::padding);
    }

private:
    BRISK_INLINE Size viewportSize() const noexcept {
        Size viewport{ 0, 0 };
        if (m_widget->m_tree)
            viewport = m_widget->m_tree->viewportRectangle().size();
        return viewport;
    }

    BRISK_INLINE yoga::Style::Length len(Brisk::Length value, float referenceLength) const noexcept {
        yoga::Style::Length result = len(value);
        if (result.unit() == yoga::Unit::Percent) {
            return yoga::StyleLength::pointsUnsafe(result.resolve(referenceLength).unwrap());
        }
        return result;
    }

    BRISK_INLINE yoga::Style::Length len(Brisk::Length value) const noexcept {
        switch (value.unit()) {
        case LengthUnit::Auto:
            return Length::ofAuto();
        case LengthUnit::Undefined:
            return Length::undefined();

        case LengthUnit::Pixels:
            return Length::pointsUnsafe(value.value() * pixelRatio());
        case LengthUnit::DevicePixels:
            return Length::pointsUnsafe(value.value());
        case LengthUnit::AlignedPixels:
            return Length::pointsUnsafe(std::round(value.value() * pixelRatio()));
        case LengthUnit::Percent:
            return Length::percentUnsafe(value.value());

        case LengthUnit::Vw:
            return Length::pointsUnsafe(value.value() * viewportSize().width * 0.01f);
        case LengthUnit::Vh:
            return Length::pointsUnsafe(value.value() * viewportSize().height * 0.01f);
        case LengthUnit::Vmax:
            return Length::pointsUnsafe(value.value() * viewportSize().shortestSide() * 0.01f);
        case LengthUnit::Vmin:
            return Length::pointsUnsafe(value.value() * viewportSize().longestSide() * 0.01f);

        case LengthUnit::Em:
            return Length::pointsUnsafe(value.value() * m_widget->resolveFontHeight());
        default:
            BRISK_UNREACHABLE();
        }
    }

    BRISK_INLINE yoga::FloatOptional opt(Brisk::OptFloat value) const noexcept {
        if (value.isUndefined())
            return yoga::FloatOptional{};
        else
            return yoga::FloatOptional(value.value());
    }

    BRISK_INLINE yoga::Style::Length fromBorder(EdgesL border, yoga::Edge edge) const {
        switch (edge) {
        case yoga::Edge::Left:
            return len(border.x1);
        case yoga::Edge::Top:
            return len(border.y1);
        case yoga::Edge::Right:
            return len(border.x2);
        case yoga::Edge::Bottom:
            return len(border.y2);
        default: // All, Horizontal, Vertical, Start, End
            return yoga::Style::Length::undefined();
        }
    }

    BRISK_INLINE yoga::Style::Length fromSize(SizeL size, yoga::Dimension axis) const {
        if (m_widget->m_placement == Placement::Window) {
            Size viewport = viewportSize();
            switch (axis) {
            case yoga::Dimension::Width:
                return len(size.x, viewport.width);
            case yoga::Dimension::Height:
                return len(size.y, viewport.height);
            default:
                BRISK_UNREACHABLE();
            }
        } else {
            switch (axis) {
            case yoga::Dimension::Width:
                return len(size.x);
            case yoga::Dimension::Height:
                return len(size.y);
            default:
                BRISK_UNREACHABLE();
            }
        }
    }

    BRISK_INLINE EdgesF computedEdges(EdgeFunction fn) const noexcept {
        EdgesF result;
        result.x1 = (m_layoutResults.*fn)(yoga::PhysicalEdge::Left);
        result.y1 = (m_layoutResults.*fn)(yoga::PhysicalEdge::Top);
        result.x2 = (m_layoutResults.*fn)(yoga::PhysicalEdge::Right);
        result.y2 = (m_layoutResults.*fn)(yoga::PhysicalEdge::Bottom);
        return result;
    }

    Widget* m_widget;
};
} // namespace Internal

void Widget::Iterator::operator++() {
    ++i;
}

void Widget::IteratorEx::operator++() {
    ++i;
}

Widget::IteratorEx Widget::begin(bool reverse) const {
    return IteratorEx{ this, 0, reverse };
}

std::nullptr_t Widget::rend() const {
    return nullptr;
}

Widget::IteratorEx Widget::rbegin() const {
    return IteratorEx{ this, 0, true };
}

std::nullptr_t Widget::end() const {
    return nullptr;
}

Widget::Iterator Widget::begin() const {
    return Iterator{ this, 0 };
}

bool Widget::Iterator::operator!=(std::nullptr_t) const {
    return i < w->m_widgets.size();
}

bool Widget::IteratorEx::operator!=(std::nullptr_t) const {
    return i < w->m_widgets.size();
}

const Rc<Widget>& Widget::Iterator::operator*() const {
    return w->m_widgets[i];
}

const Rc<Widget>& Widget::IteratorEx::operator*() const {
    if (reverse)
        return w->m_widgets[w->m_widgets.size() - 1 - i];
    return w->m_widgets[i];
}

int shufflePalette(int x) {
    constexpr static int indices[] = { 0, 8, 4, 2, 10, 6, 1, 9, 5, 3, 11, 7 };
    return indices[x % 12];
}

namespace Internal {
std::atomic_bool debugRelayoutAndRegenerate{ false };
std::atomic_bool debugBoundaries{ false };
std::atomic_bool debugDirtyRect{ false };
} // namespace Internal

void Widget::requestUpdateLayout() {
    m_layoutEngine->markDirtyAndPropagate();
}

void Widget::restyleIfRequested() {
    switch (m_restyleState) {
    [[likely]] case RestyleState::None:
        break;
    case RestyleState::NeedRestyle:
        doRestyle();
        m_restyleState = RestyleState::None;
        break;
    case RestyleState::NeedRestyleForChildren:
        for (const Ptr& w : *this) {
            w->restyleIfRequested();
        }
        m_restyleState = RestyleState::None;
        break;
    }
}

void Widget::doRestyle() {
    if (auto ss = currentStylesheet()) [[likely]]
        doRestyle(ss, ss == m_stylesheet);
}

void Widget::doRestyle(std::shared_ptr<const Stylesheet> stylesheet, bool root) {
    stylesheet->stylize(this, root);
    for (const Ptr& w : *this) {
        if (w->m_stylesheet) [[unlikely]] {
            w->doRestyle(w->m_stylesheet, true);
        } else {
            w->doRestyle(stylesheet, false);
        }
    }
}

void Widget::requestRestyle() {
    m_restyleState  = RestyleState::NeedRestyle;
    Widget* current = m_parent;
    while (current) {
        current->m_restyleState = std::max(current->m_restyleState, RestyleState::NeedRestyleForChildren);
        current                 = current->m_parent;
    }
}

void Widget::requestStateRestyle() {
    if (m_stateTriggersRestyle) {
        requestRestyle();
    } else {
        if (m_reapplyStyle)
            m_reapplyStyle(this);
    }
}

static bool fontsRegistered = false;

void registerBuiltinFonts() {
    if (fontsRegistered)
        return;

    if (auto& ttf = Resources::loadCached("fonts/default/regular.ttf", true); !ttf.empty()) {
        fonts->addFont(Font::Default, FontStyle::Normal, FontWeight::Regular, ttf, false);
    }
    if (auto& ttf = Resources::loadCached("fonts/default/light.ttf", true); !ttf.empty()) {
        fonts->addFont(Font::Default, FontStyle::Normal, FontWeight::Light, ttf, false);
    }
    if (auto& ttf = Resources::loadCached("fonts/default/bold.ttf", true); !ttf.empty()) {
        fonts->addFont(Font::Default, FontStyle::Normal, FontWeight::Bold, ttf, false);
    }
    if (auto& ttf = Resources::loadCached("fonts/default/regular-italic.ttf", true); !ttf.empty()) {
        fonts->addFont(Font::Default, FontStyle::Italic, FontWeight::Regular, ttf, false);
    }
    if (auto& ttf = Resources::loadCached("fonts/default/light-italic.ttf", true); !ttf.empty()) {
        fonts->addFont(Font::Default, FontStyle::Italic, FontWeight::Light, ttf, false);
    }
    if (auto& ttf = Resources::loadCached("fonts/default/bold-italic.ttf", true); !ttf.empty()) {
        fonts->addFont(Font::Default, FontStyle::Italic, FontWeight::Bold, ttf, false);
    }

    if (auto& ttf = Resources::loadCached("fonts/icons.ttf", true); !ttf.empty()) {
        fonts->addFont(Font::Icons, FontStyle::Normal, FontWeight::Regular, ttf, false);
    }
    if (auto& ttf = Resources::loadCached("fonts/emoji.ttf", true); !ttf.empty()) {
        fonts->addFont(Font::Emoji, FontStyle::Normal, FontWeight::Regular, ttf, false,
                       FontFlags::EnableColor);
    }
    if (auto& ttf = Resources::loadCached("fonts/mono/regular.ttf", true); !ttf.empty()) {
        fonts->addFont(Font::Monospace, FontStyle::Normal, FontWeight::Regular, ttf, false);
    }

    fontsRegistered = true;
}

Builder::Builder(Builder::PushFunc builder, BuilderKind kind) : builder(std::move(builder)), kind(kind) {}

void Builder::run(Widget* w) {
    builder(w);
}

SingleBuilder::SingleBuilder(SingleBuilder::func builder)
    : Builder([builder = std::move(builder)](Widget* target) BRISK_INLINE_LAMBDA -> void {
          target->apply(builder());
      }) {}

IndexedBuilder::IndexedBuilder(IndexedBuilder::func builder)
    : Builder([builder = std::move(builder)](Widget* target) BRISK_INLINE_LAMBDA -> void {
          size_t index = 0;
          for (;;) {
              Rc<Widget> w = builder(index);
              if (!w)
                  return;
              target->apply(std::move(w));
              ++index;
          }
      }) {}

Widget::~Widget() noexcept {
    invalidate();
    for (const Ptr& w : *this) {
        w->m_parent = nullptr;
        w->parentChanged();
    }

    for (WidgetGroup* g : m_groups) {
        std::erase(g->widgets, this);
    }

    setTree(nullptr);
}

void Widget::removeFromGroup(WidgetGroup* group) {
    m_groups.erase(group);
}

void Widget::resetSelection() {
    for (const Ptr& w : *this) {
        w->resetSelection();
    }
}

void Widget::beginConstruction() {
    //
}

void Widget::endConstruction() {
    BRISK_ASSERT(m_inConstruction);
    m_inConstruction = false;
    m_constructed    = true;
    onConstructed();

    requestRestyle();
}

void Widget::onConstructed() {}

void Widget::attached() {}

namespace {

Rectangle roundRect(RectangleF rect) {
    return {
        static_cast<int>(std::round(rect.x1)),
        static_cast<int>(std::round(rect.y1)),
        static_cast<int>(std::round(rect.x2)),
        static_cast<int>(std::round(rect.y2)),
    };
}

struct ResolveParameters {
    float fontHeight = 0.f;
    Size viewportSize;
};

float resolveValue(Length value, float defaultValue, float referenceValue, const ResolveParameters& params) {
    using enum LengthUnit;
    switch (value.unit()) {
    case Pixels:
        return dp(value.value());
    case DevicePixels:
        return value.value();
    case AlignedPixels:
        return std::round(dp(value.value()));
    case Percent:
        return value.value() * referenceValue * 0.01f;

    case Em:
        return value.value() * params.fontHeight;

    case Vw:
        return value.value() * params.viewportSize.width * 0.01f;
    case Vh:
        return value.value() * params.viewportSize.height * 0.01f;
    case Vmin:
        return value.value() * params.viewportSize.shortestSide() * 0.01f;
    case Vmax:
        return value.value() * params.viewportSize.longestSide() * 0.01f;

    default:
        return defaultValue;
    }
}

PointF resolveValue(PointL value, PointF defaultValue, PointF referenceValue,
                    const ResolveParameters& params) {
    PointF result;
    result.x = resolveValue(value.x, defaultValue.x, referenceValue.x, params);
    result.y = resolveValue(value.y, defaultValue.y, referenceValue.y, params);
    return result;
}

[[maybe_unused]] CornersF resolveValue(CornersL value, CornersF defaultValue, CornersF referenceValue,
                                       const ResolveParameters& params) {
    CornersF result;
    result.x1y1 = resolveValue(value.x1y1, defaultValue.x1y1, referenceValue.x1y1, params);
    result.x2y1 = resolveValue(value.x2y1, defaultValue.x2y1, referenceValue.x2y1, params);
    result.x1y2 = resolveValue(value.x1y2, defaultValue.x1y2, referenceValue.x1y2, params);
    result.x2y2 = resolveValue(value.x2y2, defaultValue.x2y2, referenceValue.x2y2, params);
    return result;
}
} // namespace

Size Widget::viewportSize() const noexcept {
    return m_tree ? m_tree->viewportRectangle().size() : Size(0, 0);
}

static Rectangle widgetClipRect(WidgetClip clip, Rectangle selfRect, Rectangle parentRect,
                                Rectangle parentClipRect) {
    Rectangle result = noClipRect;
    if (clip && WidgetClip::SelfRect) {
        result = result.intersection(selfRect);
    }
    if (clip && WidgetClip::ParentRect) {
        result = result.intersection(parentRect);
    }
    if (clip && WidgetClip::ParentClipRect) {
        result = result.intersection(parentClipRect);
    }
    return result;
}

void Widget::computeClipRect() {
    Widget* parent = m_parent;
    if (m_zorder == ZOrder::TopMost)
        parent = nullptr;
    m_clipRect = widgetClipRect(m_clip, m_rect, parent ? parent->m_rect : noClipRect,
                                parent ? parent->m_clipRect : noClipRect);
}

void Widget::prepareHint() {
    if (m_hint.empty()) {
        m_hintPrepared = {};
        return;
    }
    Font font      = Font{ Font::DefaultPlusIconsEmoji, dp(FontSize::Normal - 1) };
    m_hintPrepared = fonts->prepare(font, m_hint);
}

void Widget::computeHintRect() {
    if (m_hintPrepared.lines.empty()) {
        m_hintRect = {};
        return;
    }
    Size textSize = m_hintPrepared.bounds().size();
    Point p       = m_rect.at(0.5f, 1.f);
    m_hintRect    = p.alignedRect(textSize + Size{ 12_idp, 6_idp }, { 0.5f, 0.f });
    if (m_tree && !m_tree->viewportRectangle().empty() && !m_hint.empty()) {
        Size textSize          = m_hintPrepared.bounds().size();
        Rectangle boundingRect = m_tree->viewportRectangle();

        Point p                = m_rect.at(0.5f, 1.f);
        if (m_hintRect.y2 > boundingRect.y2) {
            p          = m_rect.at(0.5f, 0.f);
            m_hintRect = p.alignedRect(textSize + Size{ 12_idp, 6_idp }, { 0.5f, 1.f });
        }

        if (m_hintRect.x1 < boundingRect.x1)
            m_hintRect.applyOffset(boundingRect.x1 - m_hintRect.x1, 0);
        if (m_hintRect.x2 > boundingRect.x2)
            m_hintRect.applyOffset(boundingRect.x2 - m_hintRect.x2, 0);
        m_hintRect.x2 = std::min(m_hintRect.x2, boundingRect.x2);

        if (m_hintRect.y2 > boundingRect.y2)
            m_hintRect.applyOffset(0, boundingRect.y2 - m_hintRect.y2);
        if (m_hintRect.y1 < boundingRect.y1)
            m_hintRect.applyOffset(0, boundingRect.y1 - m_hintRect.y1);
        m_hintRect.y2 = std::min(m_hintRect.y2, boundingRect.y2);
    }
    m_hintTextOffset = m_hintPrepared.alignLines(0.5f, 0.5f);
}

/// Returns the number of changes
int32_t Widget::applyLayoutRecursively(RectangleF rectangle) {
    int32_t counter                = 0;

    m_layoutEngine->m_hasNewLayout = false;
    if (!m_hasLayout) {
        counter += m_previouslyHasLayout != m_hasLayout ? 1 : 0;
        m_previouslyHasLayout = m_hasLayout;
        return counter;
    }
    m_previouslyHasLayout = m_hasLayout;
    auto& layout          = m_layoutEngine->getLayout();
    RectangleF rect;
    if (true || m_layoutEngine->m_hasNewLayout) {
        SizeF dimensions{ layout.dimension(yoga::Dimension::Width),
                          layout.dimension(yoga::Dimension::Height) };
        PointF newOffset;
        Size viewportSize = this->viewportSize();
        ResolveParameters params{ resolveFontHeight(), viewportSize };
        if (m_placement != Placement::Normal) {
            RectangleF referenceRectangle =
                m_placement == Placement::Window ? RectangleF{ PointF(0, 0), viewportSize } : rectangle;
            PointF parent_anchor =
                resolveValue(m_absolutePosition, PointF{}, PointF(SizeF(referenceRectangle.size())), params);
            PointF self_anchor = resolveValue(m_anchor, PointF{}, PointF(dimensions), params);
            newOffset          = referenceRectangle.p1 + parent_anchor - self_anchor;

        } else {
            newOffset = rectangle.p1 + PointF(layout.position(yoga::PhysicalEdge::Left),
                                              layout.position(yoga::PhysicalEdge::Top));
        }
        PointF translate = resolveValue(m_translate, PointF(), PointF(dimensions), params);
        newOffset += translate;

        if (m_alignToViewport && AlignToViewport::X) {
            if (newOffset.x < 0) {
                newOffset.x = 0;
            } else if (newOffset.x + dimensions.x > viewportSize.x) {
                newOffset.x = viewportSize.x - dimensions.x;
            }
        }
        if (m_alignToViewport && AlignToViewport::Y) {
            if (newOffset.y < 0) {
                newOffset.y = 0;
            } else if (newOffset.y + dimensions.y > viewportSize.y) {
                newOffset.y = viewportSize.y - dimensions.y;
            }
        }

        rect.x1 = newOffset.x;
        rect.y1 = newOffset.y;
        rect.x2 = rect.x1 + dimensions.x;
        rect.y2 = rect.y1 + dimensions.y;
        if (assign(m_rect, roundRect(rect))) {
            ++counter;
        }
        if (assign(m_computedBorderWidth, m_layoutEngine->computedBorder())) {
            ++counter;
        }
        if (assign(m_computedPadding, m_layoutEngine->computedPadding())) {
            ++counter;
        }
        if (assign(m_computedMargin, m_layoutEngine->computedMargin())) {
            ++counter;
        }
        if (assign(m_clientRect,
                   roundRect(rect.withPadding(m_computedBorderWidth).withPadding(m_computedPadding)))) {
            ++counter;
        }
    } else {
        rect = m_rect;
    }
    Point bottomRight{ 0, 0 };
    RectangleF rectOffset = rect.withOffset(m_childrenOffset);
    computeClipRect();
    computeHintRect();

    m_subtreeRect = fullPaintRect();

    for (const Ptr& w : *this) {
        if (w->m_ignoreChildrenOffset) {
            counter += w->applyLayoutRecursively(rect);
            bottomRight = max(bottomRight, w->m_rect.p2);
        } else {
            counter += w->applyLayoutRecursively(rectOffset);
            bottomRight = max(bottomRight, w->m_rect.p2 - m_childrenOffset);
        }
        if (w->m_visible)
            m_subtreeRect = m_subtreeRect.union_(w->m_subtreeRect);
    }
    if (assign(m_contentSize, Size((bottomRight - Point(rect.p1)).v))) {
        ++counter;
    }
    if (counter) {
        updateScrollAxes();
        invalidate();
    }
    return counter;
}

bool Widget::setChildrenOffset(Point offset) {
    Point difference = offset - m_childrenOffset;
    if (difference == Point(0, 0))
        return false;
    m_childrenOffset = offset;
    for (const Ptr& w : *this) {
        if (!w->m_ignoreChildrenOffset)
            w->reposition(difference);
    }
    return true;
}

void Widget::reposition(Point relativeOffset) {
    if (relativeOffset == Point(0, 0))
        return;
    invalidate();
    m_rect        = m_rect.withOffset(relativeOffset);
    m_clientRect  = m_clientRect.withOffset(relativeOffset);
    m_subtreeRect = m_subtreeRect.withOffset(relativeOffset);
    m_hintRect    = m_hintRect.withOffset(relativeOffset);
    computeClipRect();
    computeHintRect();
    for (const Ptr& w : *this) {
        w->reposition(relativeOffset);
    }
    if (m_tree)
        m_tree->requestUpdateGeometry();
}

static void showDebugBorder(Canvas& canvas, Rectangle rect, double elapsed, ColorW color);

void Widget::doRefresh() {
    if (m_autoHint && !m_isHintVisible && !m_hint.empty() && m_hoverTime >= 0.0 &&
        frameStartTime - m_hoverTime >= 0.6) {
        m_isHintVisible = true;
        invalidate();
        requestHint();
    }
    onRefresh();
}

void Widget::refreshTree() {
    traverse(
        [](const Ptr& current) -> bool {
            BRISK_ASSERT(!current->m_inConstruction);
            return true;
        },
        [](const Ptr& current) {
            current->doRefresh();
        });
}

void Widget::markTreeDirty() {
    m_layoutEngine->setDirty(true);
    for (const Ptr& w : *this) {
        w->markTreeDirty();
    }
}

void Widget::updateLayout(Rectangle rectangle, bool viewportChanged) {
    // Called by widget tree for the root widget only
    if (viewportChanged)
        markTreeDirty();
    if (yoga::calculateLayout(m_layoutEngine.get(), rectangle.width(), rectangle.height(),
                              yoga::Direction::LTR)) {
        if (applyLayoutRecursively(rectangle)) {
            if (m_tree) {
                m_tree->onLayoutUpdated();
                m_tree->requestUpdateGeometry();
            }
        }
    }
}

void Widget::layoutResetRecursively() {
    if (m_hasLayout) {
        m_hasLayout = false;
        m_layoutEngine->zeroOutLayout();
        m_rect                = Rectangle(m_parent ? m_parent->m_rect.p1 : Point{}, Size{});
        m_clientRect          = m_rect;
        m_contentSize         = {};
        m_computedBorderWidth = {};
        m_computedPadding     = {};
        m_computedMargin      = {};
        for (const Ptr& w : *this) {
            w->layoutResetRecursively();
        }
    }
}

void Widget::layoutSet() {
    m_hasLayout = true;
}

static const char* propState[4]{
    /* 0 */ "-",
    /* 1 */ "overriden",
    /* 2 */ "inherited",
    /* 3 */ "overriden&inherited",
};

void Widget::dump(int depth) const {
    fprintf(stderr, "%*s%s (v=%d/%d rect=[%d,%d ; %d,%d] dirty=%d fs=%s/%.1f/%s tr=%p) {\n", depth * 4, "",
            name().c_str(), m_visible, m_isVisible, m_rect.x1, m_rect.y1, m_rect.x2, m_rect.y2,
            isLayoutDirty(), fmt::to_string(m_fontSize.value).c_str(), m_fontSize.resolved,
            propState[+getPropState(fontSize.index)], m_tree);
    for (const Rc<Widget>& w : *this) {
        w->dump(depth + 1);
    }
    fprintf(stderr, "%*s}\n", depth * 4, "");
}

std::optional<Widget::WidgetIterator> Widget::findIterator(Widget* widget, Widget** parent) {
    WidgetPtrs::iterator it =
        std::find_if(m_widgets.begin(), m_widgets.end(), [&](Ptr p) BRISK_INLINE_LAMBDA {
            return p.get() == widget;
        });
    if (it != m_widgets.end()) {
        if (parent)
            *parent = this;
        return it;
    } else {
        for (const Rc<Widget>& w : *this) {
            std::optional<WidgetPtrs::iterator> wit = w->findIterator(widget, parent);
            if (wit) {
                return *wit;
            }
        }
        return std::nullopt;
    }
}

bool Widget::replace(Rc<Widget> oldWidget, Rc<Widget> newWidget, bool deep) {
    auto it = std::find(m_widgets.begin(), m_widgets.end(), oldWidget);
    if (it != m_widgets.end()) {
        replaceChild(it, std::move(newWidget));
        return true;
    } else if (deep) {
        for (const Rc<Widget>& w : *this) {
            if (w->replace(oldWidget, newWidget, true))
                return true;
        }
    }
    return false;
}

void Widget::apply(WidgetGroup* group) {
    BRISK_ASSERT(group);

    group->widgets.push_back(this);
    m_groups.insert(group);

    if (m_tree) {
        m_tree->addGroup(group);
    }
}

void Widget::rebuildOne(Builder builder) {
    const size_t position     = m_widgets.size();
    const size_t builderCount = m_builders.size();

    if (builder.kind != BuilderKind::Delayed) {
        builder.run(this);
    }
    BRISK_ASSERT(builderCount == m_builders.size());
    const size_t count = m_widgets.size() - position;
    if (builder.kind != BuilderKind::Once) {
        m_builders.push_back(BuilderData{ std::move(builder), uint32_t(position), uint32_t(count) });
    }
    if (count) {
        childrenAdded();
    }
}

void Widget::apply(Builder builder) {
    rebuildOne(std::move(builder));
}

void Widget::doRebuild() {
    m_rebuildRequested = false;
    rebuild(true);
    m_regenerateTime = frameStartTime;
}

void Widget::rebuild(bool force) {
    if (!m_builders.empty()) {
        WidgetPtrs widgetsCopy;
        std::vector<BuilderData> buildersCopy;

        std::swap(widgetsCopy, m_widgets);
        std::swap(buildersCopy, m_builders);
        size_t copied = 0;

        for (BuilderData& g : buildersCopy) {
            if (!(g.builder.kind == BuilderKind::Delayed || force))
                continue;
            m_widgets.insert(m_widgets.end(), std::make_move_iterator(widgetsCopy.begin() + copied),
                             std::make_move_iterator(widgetsCopy.begin() + g.position));
            // reapply & store new BuilderData
            g.builder.kind = BuilderKind::Regular;
            rebuildOne(g.builder);
            copied = g.position + g.count;
        }
        if (copied < widgetsCopy.size()) {
            m_widgets.insert(m_widgets.end(), std::make_move_iterator(widgetsCopy.begin() + copied),
                             std::make_move_iterator(widgetsCopy.end()));
        }
        for (Ptr& p : widgetsCopy) {
            if (p) {
                childRemoved(std::move(p));
            }
        }
    }
}

void Widget::paintTo(Canvas& canvas) const {
    paint(canvas);
}

void Widget::postPaint(Canvas& canvas) const {
    paintFocusFrame(canvas);
    paintHint(canvas);
}

void Widget::paint(Canvas& canvas) const {
    paintBackground(canvas, m_rect);
}

Drawable Widget::drawable() const {
    return [w = shared_from_this()](Canvas& canvas) {
        w->doPaint(canvas);
    };
}

static void showDebugBorder(Canvas& canvas, Rectangle rect, double elapsed, ColorW color) {
    const double displayTime = 1.0;

    if (elapsed < displayTime) {
        float alpha = std::clamp(1.0 - elapsed / displayTime, 0.0, 1.0);
        alpha       = std::pow(alpha, 4.0);
        canvas.setStrokeColor(color.multiplyAlpha(alpha));
        canvas.setStrokeWidth(0.5_dp);
        float radius = std::min(dp(5), rect.size().shortestSide() * 0.5f);
        canvas.strokeRect(rect, radius);
        Path path;
        path.moveTo(rect.at(0, 0));
        path.lineTo(rect.at(1, 1));
        canvas.strokePath(std::move(path));
        path = {};
        path.moveTo(rect.at(1, 0));
        path.lineTo(rect.at(0, 1));
        canvas.strokePath(std::move(path));
    }
}

Widget::ScrollBarGeometry Widget::scrollBarGeometry(Orientation orientation) const noexcept {
    Range<int> range = scrollBarRange(orientation);
    Size trackSize(m_scrollBarThickness.resolved, m_scrollBarThickness.resolved);
    trackSize[+orientation] = m_rect.size()[+orientation];
    Rectangle track         = m_rect.alignedRect(trackSize, { 1.f, 1.f });
    Rectangle thumb{};
    if (range.distance() < trackSize[+orientation] && range.distance() > 0) {
        thumb                  = track;
        thumb.p1[+orientation] = track.p1[+orientation] + range.min;
        thumb.p2[+orientation] = track.p1[+orientation] + range.max;
    }
    return { track, thumb };
}

void Widget::paintScrollBar(Canvas& canvas, Orientation orientation,
                            const ScrollBarGeometry& geometry) const {
    if (isHovered() || m_overflowScroll[+orientation] == OverflowScroll::Enable) {
        canvas.setFillColor(m_scrollBarColor.current.multiplyAlpha(0.25f));
        canvas.fillRect(geometry.track);
    }
    if (!geometry.thumb.empty()) {
        canvas.setFillColor(m_scrollBarColor.current.multiplyAlpha(isHovered() ? 1.f : 0.5f));
        canvas.fillRect(geometry.thumb, m_scrollBarRadius.resolved);
    }
}

void Widget::paintScrollBars(Canvas& canvas) const {
    canvas.setClipRect(m_clipRect);
    for (Orientation orientation : { Orientation::Horizontal, Orientation::Vertical }) {
        if (hasScrollBar(orientation)) {
            paintScrollBar(canvas, orientation, scrollBarGeometry(orientation));
        }
    }
}

constexpr int hintShadowSize = 10;

void Widget::doPaint(Canvas& canvas) const {
    // Skip whole subtree
    if (m_tree && !m_tree->isDirty(m_subtreeRect)) {
        return;
    }

    canvas.setClipRect(m_clipRect);
    bool needsPaint = !m_tree || m_tree->isDirty(adjustedRect()) ||
                      (!m_hintRect.empty() && m_tree->isDirty(adjustedHintRect()));
    if (needsPaint) {
        if (m_painter)
            m_painter.paint(canvas, *this);
        else
            paint(canvas);
    }
    paintChildren(canvas);
    canvas.setClipRect(m_clipRect);
    if (needsPaint) {
        postPaint(canvas);
        paintScrollBars(canvas);
    }

    if (Internal::debugRelayoutAndRegenerate) {
        showDebugBorder(canvas, m_rect, frameStartTime - m_regenerateTime, Palette::Standard::amber);
        showDebugBorder(canvas, m_rect, frameStartTime - m_relayoutTime, Palette::Standard::cyan);
    }
    if (Internal::debugBoundaries) {
        union {
            const void* ptr;
            char bytes[sizeof(void*)];
        } u;

        u.ptr = this;
        showDebugBorder(canvas, m_rect, 0.0, Palette::Standard::index(crc32(u.bytes, 0)));
    }
}

constexpr Range<float> focusFrameRange = { 0, 0.75f };
constexpr inline float focusFrameWidth = 1;

static float remap(float x, float inmin, float inmax, float outmin, float outmax) {
    return (x - inmin) / (inmax - inmin) * (outmax - outmin) + outmin;
}

void Widget::paintFocusFrame(Canvas& canvas) const {
    if (isKeyFocused()) {
        float t = std::sin(static_cast<float>(frameStartTime * 2.5f));
        if (!m_tree)
            return;
        ColorW color = getStyleVar<ColorW>(focusFrameColor.id).value_or(Palette::blue);
        if (color.a > 0) {
            float val = dp(remap(t, -1.0f, +1.0f, focusFrameRange.min, focusFrameRange.max));
            m_tree->requestLayer([val, this, color](Canvas& canvas) {
                canvas.setStrokeColor(color);
                canvas.setStrokeWidth(dp(focusFrameWidth));
                canvas.strokeRect(RectangleF(m_rect).withMargin(val), m_borderRadius.resolved,
                                  m_squircleCorners);
            });
        }
    }
}

void Widget::paintHint(Canvas& canvas) const {
    if ((m_isHintExclusive || isHintCurrent()) && !m_hintPrepared.lines.empty() && m_tree &&
        m_isHintVisible) {
        m_tree->requestLayer([this](Canvas& canvas) {
            SizeF textSize     = m_hintPrepared.bounds().size();
            ColorW color       = getStyleVar<ColorW>(hintBackgroundColor.id).value_or(Palette::white);
            ColorW shadowColor = getStyleVar<ColorW>(hintShadowColor.id).value_or(Palette::black);
            canvas.setFillColor(shadowColor);
            canvas.blurRect(m_hintRect, dp(hintShadowSize), 4._dp, m_squircleCorners);
            canvas.setFillColor(color);
            canvas.fillRect(m_hintRect, 5._dp, m_squircleCorners);
            canvas.setFillColor(getStyleVar<ColorW>(hintTextColor.id).value_or(Palette::black));
            canvas.fillText(m_hintRect.center() + m_hintTextOffset, m_hintPrepared);
        });
    }
}

void Widget::paintBackground(Canvas& canvas, Rectangle rect) const {
    boxPainter(canvas, *this, rect);
}

void Widget::updateState(WidgetState& state, const Event& event, Rectangle rect) {
    if (auto mouse = event.as<EventMouse>()) {
        if (event.type() == EventType::MouseExited) {
            toggle(state, WidgetState::Hover, false);
        } else {
            toggle(state, WidgetState::Hover, rect.contains(mouse->point));
        }
        if (mouse->downPoint && event.type() != EventType::MouseButtonReleased)
            toggle(state, WidgetState::Pressed, rect.contains(*mouse->downPoint));
        else
            toggle(state, WidgetState::Pressed, false);
    }
}

void Widget::paintChildren(Canvas& canvas) const {
    if (m_widgets.empty())
        return;

    for (const Rc<Widget>& w : *this) {
        if (!w->m_visible || w->m_hidden || w->m_clipRect.empty())
            continue;
        if (m_tree && w->m_zorder != ZOrder::Normal) {
            m_tree->requestLayer(w->drawable());
        } else {
            w->doPaint(canvas);
        }
    }
}

void Widget::processTemporaryEvent(Event event) {
    return processEvent(event);
}

void Widget::processEvent(Event& event) {
    BRISK_ASSERT(m_tree);
    const bool pressed  = event.pressed();
    const bool released = event.released();

    if (event.type() == EventType::MouseExited) {
        m_mousePos  = std::nullopt;
        m_hoverTime = -1.0;
        if (m_autoHint && m_isHintVisible) {
            m_isHintVisible = false;
            invalidate();
        }
    } else if (event.type() == EventType::MouseEntered) {
        auto mouse = event.as<EventMouse>();
        m_mousePos = mouse->point;
        if (m_hoverTime < 0.0) {
            m_hoverTime = frameStartTime;
            if (m_autoHint && m_isHintVisible) {
                m_isHintVisible = false;
                invalidate();
            }
        }
    } else if (auto mouse = event.as<EventMouse>()) {
        m_mousePos = mouse->point;
    }

    if (auto focus = event.as<EventFocused>()) {
        reveal();
        toggleState(WidgetState::Focused, true);
        if (focus->keyboard)
            toggleState(WidgetState::KeyFocused, true);
    } else if (event.type() == EventType::Blurred) {
        toggleState(WidgetState::Focused, false);
        toggleState(WidgetState::KeyFocused, false);
    }

    if (event.shouldBubble()) {
        bubbleEvent(event, pressed ? WidgetState::Pressed : WidgetState::None,
                    released ? WidgetState::Pressed : WidgetState::None, false);
    } else {
        this->onEvent(event);
    }
    if (m_autoMouseCapture) {
        if (pressed) {
            inputQueue()->captureMouse(shared_from_this());
        } else if (released) {
            inputQueue()->stopCaptureMouse(shared_from_this());
        }
    }
}

void Widget::onEvent(Event& event) {
    if (event.doubleClicked()) {
        if (m_onDoubleClick.trigger() > 0) {
            event.stopPropagation();
        }
    }
    if (m_processClicks && event.pressed()) {
        if (m_onClick.trigger() > 0) {
            event.stopPropagation();
        }
    }

    if (event.released(m_rect, MouseButton::Right) ||
        event.released(m_rect, MouseButton::Left, KeyModifiers::Control)) {
        if (std::shared_ptr<Widget> p = getContextWidget()) {
            p->rebuild(true);
            Point clickOffset = Point(event.as<EventMouseButtonReleased>()->point) - m_rect.p1;
            p->translate.set(PointL{ clickOffset.x * 1_dpx, clickOffset.y * 1_dpx });
            p->visible = true;
        }
    }

#ifdef BRISK_MACOS
    constexpr float scrollPixels = 20.f;
#else
    constexpr float scrollPixels = 140.f;
#endif

    for (Orientation orientation : { Orientation::Horizontal, Orientation::Vertical }) {
        if (scrollSize(orientation) <= 0)
            continue;
        if (float d = event.wheelScrolled(
                static_cast<WheelOrientation>( // NOLINT(clang-analyzer-optin.core.EnumCastOutOfRange)
                    orientation))) {
            int offset =
                std::clamp(static_cast<int>(std::round(scrollOffset(orientation) - d * dp(scrollPixels))), 0,
                           scrollSize(orientation));

            if (setScrollOffset(orientation, offset)) {
                event.stopPropagation();
            }
        }
        ScrollBarGeometry geometry        = scrollBarGeometry(orientation);

        Rectangle beforeHandleRect        = geometry.track;
        Rectangle afterHandleRect         = geometry.track;
        beforeHandleRect.p2[+orientation] = geometry.thumb.p1[+orientation];
        afterHandleRect.p1[+orientation]  = geometry.thumb.p2[+orientation];

        if (event.released(beforeHandleRect)) {
            setScrollOffset(orientation, scrollOffset(orientation) - m_rect.size()[+orientation]);
            event.stopPropagation();
        } else if (event.released(afterHandleRect)) {
            setScrollOffset(orientation, scrollOffset(orientation) + m_rect.size()[+orientation]);
            event.stopPropagation();
        } else {
            switch (const auto [flag, offset, mod] =
                        event.dragged(geometry.thumb, m_scrollBarDrag[+orientation]);
                    flag) {
            case DragEvent::Started: {
                m_savedScrollOffset = scrollOffset(orientation);
                event.stopPropagation();
                break;
            }
            case DragEvent::Dragging: {
                int axisOffset = std::round(offset[+orientation] /
                                            (geometry.track.size() - geometry.thumb.size())[+orientation] *
                                            scrollSize()[+orientation]);
                setScrollOffset(orientation, m_savedScrollOffset + axisOffset);
                event.stopPropagation();
                break;
            }
            case DragEvent::Dropped:
                event.stopPropagation();
                break;
            default:
                break;
            }
        }
    }

    if (m_delegate) {
        m_delegate->delegatedEvent(this, event);
    }
}

void Widget::bubbleEvent(Event& event, WidgetState enable, WidgetState disable, bool includePopup) {
    bubble(
        [&](Widget* current) BRISK_INLINE_LAMBDA {
            current->setState((current->m_state | enable) & ~disable);
            if (event)
                current->onEvent(event);
            return true;
        },
        includePopup);
}

void Widget::updateGeometry(HitTestMap::State& state) {
    auto self                     = shared_from_this();
    Rectangle mouse_rect          = m_rect;

    HitTestMap::State saved_state = state;
    if (m_zorder != ZOrder::Normal)
        state.zindex--;
    if (m_zorder == ZOrder::Normal)
        mouse_rect = m_rect.intersection(saved_state.scissors);
    state.scissors = mouse_rect;
    state.visible  = state.visible && m_visible && !m_hidden;
    if (m_mouseInteraction == MouseInteraction::Enable)
        state.mouseTransparent = false;
    else if (m_mouseInteraction == MouseInteraction::Disable)
        state.mouseTransparent = true;

    processVisibility(state.visible);

    if (auto inputQueue = this->inputQueue()) {
        if (m_focusCapture && state.visible) {
            inputQueue->enterFocusCapture();
        }
        if (m_tabStop && state.visible) {
            if (!state.inTabGroup)
                ++inputQueue->hitTest.tabGroupId;
            m_tabGroupId = inputQueue->hitTest.tabGroupId;
            if (!state.inTabGroup)
                ++inputQueue->hitTest.tabGroupId;
            inputQueue->addTabStop(self);
        }
        state.inTabGroup = m_tabGroup;
        if (m_tabGroup) {
            ++inputQueue->hitTest.tabGroupId;
        }
        if (state.visible && !state.mouseTransparent)
            inputQueue->hitTest.add(self, mouse_rect, m_mouseAnywhere, state.zindex);
    }

    for (const Ptr& w : *this) {
        w->updateGeometry(state);
    }
    onLayoutUpdated();
    m_relayoutTime = frameStartTime;

    if (auto inputQueue = this->inputQueue()) {
        if (m_focusCapture && state.visible) {
            inputQueue->leaveFocusCapture();
        }

        if (m_autofocus && m_isVisible) {
            inputQueue->setAutoFocus(self);
        }
    }
    state = std::move(saved_state);
}

bool Widget::hasScrollBar(Orientation orientation) const noexcept {
    return m_overflowScroll[+orientation] == OverflowScroll::Enable || scrollSize(orientation) > 0;
}

Size Widget::scrollSize() const noexcept {
    return { scrollSize(Orientation::Horizontal), scrollSize(Orientation::Vertical) };
}

int Widget::scrollSize(Orientation orientation) const noexcept {
    if (m_overflowScroll[+orientation] == OverflowScroll::Disable)
        return 0;
    if (m_contentSize[+orientation] <= m_rect.size()[+orientation])
        return 0;

    return m_contentSize[+orientation] - m_rect.size()[+orientation];
}

Range<int> Widget::scrollBarRange(Orientation orientation) const noexcept {
    int offset      = scrollOffset(orientation);
    int contentSize = m_contentSize[+orientation];
    int boxSize     = m_rect.size()[+orientation];
    if (contentSize < boxSize) {
        return { 0, boxSize };
    }
    if (contentSize == 0 || contentSize == boxSize)
        return { -16777216, -16777216 };

    float value           = std::clamp(static_cast<float>(offset) / (contentSize - boxSize), 0.f, 1.f);
    float range           = std::clamp(static_cast<float>(boxSize) / contentSize, 0.f, 1.f);
    float scrollBarLength = range * boxSize;
    float min             = value * (boxSize - scrollBarLength);
    float max             = min + scrollBarLength;
    return Range<double>{ std::round(min), std::round(max) };
}

void Widget::updateScrollAxes() {
    Point offset = scrollOffset();
    for (Orientation o : { Orientation::Horizontal, Orientation::Vertical }) {
        if (scrollSize(o) <= 0) {
            offset[+o] = 0;
        }
    }
    setScrollOffset(offset);
}

bool Widget::isMenu() const noexcept {
    return m_isMenuRoot || m_role == "menu";
}

void Widget::setMenuRoot() {
    m_isMenuRoot = true;
}

void Widget::processVisibility(bool isVisible) {
    m_previouslyVisible = m_isVisible;
    m_isVisible         = isVisible;
    if (m_isVisible != m_previouslyVisible) {
        invalidate();
        if (m_isVisible) {
            rebuild(false);
            onVisible();
            if (isMenu()) {
                if (auto inputQueue = this->inputQueue()) {
                    inputQueue->startMenu(shared_from_this());
                }
            }
        } else {
            onHidden();
            if (isMenu()) {
                if (auto inputQueue = this->inputQueue()) {
                    inputQueue->finishMenu();
                }
            }
            setState(m_state & ~(WidgetState::Hover | WidgetState::Pressed | WidgetState::Focused |
                                 WidgetState::KeyFocused));
        }
    }
}

template <typename T>
static void assign_inherited(PropState state, T& target, T source, bool& changed) {
    if (state && PropState::Inherited) {
        if (source != target) {
            changed = true;
            target  = source;
        }
    }
}

template <typename T>
static void assign_inherited(PropState state, Internal::Transition<T>& target, T source, bool& changed) {
    if (state && PropState::Inherited) {
        target.set(source, false);
    }
}

void Widget::clear() {
    while (!m_widgets.empty()) {
        Ptr p = m_widgets.front();
        m_widgets.erase(m_widgets.begin());
        childRemoved(p);
    }
    requestUpdateLayout();
    requestRestyle();
}

void Widget::removeIf(function_ref<bool(Widget*)> predicate) {
    size_t num     = 0;
    size_t removed = 0;
    while (m_widgets.size() > num) {
        Ptr p = m_widgets[num];
        if (predicate(p.get())) {
            m_widgets.erase(m_widgets.begin() + num);
            childRemoved(p);
            ++removed;
        } else {
            ++num;
        }
    }
    if (removed) {
        requestUpdateLayout();
        requestRestyle();
    }
}

void Widget::removeAt(size_t pos) {
    if (pos >= m_widgets.size())
        return;
    removeChild(m_widgets.begin() + pos);
}

void Widget::childRemoved(Ptr child) {}

void Widget::removeChild(WidgetConstIterator it) {
    childRemoved(*it);
    m_widgets.erase(it);
}

void Widget::addChild(Ptr w) {
    w->setTree(m_tree);
    insertChild(m_widgets.end(), std::move(w));
}

void Widget::replaceChild(WidgetIterator it, Ptr newWidget) {
    (*it) = std::move(newWidget);
    (*it)->setTree(m_tree);
    (*it)->m_parent = this;
    (*it)->parentChanged();
    requestRestyle();
    requestUpdateLayout();
}

void Widget::insertChild(WidgetConstIterator it, Ptr w) {
    m_widgets.insert(it, w);
    w->m_parent = this;
    w->setTree(m_tree);
    w->parentChanged();
    requestRestyle();
    childAdded(w.get());
}

void Widget::childAdded(Widget* w) {
    onChildAdded(w);
}

void Widget::append(Rc<Widget> widget) {
    if (widget->m_embeddable) {
        const size_t p = m_widgets.size();
        for (size_t i = 0; i < widget->m_widgets.size(); ++i) {
            addChild(std::move(widget->m_widgets[i]));
        }
        for (size_t i = 0; i < widget->m_builders.size(); ++i) {
            BuilderData g = std::move(widget->m_builders[i]);
            g.position += p;
            m_builders.push_back(std::move(g));
        }
        widget->m_widgets.clear();
        widget->m_builders.clear();
        requestUpdateLayout();
        childrenAdded();
    } else {
        addChild(std::move(widget));
        requestUpdateLayout();
        childrenAdded();
    }
}

void Widget::apply(const Rules& rules) {
    rules.applyTo(this);
}

void Widget::toggleState(WidgetState mask, bool on) {
    WidgetState state = m_state;
    toggle(state, mask, on);
    setState(state);
}

void Widget::setState(WidgetState newState) {
    if (newState != m_state) {
        WidgetState savedState = m_state;
        bindings->assign(m_state, newState);
        stateChanged(savedState, newState);
    }
}

void Widget::stateChanged(WidgetState oldState, WidgetState newState) {
    requestStateRestyle();
    if (hasScrollBar(Orientation::Horizontal) || hasScrollBar(Orientation::Vertical)) {
        invalidate();
    }
    // If KeyFocused flag changed
    if ((oldState ^ newState) && WidgetState::KeyFocused) {
        if (newState && WidgetState::KeyFocused)
            requestAnimationFrame();
        invalidate();
    }
    onStateChanged(oldState, newState);
}

void Widget::onStateChanged(WidgetState oldState, WidgetState newState) {}

std::shared_ptr<const Stylesheet> Widget::currentStylesheet() const {
    const Widget* current = this;
    do {
        if (current->m_stylesheet)
            break;
        current = current->m_parent;
    } while (current);
    if (current && current->m_stylesheet) {
        return current->m_stylesheet;
    }
    return nullptr;
}

bool Widget::hasClass(std::string_view className) const {
    return std::find(m_classes.begin(), m_classes.end(), className) != m_classes.end();
}

void Widget::addClass(std::string className) {
    if (!hasClass(className)) {
        m_classes.push_back(std::move(className));
        requestRestyle();
    }
}

void Widget::removeClass(std::string_view className) {
    auto it = std::find(m_classes.begin(), m_classes.end(), className);
    if (it != m_classes.end()) {
        m_classes.erase(it);
        requestRestyle();
    }
}

void Widget::toggleClass(std::string_view className) {
    auto it = std::find(m_classes.begin(), m_classes.end(), className);
    if (it != m_classes.end()) {
        m_classes.erase(it);
    } else {
        m_classes.push_back(std::string(className));
    }
    requestRestyle();
}

Font Widget::font() const {
    return Font{
        m_fontFamily, m_fontSize.resolved,      m_fontStyle,
        m_fontWeight, m_textDecoration,         1.2f,
        8.f,          m_letterSpacing.resolved, m_wordSpacing.resolved,
        0.f,          m_fontFeatures,
    };
}

void Widget::onRefresh() {}

void Widget::onHidden() {}

void Widget::onVisible() {}

bool Widget::transitionAllowed() {
    return !m_inConstruction && (!m_tree || m_tree->transitionsAllowed());
}

float Widget::resolveFontHeight() const {
    return fonts->metrics(font()).vertBounds();
}

template <typename T>
BRISK_INLINE static T getFallback(Widget* widget, T Widget::* field, std::type_identity_t<T> fallback) {
    return widget ? widget->*field : fallback;
}

template <typename T>
BRISK_INLINE static T getFallback(Widget* widget, Internal::Transition<T> Widget::* field,
                                  std::type_identity_t<T> fallback) {
    return widget ? (widget->*field).stopValue : fallback;
}

template <typename T>
BRISK_INLINE static T getFallback(Widget* widget, Internal::Resolve<T> Widget::* field,
                                  std::type_identity_t<T> fallback) {
    return widget ? (widget->*field).value : fallback;
}

void Widget::requestUpdates(PropFlags flags) {

    if (flags && AffectHint) {
        prepareHint();
    }
    if (flags && AffectLayout) {
        requestUpdateLayout();
    }
    if (flags && AffectStyle) {
        requestRestyle();
    }
    if (flags && AffectFont) {
        onFontChanged();
    }
    if (flags && AffectPaint) {
        invalidate();
    }
}

void Widget::resolveProperties(PropFlags flags) {
    Size viewportSize = this->viewportSize();
    if (flags && AffectFont) {
        const Internal::Resolve<Length> parentFont =
            getFallback(m_parent, &Widget::m_fontSize, { 100_perc, dp(FontSize::Normal) });
        if (getPropState(fontSize.index) && PropState::Inherited) {
            m_fontSize = parentFont;
        } else {
            m_fontSize.resolved = resolveValue(m_fontSize.value, 0.f, parentFont.resolved,
                                               ResolveParameters{ parentFont.resolved, viewportSize });
        }
    }
    float resolvedFontHeight = resolveFontHeight();

    if (getPropState(shadowSize.index) && PropState::Inherited) {
        m_shadowSize = getFallback(m_parent, &Widget::m_shadowSize, { 0_px, 0 });
    } else {
        m_shadowSize.resolved = resolveValue(m_shadowSize.value, 0.f, m_rect.shortestSide() * 0.5f,
                                             ResolveParameters{ resolvedFontHeight, viewportSize });
    }

    if (getPropState(squircleCorners.index) && PropState::Inherited) {
        m_squircleCorners = getFallback(m_parent, &Widget::m_squircleCorners, false);
    }

    if (flags && AffectFont) {
        if (getPropState(letterSpacing.index) && PropState::Inherited) {
            m_letterSpacing = getFallback(m_parent, &Widget::m_letterSpacing, { 0_px, 0 });
        } else {
            m_letterSpacing.resolved = resolveValue(m_letterSpacing.value, 0.f, resolvedFontHeight,
                                                    ResolveParameters{ resolvedFontHeight, viewportSize });
        }
        if (getPropState(wordSpacing.index) && PropState::Inherited) {
            m_wordSpacing = getFallback(m_parent, &Widget::m_wordSpacing, { 0_px, 0 });
        } else {
            m_wordSpacing.resolved = resolveValue(m_wordSpacing.value, 0.f, resolvedFontHeight,
                                                  ResolveParameters{ resolvedFontHeight, viewportSize });
        }
        if (getPropState(tabSize.index) && PropState::Inherited) {
            m_tabSize = getFallback(m_parent, &Widget::m_tabSize, { 0_px, 0 });
        } else {
            m_tabSize.resolved = resolveValue(m_tabSize.value, 0.f, resolvedFontHeight,
                                              ResolveParameters{ resolvedFontHeight, viewportSize });
        }

        if (getPropState(fontFamily.index) && PropState::Inherited) {
            m_fontFamily = getFallback(m_parent, &Widget::m_fontFamily, Font::DefaultPlusIconsEmoji);
        }
        if (getPropState(fontStyle.index) && PropState::Inherited) {
            m_fontStyle = getFallback(m_parent, &Widget::m_fontStyle, FontStyle::Normal);
        }
        if (getPropState(fontWeight.index) && PropState::Inherited) {
            m_fontWeight = getFallback(m_parent, &Widget::m_fontWeight, FontWeight::Regular);
        }
        if (getPropState(textDecoration.index) && PropState::Inherited) {
            m_textDecoration = getFallback(m_parent, &Widget::m_textDecoration, TextDecoration::None);
        }
        if (getPropState(fontFeatures.index) && PropState::Inherited) {
            m_fontFeatures = getFallback(m_parent, &Widget::m_fontFeatures, {});
        }
    }
    if (getPropState(color.index) && PropState::Inherited) {
        m_color = getFallback(m_parent, &Widget::m_color, Palette::white);
    }
    if (getPropState(textAlign.index) && PropState::Inherited) {
        m_textAlign = getFallback(m_parent, &Widget::m_textAlign, TextAlign::Start);
    }
    if (getPropState(textVerticalAlign.index) && PropState::Inherited) {
        m_textVerticalAlign = getFallback(m_parent, &Widget::m_textVerticalAlign, TextAlign::Center);
    }
    if (getPropState(scrollBarColor.index) && PropState::Inherited) {
        m_scrollBarColor = getFallback(m_parent, &Widget::m_scrollBarColor, Palette::grey);
    }
    if (getPropState(scrollBarRadius.index) && PropState::Inherited) {
        m_scrollBarRadius = getFallback(m_parent, &Widget::m_scrollBarRadius, 0);
    }
    if (getPropState(scrollBarThickness.index) && PropState::Inherited) {
        m_scrollBarThickness = getFallback(m_parent, &Widget::m_scrollBarThickness, 8_px);
    }

    m_borderRadius.resolved =
        resolveValue(m_borderRadius.value, CornersF(0.f), CornersF(m_rect.shortestSide() * 0.5f),
                     ResolveParameters{ resolvedFontHeight, viewportSize });
    m_scrollBarThickness.resolved = resolveValue(m_scrollBarThickness.value, 0.f, m_rect.shortestSide(),
                                                 ResolveParameters{ resolvedFontHeight, viewportSize });
    m_scrollBarRadius.resolved    = resolveValue(m_scrollBarRadius.value, 0.f, m_rect.shortestSide(),
                                                 ResolveParameters{ resolvedFontHeight, viewportSize });

    requestUpdates(flags);

    for (const Ptr& w : *this) {
        w->resolveProperties(flags);
    }
}

std::optional<std::string> Widget::textContent() const {
    return std::nullopt;
}

void Widget::closeNearestPopup() {
    Widget* self = this;
    bubble([self](Widget* w) BRISK_INLINE_LAMBDA -> bool {
        if (w->m_isPopup) {
            w->close(self);
            return false;
        }
        return true;
    });
}

void Widget::closeMenuChain() {
    Widget* self = this;
    bubble(
        [self](Widget* w) BRISK_INLINE_LAMBDA -> bool {
            if (w->isMenu()) {
                w->close(self);
                return false;
            }
            return true;
        },
        true);
}

void Widget::close(Widget* sender) {
    visible = false;
}

std::string Widget::name() const {
    return std::string(dynamicMetaClass()->className);
}

std::optional<size_t> Widget::indexOf(const Widget* widget) const {
    auto it =
        std::find_if(m_widgets.begin(), m_widgets.end(), [widget](const Rc<Widget>& p) BRISK_INLINE_LAMBDA {
            return p.get() == widget;
        });
    if (it == m_widgets.end())
        return std::nullopt;
    return it - m_widgets.begin();
}

void Widget::remove(Widget* widget) {
    auto it =
        std::find_if(m_widgets.begin(), m_widgets.end(), [widget](const Rc<Widget>& p) BRISK_INLINE_LAMBDA {
            return p.get() == widget;
        });
    BRISK_ASSERT(it != m_widgets.end());
    removeChild(it);
}

void Widget::apply(Rc<Widget> widget) {
    if (widget)
        append(std::move(widget));
}

const Widget::WidgetPtrs& Widget::widgets() const {
    return m_widgets;
}

Rc<Widget> Widget::clone() const {
    Ptr result                       = cloneThis();

    result->m_layoutEngine->m_widget = result.get();
    result->propInit                 = result.get();

    if (result->m_tree) {
        // Attach clone to the same tree
        result->m_tree->attach(result.get());
    }
    result->m_parent = nullptr;
    for (Ptr& w : result->m_widgets) {
        w           = w->clone();
        w->m_parent = result.get();
    }
    return result;
}

Widget::Widget(const Widget&) = default;

Widget::Widget(Construction construction) : m_layoutEngine{ this } {
    registerBuiltinFonts();
    setPropState(fontFamily.index, PropState::Inherited);
    setPropState(fontStyle.index, PropState::Inherited);
    setPropState(fontWeight.index, PropState::Inherited);
    setPropState(textDecoration.index, PropState::Inherited);
    setPropState(fontSize.index, PropState::Inherited);
    setPropState(letterSpacing.index, PropState::Inherited);
    setPropState(wordSpacing.index, PropState::Inherited);
    setPropState(textAlign.index, PropState::Inherited);
    setPropState(color.index, PropState::Inherited);
    setPropState(fontFeatures.index, PropState::Inherited);
    setPropState(scrollBarColor.index, PropState::Inherited);
    setPropState(scrollBarThickness.index, PropState::Inherited);
    setPropState(scrollBarRadius.index, PropState::Inherited);
    setPropState(squircleCorners.index, PropState::Inherited);

    beginConstruction();
    m_type = construction.type;
}

std::optional<PointF> Widget::mousePos() const {
    return m_mousePos;
}

bool Widget::isHintCurrent() const {
    if (auto inputQueue = this->inputQueue())
        return inputQueue->activeHint.lock() == shared_from_this();
    return false;
}

void Widget::requestHint() const {
    if (auto inputQueue = this->inputQueue())
        inputQueue->activeHint = shared_from_this();
}

bool Widget::hasFocus() const {
    if (auto inputQueue = this->inputQueue())
        return inputQueue->focused.lock() == shared_from_this();
    return false;
}

void Widget::blur() {
    if (auto inputQueue = this->inputQueue())
        inputQueue->resetFocus();
}

void Widget::focus(bool byKeyboard) {
    if (auto inputQueue = this->inputQueue())
        inputQueue->setFocus(shared_from_this(), byKeyboard);
}

bool Widget::isVisible() const noexcept {
    return m_isVisible;
}

void Widget::attachedToTree() {
    if (m_pendingAnimationRequest) {
        m_pendingAnimationRequest = false;
        requestAnimationFrame();
    }
}

void Widget::detachedFromTree() {}

void Widget::setTree(WidgetTree* tree) {
    if (tree != m_tree) {
        if (m_tree) {
            m_tree->detach(this);
            detachedFromTree();
        }
        m_tree = tree;
        if (m_tree) {
            m_tree->attach(this);
            attachedToTree();
        }
        for (const Ptr& w : *this) {
            w->setTree(tree);
        }
    }
}

WidgetTree* Widget::tree() const noexcept {
    return m_tree;
}

void Widget::enableCustomMeasure() noexcept {
    m_layoutEngine->m_customMeasure = true;
}

SizeF Widget::measure(AvailableSize size) const {
    return { 0.f, 0.f };
}

SizeF Widget::measuredDimensions() const noexcept {
    return SizeF{ m_layoutEngine->getLayout().measuredDimension(yoga::Dimension::Width),
                  m_layoutEngine->getLayout().measuredDimension(yoga::Dimension::Height) };
}

bool Widget::hadOverflow() const noexcept {
    return m_layoutEngine->getLayout().hadOverflow();
}

bool Widget::isLayoutDirty() const noexcept {
    return m_layoutEngine->m_layoutDirty;
}

void Widget::onLayoutUpdated() {}

Size Widget::contentSize() const noexcept {
    return m_contentSize;
}

EdgesF Widget::computedMargin() const noexcept {
    return m_computedMargin;
}

EdgesF Widget::computedPadding() const noexcept {
    return m_computedPadding;
}

EdgesF Widget::computedBorderWidth() const noexcept {
    return m_computedBorderWidth;
}

SizeF Widget::computeSize(AvailableSize size) {
    SizeF ownerSize{ 0, 0 };
    if (m_parent) {
        ownerSize = m_parent->m_rect.size();
    } else if (m_tree) {
        ownerSize = m_tree->viewportRectangle().size();
    }
    yoga::gCurrentGenerationCount.fetch_add(1, std::memory_order_relaxed);
    yoga::calculateLayoutInternal(
        m_layoutEngine.get(), size.x.value(), size.y.value(), m_layoutEngine->getLayout().lastOwnerDirection,
        yoga::sizingMode(static_cast<yoga::MeasureMode>(size.x.unit())),
        yoga::sizingMode(static_cast<yoga::MeasureMode>(size.y.unit())), ownerSize.x, ownerSize.y, false, 1,
        yoga::gCurrentGenerationCount.load(std::memory_order_relaxed));
    return {
        m_layoutEngine->getLayout().measuredDimension(yoga::Dimension::Width),
        m_layoutEngine->getLayout().measuredDimension(yoga::Dimension::Height),
    };
}

void Widget::requestAnimationFrame() {
    if (m_animationRequested) [[unlikely]]
        return;
    if (!m_tree) [[unlikely]] {
        m_pendingAnimationRequest = true;
        return;
    }
    m_animationRequested = true;
    m_tree->requestAnimationFrame(shared_from_this());
    invalidate();
}

void Widget::requestRebuild() {
    m_rebuildRequested = true;
    if (m_tree) [[likely]] {
        m_tree->requestRebuild(shared_from_this());
    }
}

void Widget::animationFrame() {
    m_animationRequested = false;
    if (m_color.isActive() || m_borderColor.isActive() || m_backgroundColor.isActive() ||
        m_shadowColor.isActive() || isKeyFocused())
        requestAnimationFrame();

    m_color.tick(m_colorTransition, m_colorEasing);
    m_borderColor.tick(m_borderColorTransition, m_borderColorEasing);
    m_backgroundColor.tick(m_backgroundColorTransition, m_backgroundColorEasing);
    m_shadowColor.tick(shadowColorTransition, m_shadowColorEasing);
    if (isKeyFocused()) {
        invalidate();
    }

    onAnimationFrame();
}

void Widget::onAnimationFrame() {}

Value<Trigger<>> Widget::trigRebuild() {
    return Value{
        &m_rebuildTrigger,
        [this]() {
            doRebuild();
        },
    };
}

Widget::Widget(Construction construction, ArgumentsView<Widget> args) : Widget{ construction } {
    args.apply(this);
}

void Widget::onFontChanged() {}

void Widget::apply(const Attributes& arg) {
    arg.applyTo(this);
}

Rc<Widget> Widget::cloneThis() const {
    BRISK_CLONE_IMPLEMENTATION
}

void Widget::childrenAdded() {
    requestRestyle();
}

void Widget::parentChanged() {
    if (m_parent) {
        resolveProperties(AffectResolve | AffectFont | AffectLayout | AffectStyle);
    }
    onParentChanged();
    if (m_parent) {
        for (const auto& fn : m_onParentSet) {
            fn(this);
        }
    }
}

void Widget::onParentChanged() {}

struct MatchMenuRole {
    template <std::derived_from<Widget> WidgetClass>
    constexpr bool operator()(const std::shared_ptr<WidgetClass>& w) const noexcept {
        return w->role.get() == "menu";
    }
};

Rc<Widget> Widget::getContextWidget() {
    return find<Widget>(MatchMenuRole{});
}

void Widget::onChildAdded(Widget* w) {}

WidgetState Widget::state() const noexcept {
    return m_state;
}

bool Widget::isHovered() const noexcept {
    return m_state && WidgetState::Hover;
}

bool Widget::isPressed() const noexcept {
    return m_state && WidgetState::Pressed;
}

bool Widget::isFocused() const noexcept {
    return m_state && WidgetState::Focused;
}

bool Widget::isSelected() const noexcept {
    return m_state && WidgetState::Selected;
}

bool Widget::isKeyFocused() const noexcept {
    return m_state && WidgetState::KeyFocused;
}

bool Widget::isDisabled() const noexcept {
    return m_state && WidgetState::Disabled;
}

Widget* Widget::parent() const noexcept {
    return m_parent;
}

template <typename T>
std::optional<T> Widget::getStyleVar(uint64_t id) const {
    const Widget* self = this;
    do {
        if (auto it = self->m_styleVars.find(id); it != self->m_styleVars.end()) {
            if (const T* val = std::get_if<T>(&it->second)) {
                return *val;
            }
        }
        self = self->m_parent;
    } while (self);
    return std::nullopt;
}

template <typename T>
T Widget::getStyleVar(uint64_t id, T fallback) const {
    return getStyleVar<T>(id).value_or(fallback);
}

template std::optional<ColorW> Widget::getStyleVar<ColorW>(uint64_t) const;
template std::optional<EdgesL> Widget::getStyleVar<EdgesL>(uint64_t) const;
template std::optional<float> Widget::getStyleVar<float>(uint64_t) const;
template std::optional<int> Widget::getStyleVar<int>(uint64_t) const;

template ColorW Widget::getStyleVar<ColorW>(uint64_t, ColorW) const;
template EdgesL Widget::getStyleVar<EdgesL>(uint64_t, EdgesL) const;
template float Widget::getStyleVar<float>(uint64_t, float) const;
template int Widget::getStyleVar<int>(uint64_t, int) const;

const std::string& Widget::type() const noexcept {
    return m_type;
}

Rectangle Widget::clientRect() const noexcept {
    return m_clientRect;
}

Rectangle Widget::rect() const noexcept {
    return m_rect;
}

void Widget::setRect(Rectangle rect) {
    invalidate();
    m_rect        = rect;
    m_clientRect  = rect;
    m_contentSize = rect.size();
    computeClipRect();
    computeHintRect();
    m_subtreeRect = fullPaintRect();
    invalidate();
}

void Widget::reveal() {
    if (m_parent)
        m_parent->revealChild(this);
}

void Widget::revealChild(Widget* child) {
    for (Orientation orientation : { Orientation::Horizontal, Orientation::Vertical }) {
        float scrollSize = this->scrollSize(orientation);
        if (scrollSize > 0) {
            Rectangle containerRect = m_rect;
            Rectangle childRect     = child->rect();
            int32_t offset          = childRect.p1[+orientation] - containerRect.p1[+orientation];
            if (offset < 0) {
                setScrollOffset(
                    orientation,
                    std::clamp(static_cast<float>(scrollOffset(orientation) + offset), 0.f, scrollSize));
            } else {
                offset = childRect.p2[+orientation] - containerRect.p2[+orientation];
                if (offset > 0) {
                    setScrollOffset(
                        orientation,
                        std::clamp(static_cast<float>(scrollOffset(orientation) + offset), 0.f, scrollSize));
                }
            }
        }
    }

    if (m_parent)
        m_parent->revealChild(child);
}

Painter::Painter(PaintFunc painter) : painter(std::move(painter)) {}

void Painter::paint(Canvas& canvas, const Widget& w) const {
    if (painter) {
        painter(canvas, w);
    }
}

void boxPainter(Canvas& canvas, const Widget& widget) {
    boxPainter(canvas, widget, widget.rect());
}

void boxPainter(Canvas& canvas, const Widget& widget, RectangleF rect) {
    ColorW m_backgroundColor = widget.backgroundColor.current().multiplyAlpha(widget.opacity.get());
    ColorW m_borderColor     = widget.borderColor.current().multiplyAlpha(widget.opacity.get());

    if (widget.shadowSize.resolved() > 0) {
        auto&& clipRect = canvas.saveClipRect();
        if (widget.parent()) {
            if (widget.zorder != ZOrder::Normal || widget.clip == WidgetClip::None)
                *clipRect = noClipRect;
            else
                *clipRect = RectangleF(widget.parent()->clipRect());
        }

        canvas.setFillColor(widget.shadowColor.current().multiplyAlpha(widget.opacity.get()));
        canvas.blurRect(rect.withOffset(scalePixels(widget.shadowOffset))
                            .withMargin(scalePixels(widget.shadowSpread.get())),
                        widget.shadowSize.resolved(),
                        widget.borderRadius.resolved() + scalePixels(widget.shadowSpread.get()));
    }

    EdgesF borderWidth = widget.computedBorderWidth();

    if (m_backgroundColor != ColorW(0, 0, 0, 0) || !borderWidth.empty()) {
        CornersF borderRadius = widget.borderRadius.resolved();
        bool squircle         = widget.squircleCorners.get();
        float maxBorderWidth  = borderWidth.max();
        float maxBorderRadius = borderRadius.max();
        RectangleF innerRect  = rect.withPadding(borderWidth * 0.5f);

        if (maxBorderWidth == borderWidth.min() &&
            (maxBorderRadius == 0 || maxBorderRadius > maxBorderWidth * 0.5f)) {
            // Edges widths are equal
            canvas.setStrokeColor(m_borderColor);
            canvas.setStrokeWidth(maxBorderWidth);
            canvas.setFillColor(m_backgroundColor);
            canvas.drawRect(innerRect,
                            CornersF(max(borderRadius.v - maxBorderWidth * 0.5f, Simd<float, 4>(0))),
                            squircle);
        } else {
            Simd<float, 4> corr = borderWidth.v.shuffle(size_constants<0, 1, 3, 2>{}) +
                                  borderWidth.v.shuffle(size_constants<1, 2, 0, 3>{});
            corr *= 0.5f;
            canvas.setFillColor(m_backgroundColor);
            canvas.fillRect(innerRect, CornersF(max(borderRadius.v - corr * 0.5f, Simd<float, 4>(0))),
                            squircle);

            canvas.setFillColor(m_borderColor);

            uint8_t bits = maskToBits(ne(borderWidth.v, Simd<float, 4>{ 0 }));
            if (maxBorderRadius != 0) {
                bits = 0b1111; // Disable short path for border drawaing
            }
            switch (bits) {
            case 0b0001: // left
                canvas.fillRect(rect.alignedRect({ borderWidth[0], rect.height() }, { 0.f, 0.5f }));
                break;
            case 0b0010: // top
                canvas.fillRect(rect.alignedRect({ rect.width(), borderWidth[1] }, { 0.5f, 0.f }));
                break;
            case 0b0100: // right
                canvas.fillRect(rect.alignedRect({ borderWidth[2], rect.height() }, { 1.f, 0.5f }));
                break;
            case 0b1000: // bottom
                canvas.fillRect(rect.alignedRect({ rect.width(), borderWidth[3] }, { 0.5f, 1.f }));
                break;
            case 0b0101: // left+right
                canvas.fillRect(rect.alignedRect({ borderWidth[0], rect.height() }, { 0.f, 0.5f }));
                canvas.fillRect(rect.alignedRect({ borderWidth[2], rect.height() }, { 1.f, 0.5f }));
                break;
            case 0b1010: // top+bottom
                canvas.fillRect(rect.alignedRect({ rect.width(), borderWidth[1] }, { 0.5f, 0.f }));
                canvas.fillRect(rect.alignedRect({ rect.width(), borderWidth[3] }, { 0.5f, 1.f }));
                break;
            default: {
                Path path;
                path.addRoundRect(rect, CornersF(borderRadius.v), squircle, Path::Direction::CW);
                path.addRoundRect(rect.withPadding(borderWidth),
                                  CornersF(max(borderRadius.v - corr, Simd<float, 4>(0))), squircle,
                                  Path::Direction::CCW);
                canvas.fillPath(path);
            }
            }
        }
    }
}

namespace Internal {

template <typename T, uint32_t mask>
static void blendValue(T& value, const T& newValue) {
    int index = 0;
    forEachField<T>([&value, &newValue, &index]<typename FT>(ReflectionField<T, FT> field)
                        BRISK_INLINE_LAMBDA {
                            if ((1 << index++) & mask)
                                value.*field.pointerToField = newValue.*field.pointerToField;
                        });
}

template <typename T>
BRISK_INLINE inline const T& getterConvert(const T& value) {
    return value;
}

template <typename T>
BRISK_INLINE inline const T& getterConvert(const Internal::Resolve<T>& value) {
    return value.value;
}

template <typename T>
BRISK_INLINE inline const T& getterConvert(const Internal::Transition<T>& value) {
    return value.stopValue;
}

template <bool resolved, typename U>
U& resolveField(std::bool_constant<resolved>, U& field) noexcept {
    return field;
}

template <typename InputT>
typename Internal::ResolvedType<InputT>::Type& resolveField(std::bool_constant<true>,
                                                            Internal::Resolve<InputT>& field) noexcept {
    return field.resolved;
}

template <typename InputT>
InputT& resolveField(std::bool_constant<false>, Internal::Resolve<InputT>& field) noexcept {
    return field.value;
}

template <typename InputT>
const typename Internal::ResolvedType<InputT>::Type& resolveField(
    std::bool_constant<true>, const Internal::Resolve<InputT>& field) noexcept {
    return field.resolved;
}

template <typename InputT>
const InputT& resolveField(std::bool_constant<false>, const Internal::Resolve<InputT>& field) noexcept {
    return field.value;
}

template <auto Widget::* field, int subfield, typename U, bool resolved>
decltype(auto) subField(std::bool_constant<resolved> cresolved, U&& self) noexcept {
    auto& f = resolveField(cresolved, self.*field);
    if constexpr (subfield == -1)
        return f;
    else
        return accessField<subfield>(f);
}
} // namespace Internal

template <typename T>
float Widget::* Widget::transitionField(Internal::Transition<T> Widget::* field) const noexcept {
    if (field == &Widget::m_backgroundColor)
        return &Widget::m_backgroundColorTransition;
    else if (field == &Widget::m_borderColor)
        return &Widget::m_borderColorTransition;
    else if (field == &Widget::m_color)
        return &Widget::m_colorTransition;
    else if (field == &Widget::m_shadowColor)
        return &Widget::m_shadowColorTransition;
    else
        return nullptr;
}

template <typename T, PropFlags flags, PropFieldStorageType<T, flags> Widget::* field, int subfield>
OptConstRef<PropFieldType<T, subfield>> Widget::getter() const noexcept {
    return Internal::getterConvert(Internal::subField<field, subfield>(std::false_type{}, *this));
}

template <typename T, PropFlags flags, PropFieldStorageType<T, flags> Widget::* field, int subfield>
OptConstRef<ResolvedType<PropFieldType<T, subfield>>> Widget::getterResolved() const noexcept {
    return Internal::subField<field, subfield>(std::true_type{}, *this);
}

template <typename T, PropFlags flags, PropFieldStorageType<T, flags> Widget::* field, int subfield>
OptConstRef<PropFieldType<T, subfield>> Widget::getterCurrent() const noexcept {
    return Internal::subField<field, subfield>(std::false_type{}, *this).current;
}

// NOLINTBEGIN(clang-analyzer-optin.core.EnumCastOutOfRange)
template <typename T, size_t index, PropFlags flags, PropFieldStorageType<T, flags> Widget::* field_,
          int subfield_>
void Widget::setter(PropFieldType<T, subfield_> value) {
    auto& field = Internal::subField<field_, subfield_>(std::false_type{}, *this);

    if constexpr (index != noIndex) {
        PropState state = getPropState(index);
        if (!m_styleApplying) {
            state |= PropState::Overriden;
        } else if (state && PropState::Overriden) {
            return; // Attempting to modify overriden property
        }

        state &= ~PropState::Inherited; // Clear inherited
        setPropState(index, state);
    }

    if constexpr (flags && Resolvable) {
        if (value == field) {
            return; // Not changed
        }
        field = value;
    } else {
        if constexpr ((flags && Transition)) {
            auto tf = transitionField(field_);
            if (!tf)
                return;
            if (!field.set(value, transitionAllowed() ? this->*tf : 0.f))
                return;
            if (field.isActive()) {
                requestAnimationFrame();
            }
        } else {
            if (value == field) {
                return; // Not changed
            }
            field = value;
        }
    }

    // Resolve
    if constexpr (flags && Inheritable || flags && Resolvable || flags && AffectResolve) {
        resolveProperties(flags);
    } else {
        requestUpdates(flags);
    }

    bindings->notify(&field);
}

// NOLINTEND(clang-analyzer-optin.core.EnumCastOutOfRange)

template <typename T, size_t index, PropFlags flags, PropFieldStorageType<T, flags> Widget::* field,
          int subfield>
void Widget::setter(Inherit) {
    static_assert(index != noIndex);

    PropState state = getPropState(index);

    if (!m_styleApplying) {
        state |= PropState::Overriden;
    } else if (state && PropState::Overriden) {
        return; // Attempting to modify overriden property
    }

    state |= PropState::Inherited;
    setPropState(index, state);

    if (!m_parent)
        return; // No one to inherit from

    resolveProperties(flags); // NOLINT(clang-analyzer-optin.core.EnumCastOutOfRange)
}

PropState Widget::getPropState(size_t index) const noexcept {
    const size_t shift = index * Internal::propStateBits;
    using bitset_t     = std::bitset<Internal::propStateBits * Internal::numProperties>;
    return static_cast<PropState>(((m_propStates >> shift) & bitset_t(+PropState::Mask)).to_ulong());
}

void Widget::setPropState(size_t index, PropState state) noexcept {
    const size_t shift = index * Internal::propStateBits;
    using bitset_t     = std::bitset<Internal::propStateBits * Internal::numProperties>;
    m_propStates &= ~(bitset_t(+PropState::Mask) << shift);
    m_propStates |= bitset_t(+state) << shift;
}

template <size_t index_, typename T, PropFlags flags_, PropFieldStorageType<T, flags_> Widget::* field_,
          int subfield_>
BindingAddress GuiProperty<index_, T, flags_, field_, subfield_>::address() const noexcept {
    return toBindingAddress(&Internal::subField<field, subfield>(std::false_type{}, *this_pointer));
}

template <size_t index_, typename T, PropFlags flags_, PropFieldStorageType<T, flags_> Widget::* field_,
          int subfield_>
auto GuiProperty<index_, T, flags_, field_, subfield_>::get() const noexcept -> OptConstRef<ValueType> {
    return this_pointer->getter<T, flags_, field, subfield>();
}

template <size_t index_, typename T, PropFlags flags_, PropFieldStorageType<T, flags_> Widget::* field,
          int subfield>
auto GuiProperty<index_, T, flags_, field, subfield>::resolved() const noexcept
    -> OptConstRef<ResolvedType<ValueType>>
    requires(isResolvable(flags_))
{
    return this_pointer->getterResolved<T, flags_, field, subfield>();
}

template <size_t index_, typename T, PropFlags flags_, PropFieldStorageType<T, flags_> Widget::* field,
          int subfield>
auto GuiProperty<index_, T, flags_, field, subfield>::current() const noexcept -> OptConstRef<ValueType>
    requires(isTransition(flags_))
{
    return this_pointer->getterCurrent<T, flags_, field, subfield>();
}

template <size_t index_, typename T, PropFlags flags_, PropFieldStorageType<T, flags_> Widget::* field,
          int subfield>
void GuiProperty<index_, T, flags_, field, subfield>::internalSet(ValueType value) {
    this_pointer->setter<T, index, flags_, field, subfield>(std::move(value));
}

template <size_t index_, typename T, PropFlags flags_, PropFieldStorageType<T, flags_> Widget::* field,
          int subfield>
void GuiProperty<index_, T, flags_, field, subfield>::internalSetInherit() {
    if constexpr (isInheritable(flags_)) // NOLINT(clang-analyzer-optin.core.EnumCastOutOfRange)
        this_pointer->setter<T, index, flags_, field, subfield>(inherit);
}

template <size_t index_, typename Type_, PropFlags flags_,
          PropFieldStorageType<Type_, flags_> Widget::* field, typename... Properties>
OptConstRef<Type_> GuiPropertyCompound<index_, Type_, flags_, field, Properties...>::get() const noexcept {
    return Internal::getterConvert(this_pointer->*field);
}

template <size_t index_, typename Type_, PropFlags flags_,
          PropFieldStorageType<Type_, flags_> Widget::* field, typename... Properties>
OptConstRef<ResolvedType<Type_>> GuiPropertyCompound<index_, Type_, flags_, field, Properties...>::resolved()
    const noexcept
    requires(isResolvable(flags_))
{
    return (this_pointer->*field).resolved;
}

template <size_t index_, typename Type_, PropFlags flags_,
          PropFieldStorageType<Type_, flags_> Widget::* field, typename... Properties>
void GuiPropertyCompound<index_, Type_, flags_, field, Properties...>::internalSet(Type value) {
    (Properties{ this_pointer }.internalSet(Properties::sub(value)), ...);
}

template <size_t index_, typename Type_, PropFlags flags_,
          PropFieldStorageType<Type_, flags_> Widget::* field, typename... Properties>
void GuiPropertyCompound<index_, Type_, flags_, field, Properties...>::internalSetInherit() {
    if constexpr (isInheritable(flags_)) // NOLINT(clang-analyzer-optin.core.EnumCastOutOfRange)
        (Properties{ this_pointer }.internalSetInherit(), ...);
}

template <size_t index_, typename Type_, PropFlags flags_,
          PropFieldStorageType<Type_, flags_> Widget::* field, typename... Properties>
BindingAddress GuiPropertyCompound<index_, Type_, flags_, field, Properties...>::address() const noexcept {
    return toBindingAddress(&(this_pointer->*field));
}

namespace Internal {

const std::string_view propNames[numProperties]{
    /* 0*/ "absolutePosition",
    /* 1*/ "alignContent",
    /* 2*/ "alignItems",
    /* 3*/ "alignSelf",
    /* 4*/ "anchor",
    /* 5*/ "aspect",
    /* 6*/ "backgroundColorEasing",
    /* 7*/ "backgroundColorTransition",
    /* 8*/ "backgroundColor",
    /* 9*/ "borderColorEasing",
    /*10*/ "borderColorTransition",
    /*11*/ "borderColor",
    /*12*/ "borderRadiusTopLeft",
    /*13*/ "borderRadiusTopRight",
    /*14*/ "borderRadiusBottomLeft",
    /*15*/ "borderRadiusBottomRight",
    /*16*/ "borderWidthLeft",
    /*17*/ "borderWidthTop",
    /*18*/ "borderWidthRight",
    /*19*/ "borderWidthBottom",
    /*20*/ "clip",
    /*21*/ "colorEasing",
    /*22*/ "colorTransition",
    /*23*/ "color",
    /*24*/ "shadowOffset",
    /*25*/ "cursor",
    /*26*/ "width",
    /*27*/ "height",
    /*28*/ "flexBasis",
    /*29*/ "flexGrow",
    /*30*/ "flexShrink",
    /*31*/ "flexWrap",
    /*32*/ "fontFamily",
    /*33*/ "fontSize",
    /*34*/ "fontStyle",
    /*35*/ "fontWeight",
    /*36*/ "gapColumn",
    /*37*/ "gapRow",
    /*38*/ "hidden",
    /*39*/ "justifyContent",
    /*40*/ "layoutOrder",
    /*41*/ "layout",
    /*42*/ "letterSpacing",
    /*43*/ "marginLeft",
    /*44*/ "marginTop",
    /*45*/ "marginRight",
    /*46*/ "marginBottom",
    /*47*/ "maxWidth",
    /*48*/ "maxHeight",
    /*49*/ "minWidth",
    /*50*/ "minHeight",
    /*51*/ "opacity",
    /*52*/ "overflow",
    /*53*/ "paddingLeft",
    /*54*/ "paddingTop",
    /*55*/ "paddingRight",
    /*56*/ "paddingBottom",
    /*57*/ "placement",
    /*58*/ "shadowSize",
    /*59*/ "shadowColor",
    /*60*/ "shadowColorTransition",
    /*61*/ "shadowColorEasing",
    /*62*/ "tabSize",
    /*63*/ "textAlign",
    /*64*/ "textVerticalAlign",
    /*65*/ "textDecoration",
    /*66*/ "translate",
    /*67*/ "visible",
    /*68*/ "wordSpacing",
    /*69*/ "alignToViewport",
    /*70*/ "boxSizing",
    /*71*/ "zorder",
    /*72*/ "stateTriggersRestyle",
    /*73*/ "id",
    /*74*/ "role",
    /*75*/ "classes",
    /*76*/ "mouseInteraction",
    /*77*/ "mousePassThrough",
    /*78*/ "autoMouseCapture",
    /*79*/ "mouseAnywhere",
    /*80*/ "focusCapture",
    /*81*/ "isHintVisible",
    /*82*/ "tabStop",
    /*83*/ "tabGroup",
    /*84*/ "autofocus",
    /*85*/ "autoHint",
    /*86*/ "squircleCorners",
    /*87*/ "delegate",
    /*88*/ "hint",
    /*89*/ "stylesheet",
    /*90*/ "painter",
    /*91*/ "isHintExclusive",
    /*92*/ "borderRadius",
    /*93*/ "borderWidth",
    /*94*/ "dimensions",
    /*95*/ "gap",
    /*96*/ "margin",
    /*97*/ "maxDimensions",
    /*98*/ "minDimensions",
    /*99*/ "padding",
    /*100*/ "fontFeatures",
    /*101*/ "scrollBarColor",
    /*102*/ "scrollBarThickness",
    /*103*/ "scrollBarRadius",
    /*104*/ "shadowSpread",
};

} // namespace Internal

uint64_t hashSum = 0;

template <PropertyLike Prop>
void instantiateProp() {
    fastHashAccum(hashSum, &Prop::get);
    if constexpr (Prop::flags && PropFlags::Resolvable) {
        fastHashAccum(hashSum, &Prop::resolved);
    }
    if constexpr (Prop::flags && PropFlags::Transition) {
        fastHashAccum(hashSum, &Prop::current);
    }
    using Type = typename Prop::ValueType;
    fastHashAccum(hashSum, &Prop::internalSet);
    fastHashAccum(hashSum, &Prop::internalSetInherit);
    fastHashAccum(hashSum, &Prop::address);
}

template void instantiateProp<decltype(Widget::borderRadius)>();
template void instantiateProp<decltype(Widget::borderWidth)>();
template void instantiateProp<decltype(Widget::dimensions)>();
template void instantiateProp<decltype(Widget::gap)>();
template void instantiateProp<decltype(Widget::margin)>();
template void instantiateProp<decltype(Widget::maxDimensions)>();
template void instantiateProp<decltype(Widget::minDimensions)>();
template void instantiateProp<decltype(Widget::padding)>();

template void instantiateProp<decltype(Widget::absolutePosition)>();
template void instantiateProp<decltype(Widget::alignContent)>();
template void instantiateProp<decltype(Widget::alignItems)>();
template void instantiateProp<decltype(Widget::alignSelf)>();
template void instantiateProp<decltype(Widget::anchor)>();
template void instantiateProp<decltype(Widget::aspect)>();
template void instantiateProp<decltype(Widget::backgroundColorEasing)>();
template void instantiateProp<decltype(Widget::backgroundColorTransition)>();
template void instantiateProp<decltype(Widget::backgroundColor)>();
template void instantiateProp<decltype(Widget::borderColorEasing)>();
template void instantiateProp<decltype(Widget::borderColorTransition)>();
template void instantiateProp<decltype(Widget::borderColor)>();
template void instantiateProp<decltype(Widget::clip)>();
template void instantiateProp<decltype(Widget::colorEasing)>();
template void instantiateProp<decltype(Widget::colorTransition)>();
template void instantiateProp<decltype(Widget::color)>();
template void instantiateProp<decltype(Widget::cursor)>();
template void instantiateProp<decltype(Widget::flexBasis)>();
template void instantiateProp<decltype(Widget::flexGrow)>();
template void instantiateProp<decltype(Widget::flexShrink)>();
template void instantiateProp<decltype(Widget::flexWrap)>();
template void instantiateProp<decltype(Widget::fontFamily)>();
template void instantiateProp<decltype(Widget::fontSize)>();
template void instantiateProp<decltype(Widget::fontStyle)>();
template void instantiateProp<decltype(Widget::fontWeight)>();
template void instantiateProp<decltype(Widget::hidden)>();
template void instantiateProp<decltype(Widget::justifyContent)>();
template void instantiateProp<decltype(Widget::layoutOrder)>();
template void instantiateProp<decltype(Widget::layout)>();
template void instantiateProp<decltype(Widget::letterSpacing)>();
template void instantiateProp<decltype(Widget::opacity)>();
template void instantiateProp<decltype(Widget::overflowScrollX)>();
template void instantiateProp<decltype(Widget::overflowScrollY)>();
template void instantiateProp<decltype(Widget::overflowScroll)>();
template void instantiateProp<decltype(Widget::contentOverflowX)>();
template void instantiateProp<decltype(Widget::contentOverflowY)>();
template void instantiateProp<decltype(Widget::contentOverflow)>();
template void instantiateProp<decltype(Widget::placement)>();
template void instantiateProp<decltype(Widget::shadowSize)>();
template void instantiateProp<decltype(Widget::shadowOffset)>();
template void instantiateProp<decltype(Widget::shadowColor)>();
template void instantiateProp<decltype(Widget::shadowColorTransition)>();
template void instantiateProp<decltype(Widget::shadowColorEasing)>();
template void instantiateProp<decltype(Widget::tabSize)>();
template void instantiateProp<decltype(Widget::textAlign)>();
template void instantiateProp<decltype(Widget::textVerticalAlign)>();
template void instantiateProp<decltype(Widget::textDecoration)>();
template void instantiateProp<decltype(Widget::translate)>();
template void instantiateProp<decltype(Widget::visible)>();
template void instantiateProp<decltype(Widget::wordSpacing)>();
template void instantiateProp<decltype(Widget::alignToViewport)>();
template void instantiateProp<decltype(Widget::stateTriggersRestyle)>();
template void instantiateProp<decltype(Widget::id)>();
template void instantiateProp<decltype(Widget::role)>();
template void instantiateProp<decltype(Widget::classes)>();
template void instantiateProp<decltype(Widget::mouseInteraction)>();
template void instantiateProp<decltype(Widget::mousePassThrough)>();
template void instantiateProp<decltype(Widget::autoMouseCapture)>();
template void instantiateProp<decltype(Widget::mouseAnywhere)>();
template void instantiateProp<decltype(Widget::focusCapture)>();
template void instantiateProp<decltype(Widget::isHintVisible)>();
template void instantiateProp<decltype(Widget::tabStop)>();
template void instantiateProp<decltype(Widget::tabGroup)>();
template void instantiateProp<decltype(Widget::autofocus)>();
template void instantiateProp<decltype(Widget::autoHint)>();
template void instantiateProp<decltype(Widget::squircleCorners)>();
template void instantiateProp<decltype(Widget::delegate)>();
template void instantiateProp<decltype(Widget::hint)>();
template void instantiateProp<decltype(Widget::zorder)>();
template void instantiateProp<decltype(Widget::stylesheet)>();
template void instantiateProp<decltype(Widget::painter)>();
template void instantiateProp<decltype(Widget::isHintExclusive)>();
template void instantiateProp<decltype(Widget::borderRadiusTopLeft)>();
template void instantiateProp<decltype(Widget::borderRadiusTopRight)>();
template void instantiateProp<decltype(Widget::borderRadiusBottomLeft)>();
template void instantiateProp<decltype(Widget::borderRadiusBottomRight)>();
template void instantiateProp<decltype(Widget::width)>();
template void instantiateProp<decltype(Widget::height)>();
template void instantiateProp<decltype(Widget::maxWidth)>();
template void instantiateProp<decltype(Widget::maxHeight)>();
template void instantiateProp<decltype(Widget::minWidth)>();
template void instantiateProp<decltype(Widget::minHeight)>();
template void instantiateProp<decltype(Widget::gapColumn)>();
template void instantiateProp<decltype(Widget::gapRow)>();
template void instantiateProp<decltype(Widget::borderWidthLeft)>();
template void instantiateProp<decltype(Widget::borderWidthTop)>();
template void instantiateProp<decltype(Widget::borderWidthRight)>();
template void instantiateProp<decltype(Widget::borderWidthBottom)>();
template void instantiateProp<decltype(Widget::marginLeft)>();
template void instantiateProp<decltype(Widget::marginTop)>();
template void instantiateProp<decltype(Widget::marginRight)>();
template void instantiateProp<decltype(Widget::marginBottom)>();
template void instantiateProp<decltype(Widget::paddingLeft)>();
template void instantiateProp<decltype(Widget::paddingTop)>();
template void instantiateProp<decltype(Widget::paddingRight)>();
template void instantiateProp<decltype(Widget::paddingBottom)>();
template void instantiateProp<decltype(Widget::fontFeatures)>();
template void instantiateProp<decltype(Widget::scrollBarColor)>();
template void instantiateProp<decltype(Widget::scrollBarThickness)>();
template void instantiateProp<decltype(Widget::scrollBarRadius)>();
template void instantiateProp<decltype(Widget::shadowSpread)>();

inline namespace Arg {

const Argument<Tag::PropArg<decltype(Widget::absolutePosition)>> absolutePosition{};
const Argument<Tag::PropArg<decltype(Widget::alignContent)>> alignContent{};
const Argument<Tag::PropArg<decltype(Widget::alignItems)>> alignItems{};
const Argument<Tag::PropArg<decltype(Widget::alignSelf)>> alignSelf{};
const Argument<Tag::PropArg<decltype(Widget::anchor)>> anchor{};
const Argument<Tag::PropArg<decltype(Widget::aspect)>> aspect{};
const Argument<Tag::PropArg<decltype(Widget::backgroundColorEasing)>> backgroundColorEasing{};
const Argument<Tag::PropArg<decltype(Widget::backgroundColorTransition)>> backgroundColorTransition{};
const Argument<Tag::PropArg<decltype(Widget::backgroundColor)>> backgroundColor{};
const Argument<Tag::PropArg<decltype(Widget::borderColorEasing)>> borderColorEasing{};
const Argument<Tag::PropArg<decltype(Widget::borderColorTransition)>> borderColorTransition{};
const Argument<Tag::PropArg<decltype(Widget::borderColor)>> borderColor{};
const Argument<Tag::PropArg<decltype(Widget::borderRadius)>> borderRadius{};
const Argument<Tag::PropArg<decltype(Widget::borderWidth)>> borderWidth{};
const Argument<Tag::PropArg<decltype(Widget::clip)>> clip{};
const Argument<Tag::PropArg<decltype(Widget::colorEasing)>> colorEasing{};
const Argument<Tag::PropArg<decltype(Widget::colorTransition)>> colorTransition{};
const Argument<Tag::PropArg<decltype(Widget::color)>> color{};
const Argument<Tag::PropArg<decltype(Widget::cursor)>> cursor{};
const Argument<Tag::PropArg<decltype(Widget::dimensions)>> dimensions{};
const Argument<Tag::PropArg<decltype(Widget::flexBasis)>> flexBasis{};
const Argument<Tag::PropArg<decltype(Widget::flexGrow)>> flexGrow{};
const Argument<Tag::PropArg<decltype(Widget::flexShrink)>> flexShrink{};
const Argument<Tag::PropArg<decltype(Widget::flexWrap)>> flexWrap{};
const Argument<Tag::PropArg<decltype(Widget::fontFamily)>> fontFamily{};
const Argument<Tag::PropArg<decltype(Widget::fontSize)>> fontSize{};
const Argument<Tag::PropArg<decltype(Widget::fontStyle)>> fontStyle{};
const Argument<Tag::PropArg<decltype(Widget::fontWeight)>> fontWeight{};
const Argument<Tag::PropArg<decltype(Widget::gap)>> gap{};
const Argument<Tag::PropArg<decltype(Widget::hidden)>> hidden{};
const Argument<Tag::PropArg<decltype(Widget::justifyContent)>> justifyContent{};
const Argument<Tag::PropArg<decltype(Widget::layoutOrder)>> layoutOrder{};
const Argument<Tag::PropArg<decltype(Widget::layout)>> layout{};
const Argument<Tag::PropArg<decltype(Widget::letterSpacing)>> letterSpacing{};
const Argument<Tag::PropArg<decltype(Widget::margin)>> margin{};
const Argument<Tag::PropArg<decltype(Widget::maxDimensions)>> maxDimensions{};
const Argument<Tag::PropArg<decltype(Widget::minDimensions)>> minDimensions{};
const Argument<Tag::PropArg<decltype(Widget::opacity)>> opacity{};
const Argument<Tag::PropArg<decltype(Widget::overflowScrollX)>> overflowScrollX{};
const Argument<Tag::PropArg<decltype(Widget::overflowScrollY)>> overflowScrollY{};
const Argument<Tag::PropArg<decltype(Widget::overflowScroll)>> overflowScroll{};
const Argument<Tag::PropArg<decltype(Widget::contentOverflowX)>> contentOverflowX{};
const Argument<Tag::PropArg<decltype(Widget::contentOverflowY)>> contentOverflowY{};
const Argument<Tag::PropArg<decltype(Widget::contentOverflow)>> contentOverflow{};
const Argument<Tag::PropArg<decltype(Widget::padding)>> padding{};
const Argument<Tag::PropArg<decltype(Widget::placement)>> placement{};
const Argument<Tag::PropArg<decltype(Widget::shadowSize)>> shadowSize{};
const Argument<Tag::PropArg<decltype(Widget::shadowOffset)>> shadowOffset{};
const Argument<Tag::PropArg<decltype(Widget::shadowColor)>> shadowColor{};
const Argument<Tag::PropArg<decltype(Widget::shadowColorTransition)>> shadowColorTransition{};
const Argument<Tag::PropArg<decltype(Widget::shadowColorEasing)>> shadowColorEasing{};
const Argument<Tag::PropArg<decltype(Widget::tabSize)>> tabSize{};
const Argument<Tag::PropArg<decltype(Widget::textAlign)>> textAlign{};
const Argument<Tag::PropArg<decltype(Widget::textVerticalAlign)>> textVerticalAlign{};
const Argument<Tag::PropArg<decltype(Widget::textDecoration)>> textDecoration{};
const Argument<Tag::PropArg<decltype(Widget::translate)>> translate{};
const Argument<Tag::PropArg<decltype(Widget::visible)>> visible{};
const Argument<Tag::PropArg<decltype(Widget::wordSpacing)>> wordSpacing{};
const Argument<Tag::PropArg<decltype(Widget::alignToViewport)>> alignToViewport{};
const Argument<Tag::PropArg<decltype(Widget::stateTriggersRestyle)>> stateTriggersRestyle{};
const Argument<Tag::PropArg<decltype(Widget::id)>> id{};
const Argument<Tag::PropArg<decltype(Widget::role)>> role{};
const Argument<Tag::PropArg<decltype(Widget::classes)>> classes{};
const Argument<Tag::PropArg<decltype(Widget::mouseInteraction)>> mouseInteraction{};
const Argument<Tag::PropArg<decltype(Widget::mousePassThrough)>> mousePassThrough{};
const Argument<Tag::PropArg<decltype(Widget::autoMouseCapture)>> autoMouseCapture{};
const Argument<Tag::PropArg<decltype(Widget::mouseAnywhere)>> mouseAnywhere{};
const Argument<Tag::PropArg<decltype(Widget::focusCapture)>> focusCapture{};
const Argument<Tag::PropArg<decltype(Widget::isHintVisible)>> isHintVisible{};
const Argument<Tag::PropArg<decltype(Widget::tabStop)>> tabStop{};
const Argument<Tag::PropArg<decltype(Widget::tabGroup)>> tabGroup{};
const Argument<Tag::PropArg<decltype(Widget::autofocus)>> autofocus{};
const Argument<Tag::PropArg<decltype(Widget::autoHint)>> autoHint{};
const Argument<Tag::PropArg<decltype(Widget::squircleCorners)>> squircleCorners{};
const Argument<Tag::PropArg<decltype(Widget::delegate)>> delegate{};
const Argument<Tag::PropArg<decltype(Widget::hint)>> hint{};
const Argument<Tag::PropArg<decltype(Widget::zorder)>> zorder{};
const Argument<Tag::PropArg<decltype(Widget::stylesheet)>> stylesheet{};
const Argument<Tag::PropArg<decltype(Widget::painter)>> painter{};
const Argument<Tag::PropArg<decltype(Widget::isHintExclusive)>> isHintExclusive{};
const Argument<Tag::PropArg<decltype(Widget::width)>> width{};
const Argument<Tag::PropArg<decltype(Widget::height)>> height{};
const Argument<Tag::PropArg<decltype(Widget::maxWidth)>> maxWidth{};
const Argument<Tag::PropArg<decltype(Widget::maxHeight)>> maxHeight{};
const Argument<Tag::PropArg<decltype(Widget::minWidth)>> minWidth{};
const Argument<Tag::PropArg<decltype(Widget::minHeight)>> minHeight{};
const Argument<Tag::PropArg<decltype(Widget::gapColumn)>> gapColumn{};
const Argument<Tag::PropArg<decltype(Widget::gapRow)>> gapRow{};
const Argument<Tag::PropArg<decltype(Widget::borderWidthLeft)>> borderWidthLeft{};
const Argument<Tag::PropArg<decltype(Widget::borderWidthTop)>> borderWidthTop{};
const Argument<Tag::PropArg<decltype(Widget::borderWidthRight)>> borderWidthRight{};
const Argument<Tag::PropArg<decltype(Widget::borderWidthBottom)>> borderWidthBottom{};
const Argument<Tag::PropArg<decltype(Widget::borderRadiusTopLeft)>> borderRadiusTopLeft{};
const Argument<Tag::PropArg<decltype(Widget::borderRadiusTopRight)>> borderRadiusTopRight{};
const Argument<Tag::PropArg<decltype(Widget::borderRadiusBottomLeft)>> borderRadiusBottomLeft{};
const Argument<Tag::PropArg<decltype(Widget::borderRadiusBottomRight)>> borderRadiusBottomRight{};
const Argument<Tag::PropArg<decltype(Widget::marginLeft)>> marginLeft{};
const Argument<Tag::PropArg<decltype(Widget::marginTop)>> marginTop{};
const Argument<Tag::PropArg<decltype(Widget::marginRight)>> marginRight{};
const Argument<Tag::PropArg<decltype(Widget::marginBottom)>> marginBottom{};
const Argument<Tag::PropArg<decltype(Widget::paddingLeft)>> paddingLeft{};
const Argument<Tag::PropArg<decltype(Widget::paddingTop)>> paddingTop{};
const Argument<Tag::PropArg<decltype(Widget::paddingRight)>> paddingRight{};
const Argument<Tag::PropArg<decltype(Widget::paddingBottom)>> paddingBottom{};
const Argument<Tag::PropArg<decltype(Widget::fontFeatures)>> fontFeatures{};
const Argument<Tag::PropArg<decltype(Widget::scrollBarColor)>> scrollBarColor;
const Argument<Tag::PropArg<decltype(Widget::scrollBarThickness)>> scrollBarThickness;
const Argument<Tag::PropArg<decltype(Widget::scrollBarRadius)>> scrollBarRadius;
const Argument<Tag::PropArg<decltype(Widget::shadowSpread)>> shadowSpread;

const Argument<Tag::PropArg<decltype(Widget::disabled)>> disabled{};
const Argument<Tag::PropArg<decltype(Widget::selected)>> selected{};

} // namespace Arg

void Widget::setDisabled(bool value) {
    toggleState(WidgetState::Disabled, value);
}

void Widget::setSelected(bool value) {
    toggleState(WidgetState::Selected, value);
}

bool Widget::setScrollOffset(Point newOffset) {
    newOffset = max(newOffset, Point(0));
    newOffset = min(newOffset, Point(scrollSize()));
    return setChildrenOffset(-newOffset);
}

bool Widget::setScrollOffset(Orientation orientation, int newOffset) {
    Point offset         = scrollOffset();
    offset[+orientation] = newOffset;
    return setScrollOffset(offset);
}

Point Widget::scrollOffset() const {
    return -m_childrenOffset;
}

int Widget::scrollOffset(Orientation orientation) const {
    return -m_childrenOffset[+orientation];
}

void Widget::apply(const WidgetActions& action) {
    if (action.onParentSet)
        m_onParentSet.push_back(action.onParentSet);
}

void Widget::invalidate() {
    if (!m_isVisible)
        return;
    if (m_tree) {
        m_tree->invalidateRect(adjustedRect());
        m_tree->invalidateRect(adjustedHintRect());
    }
}

Rectangle Widget::subtreeRect() const noexcept {
    return m_subtreeRect;
}

Rectangle Widget::clipRect() const noexcept {
    return m_clipRect;
}

static Rectangle adjustForShadowSize(RectangleF rect, float shadowSize, PointF shadowOffset,
                                     float shadowSpread) {
    if (rect.empty())
        return Rectangle{};
    RectangleF result = rect.withOffset(shadowOffset).withMargin(std::ceil(shadowSize + shadowSpread + 1.f));
    return result.roundOutward();
}

Rectangle Widget::fullPaintRect() const {
    Rectangle rect = adjustedRect();
    if (m_isHintVisible)
        rect = rect.union_(adjustedHintRect());
    return rect;
}

Rectangle Widget::hintRect() const noexcept {
    return m_hintRect;
}

Rectangle Widget::adjustedRect() const noexcept {
    float shadowSize = m_shadowSize.resolved;
    if (isKeyFocused()) {
        shadowSize = std::max(shadowSize, dp(focusFrameRange.max));
    }
    return adjustForShadowSize(m_rect, shadowSize, scalePixels(m_shadowOffset), scalePixels(m_shadowSpread));
}

Rectangle Widget::adjustedHintRect() const noexcept {
    return adjustForShadowSize(m_hintRect, dp(hintShadowSize), scalePixels(m_shadowOffset),
                               scalePixels(m_shadowSpread));
}

Nullable<InputQueue> Widget::inputQueue() const noexcept {
    return m_tree ? m_tree->inputQueue() : nullptr;
}

bool Widget::hasParent(Widget* parent, bool includePopup) const {
    const Widget* current = this;
    while (current) {
        if (current == parent)
            return true;
        if (current->m_zorder != ZOrder::Normal && !includePopup)
            return false;
        current = current->m_parent ? current->m_parent : nullptr;
    }
    return false;
}

void Widget::bubble(function_ref<bool(Widget*)> fn, bool includePopup) {
    Rc<Widget> current = this->shared_from_this();
    while (current) {
        if (!fn(current.get()))
            return;
        if (current->m_zorder != ZOrder::Normal && !includePopup)
            return;
        current = current->m_parent ? current->m_parent->shared_from_this() : nullptr;
    }
}
} // namespace Brisk
