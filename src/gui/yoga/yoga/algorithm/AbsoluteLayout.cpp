/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <yoga/algorithm/AbsoluteLayout.h>
#include <yoga/algorithm/Align.h>
#include <yoga/algorithm/BoundAxis.h>
#include <yoga/algorithm/CalculateLayout.h>
#include <yoga/algorithm/TrailingPosition.h>

namespace facebook::yoga {

static inline void setFlexStartLayoutPosition(
    const yoga::Node* const parent,
    yoga::Node* child,
    const Direction direction,
    const FlexDirection axis,
    const float containingBlockWidth) {
  child->setLayoutPosition(
      child->style().computeFlexStartMargin(
          axis, direction, containingBlockWidth) +
          parent->getLayout().border(flexStartEdge(axis)) +
          parent->getLayout().padding(flexStartEdge(axis)),
      flexStartEdge(axis));
}

static inline void setFlexEndLayoutPosition(
    const yoga::Node* const parent,
    yoga::Node* child,
    const Direction direction,
    const FlexDirection axis,
    const float containingBlockWidth) {
  child->setLayoutPosition(
      getPositionOfOppositeEdge(
          parent->getLayout().border(flexEndEdge(axis)) +
              parent->getLayout().padding(flexEndEdge(axis)) +
              child->style().computeFlexEndMargin(
                  axis, direction, containingBlockWidth),
          axis,
          parent,
          child),
      flexStartEdge(axis));
}

static inline void setCenterLayoutPosition(
    const yoga::Node* const parent,
    yoga::Node* child,
    const Direction direction,
    const FlexDirection axis,
    const float containingBlockWidth) {
  const float parentContentBoxSize =
      parent->getLayout().measuredDimension(dimension(axis)) -
      parent->getLayout().border(flexStartEdge(axis)) -
      parent->getLayout().border(flexEndEdge(axis)) -
      parent->getLayout().padding(flexStartEdge(axis)) -
      parent->getLayout().padding(flexEndEdge(axis));
  const float childOuterSize =
      child->getLayout().measuredDimension(dimension(axis)) +
      child->style().computeMarginForAxis(axis, containingBlockWidth);
  child->setLayoutPosition(
      (parentContentBoxSize - childOuterSize) / 2.0f +
          parent->getLayout().border(flexStartEdge(axis)) +
          parent->getLayout().padding(flexStartEdge(axis)) +
          child->style().computeFlexStartMargin(
              axis, direction, containingBlockWidth),
      flexStartEdge(axis));
}

static void justifyAbsoluteChild(
    const yoga::Node* const parent,
    yoga::Node* child,
    const Direction direction,
    const FlexDirection mainAxis,
    const float containingBlockWidth) {
  const Justify parentJustifyContent = parent->style().justifyContent();
  switch (parentJustifyContent) {
    case Justify::FlexStart:
    case Justify::SpaceBetween:
      setFlexStartLayoutPosition(
          parent, child, direction, mainAxis, containingBlockWidth);
      break;
    case Justify::FlexEnd:
      setFlexEndLayoutPosition(
          parent, child, direction, mainAxis, containingBlockWidth);
      break;
    case Justify::Center:
    case Justify::SpaceAround:
    case Justify::SpaceEvenly:
      setCenterLayoutPosition(
          parent, child, direction, mainAxis, containingBlockWidth);
      break;
  }
}

static void alignAbsoluteChild(
    const yoga::Node* const parent,
    yoga::Node* child,
    const Direction direction,
    const FlexDirection crossAxis,
    const float containingBlockWidth) {
  Align itemAlign = resolveChildAlignment(parent, child);
  const Wrap parentWrap = parent->style().flexWrap();
  if (parentWrap == Wrap::WrapReverse) {
    if (itemAlign == Align::FlexEnd) {
      itemAlign = Align::FlexStart;
    } else if (itemAlign != Align::Center) {
      itemAlign = Align::FlexEnd;
    }
  }

  switch (itemAlign) {
    case Align::Auto:
    case Align::FlexStart:
    case Align::Baseline:
    case Align::SpaceAround:
    case Align::SpaceBetween:
    case Align::Stretch:
    case Align::SpaceEvenly:
      setFlexStartLayoutPosition(
          parent, child, direction, crossAxis, containingBlockWidth);
      break;
    case Align::FlexEnd:
      setFlexEndLayoutPosition(
          parent, child, direction, crossAxis, containingBlockWidth);
      break;
    case Align::Center:
      setCenterLayoutPosition(
          parent, child, direction, crossAxis, containingBlockWidth);
      break;
  }
}

// To ensure no breaking changes, we preserve the legacy way of positioning
// absolute children and determine if we should use it using an errata.
static void positionAbsoluteChildLegacy(
    const yoga::Node* const containingNode,
    const yoga::Node* const parent,
    yoga::Node* child,
    const Direction direction,
    const FlexDirection axis,
    const bool isMainAxis,
    const float containingBlockWidth,
    const float containingBlockHeight) {
  const bool isAxisRow = isRow(axis);
  const bool shouldCenter = isMainAxis
      ? parent->style().justifyContent() == Justify::Center
      : resolveChildAlignment(parent, child) == Align::Center;
  const bool shouldFlexEnd = isMainAxis
      ? parent->style().justifyContent() == Justify::FlexEnd
      : ((resolveChildAlignment(parent, child) == Align::FlexEnd) ^
         (parent->style().flexWrap() == Wrap::WrapReverse));

  if (child->style().isFlexEndPositionDefined(axis, direction) &&
      !child->style().isFlexStartPositionDefined(axis, direction)) {
    child->setLayoutPosition(
        containingNode->getLayout().measuredDimension(dimension(axis)) -
            child->getLayout().measuredDimension(dimension(axis)) -
            containingNode->style().computeFlexEndBorder(axis, direction) -
            child->style().computeFlexEndMargin(
                axis,
                direction,
                isAxisRow ? containingBlockWidth : containingBlockHeight) -
            child->style().computeFlexEndPosition(
                axis,
                direction,
                isAxisRow ? containingBlockWidth : containingBlockHeight),
        flexStartEdge(axis));
  } else if (
      !child->style().isFlexStartPositionDefined(axis, direction) &&
      shouldCenter) {
    child->setLayoutPosition(
        (parent->getLayout().measuredDimension(dimension(axis)) -
         child->getLayout().measuredDimension(dimension(axis))) /
            2.0f,
        flexStartEdge(axis));
  } else if (
      !child->style().isFlexStartPositionDefined(axis, direction) &&
      shouldFlexEnd) {
    child->setLayoutPosition(
        (parent->getLayout().measuredDimension(dimension(axis)) -
         child->getLayout().measuredDimension(dimension(axis))),
        flexStartEdge(axis));
  }
}

/*
 * Absolutely positioned nodes do not participate in flex layout and thus their
 * positions can be determined independently from the rest of their siblings.
 * For each axis there are essentially two cases:
 *
 * 1) The node has insets defined. In this case we can just use these to
 *    determine the position of the node.
 * 2) The node does not have insets defined. In this case we look at the style
 *    of the parent to position the node. Things like justify content and
 *    align content will move absolute children around. If none of these
 *    special properties are defined, the child is positioned at the start
 *    (defined by flex direction) of the leading flex line.
 *
 * This function does that positioning for the given axis. The spec has more
 * information on this topic: https://www.w3.org/TR/css-flexbox-1/#abspos-items
 */
static void positionAbsoluteChildImpl(
    const yoga::Node* const containingNode,
    const yoga::Node* const parent,
    yoga::Node* child,
    const Direction direction,
    const FlexDirection axis,
    const bool isMainAxis,
    const float containingBlockWidth,
    const float containingBlockHeight) {
  const bool isAxisRow = isRow(axis);
  const float containingBlockSize =
      isAxisRow ? containingBlockWidth : containingBlockHeight;

  // The inline-start position takes priority over the end position in the case
  // that they are both set and the node has a fixed width. Thus we only have 2
  // cases here: if inline-start is defined and if inline-end is defined.
  //
  // Despite checking inline-start to honor prioritization of insets, we write
  // to the flex-start edge because this algorithm works by positioning on the
  // flex-start edge and then filling in the flex-end direction at the end if
  // necessary.
  if (child->style().isInlineStartPositionDefined(axis, direction)) {
    const float positionRelativeToInlineStart =
        child->style().computeInlineStartPosition(
            axis, direction, containingBlockSize) +
        containingNode->style().computeInlineStartBorder(axis, direction) +
        child->style().computeInlineStartMargin(
            axis, direction, containingBlockSize);
    const float positionRelativeToFlexStart =
        inlineStartEdge(axis, direction) != flexStartEdge(axis)
        ? getPositionOfOppositeEdge(
              positionRelativeToInlineStart, axis, containingNode, child)
        : positionRelativeToInlineStart;

    child->setLayoutPosition(positionRelativeToFlexStart, flexStartEdge(axis));
  } else if (child->style().isInlineEndPositionDefined(axis, direction)) {
    const float positionRelativeToInlineStart =
        containingNode->getLayout().measuredDimension(dimension(axis)) -
        child->getLayout().measuredDimension(dimension(axis)) -
        containingNode->style().computeInlineEndBorder(axis, direction) -
        child->style().computeInlineEndMargin(
            axis, direction, containingBlockSize) -
        child->style().computeInlineEndPosition(
            axis, direction, containingBlockSize);
    const float positionRelativeToFlexStart =
        inlineStartEdge(axis, direction) != flexStartEdge(axis)
        ? getPositionOfOppositeEdge(
              positionRelativeToInlineStart, axis, containingNode, child)
        : positionRelativeToInlineStart;

    child->setLayoutPosition(positionRelativeToFlexStart, flexStartEdge(axis));
  } else {
    isMainAxis ? justifyAbsoluteChild(
                     parent, child, direction, axis, containingBlockWidth)
               : alignAbsoluteChild(
                     parent, child, direction, axis, containingBlockWidth);
  }
}

static void positionAbsoluteChild(
    const yoga::Node* const containingNode,
    const yoga::Node* const parent,
    yoga::Node* child,
    const Direction direction,
    const FlexDirection axis,
    const bool isMainAxis,
    const float containingBlockWidth,
    const float containingBlockHeight) {
  hasErrata(Errata::AbsolutePositioningIncorrect) ? positionAbsoluteChildLegacy(
                                                        containingNode,
                                                        parent,
                                                        child,
                                                        direction,
                                                        axis,
                                                        isMainAxis,
                                                        containingBlockWidth,
                                                        containingBlockHeight)
                                                  : positionAbsoluteChildImpl(
                                                        containingNode,
                                                        parent,
                                                        child,
                                                        direction,
                                                        axis,
                                                        isMainAxis,
                                                        containingBlockWidth,
                                                        containingBlockHeight);
}

void layoutAbsoluteChild(
    const yoga::Node* const containingNode,
    const yoga::Node* const node,
    yoga::Node* const child,
    const float containingBlockWidth,
    const float containingBlockHeight,
    const SizingMode widthMode,
    const Direction direction,
    const uint32_t depth,
    const uint32_t generationCount) {
  const FlexDirection mainAxis =
      resolveDirection(node->style().flexDirection(), direction);
  const FlexDirection crossAxis = resolveCrossDirection(mainAxis, direction);
  const bool isMainAxisRow = isRow(mainAxis);

  float childWidth = undefined;
  float childHeight = undefined;
  SizingMode childWidthSizingMode = SizingMode::MaxContent;
  SizingMode childHeightSizingMode = SizingMode::MaxContent;

  auto marginRow = child->style().computeMarginForAxis(
      FlexDirection::Row, containingBlockWidth);
  auto marginColumn = child->style().computeMarginForAxis(
      FlexDirection::Column, containingBlockWidth);

  if (child->hasDefiniteLength(Dimension::Width, containingBlockWidth)) {
    childWidth = child->getResolvedDimension(Dimension::Width)
                     .resolve(containingBlockWidth)
                     .unwrap() +
        marginRow;
  } else {
    // If the child doesn't have a specified width, compute the width based on
    // the left/right offsets if they're defined.
    if (child->style().isFlexStartPositionDefined(
            FlexDirection::Row, direction) &&
        child->style().isFlexEndPositionDefined(
            FlexDirection::Row, direction)) {
      childWidth =
          containingNode->getLayout().measuredDimension(Dimension::Width) -
          (containingNode->style().computeFlexStartBorder(
               FlexDirection::Row, direction) +
           containingNode->style().computeFlexEndBorder(
               FlexDirection::Row, direction)) -
          (child->style().computeFlexStartPosition(
               FlexDirection::Row, direction, containingBlockWidth) +
           child->style().computeFlexEndPosition(
               FlexDirection::Row, direction, containingBlockWidth));
      childWidth = boundAxis(
          child,
          FlexDirection::Row,
          childWidth,
          containingBlockWidth,
          containingBlockWidth);
    }
  }

  if (child->hasDefiniteLength(Dimension::Height, containingBlockHeight)) {
    childHeight = child->getResolvedDimension(Dimension::Height)
                      .resolve(containingBlockHeight)
                      .unwrap() +
        marginColumn;
  } else {
    // If the child doesn't have a specified height, compute the height based
    // on the top/bottom offsets if they're defined.
    if (child->style().isFlexStartPositionDefined(
            FlexDirection::Column, direction) &&
        child->style().isFlexEndPositionDefined(
            FlexDirection::Column, direction)) {
      childHeight =
          containingNode->getLayout().measuredDimension(Dimension::Height) -
          (containingNode->style().computeFlexStartBorder(
               FlexDirection::Column, direction) +
           containingNode->style().computeFlexEndBorder(
               FlexDirection::Column, direction)) -
          (child->style().computeFlexStartPosition(
               FlexDirection::Column, direction, containingBlockHeight) +
           child->style().computeFlexEndPosition(
               FlexDirection::Column, direction, containingBlockHeight));
      childHeight = boundAxis(
          child,
          FlexDirection::Column,
          childHeight,
          containingBlockHeight,
          containingBlockWidth);
    }
  }

  // Exactly one dimension needs to be defined for us to be able to do aspect
  // ratio calculation. One dimension being the anchor and the other being
  // flexible.
  const auto& childStyle = child->style();
  if (yoga::isUndefined(childWidth) ^ yoga::isUndefined(childHeight)) {
    if (childStyle.aspectRatio().isDefined()) {
      if (yoga::isUndefined(childWidth)) {
        childWidth = marginRow +
            (childHeight - marginColumn) * childStyle.aspectRatio().unwrap();
      } else if (yoga::isUndefined(childHeight)) {
        childHeight = marginColumn +
            (childWidth - marginRow) / childStyle.aspectRatio().unwrap();
      }
    }
  }

  // If we're still missing one or the other dimension, measure the content.
  if (yoga::isUndefined(childWidth) || yoga::isUndefined(childHeight)) {
    childWidthSizingMode = yoga::isUndefined(childWidth)
        ? SizingMode::MaxContent
        : SizingMode::StretchFit;
    childHeightSizingMode = yoga::isUndefined(childHeight)
        ? SizingMode::MaxContent
        : SizingMode::StretchFit;

    // If the size of the owner is defined then try to constrain the absolute
    // child to that size as well. This allows text within the absolute child
    // to wrap to the size of its owner. This is the same behavior as many
    // browsers implement.
    if (!isMainAxisRow && yoga::isUndefined(childWidth) &&
        widthMode != SizingMode::MaxContent &&
        yoga::isDefined(containingBlockWidth) && containingBlockWidth > 0) {
      childWidth = containingBlockWidth;
      childWidthSizingMode = SizingMode::FitContent;
    }

    calculateLayoutInternal(
        child,
        childWidth,
        childHeight,
        direction,
        childWidthSizingMode,
        childHeightSizingMode,
        containingBlockWidth,
        containingBlockHeight,
        false,
        depth,
        generationCount);
    childWidth = child->getLayout().measuredDimension(Dimension::Width) +
        child->style().computeMarginForAxis(
            FlexDirection::Row, containingBlockWidth);
    childHeight = child->getLayout().measuredDimension(Dimension::Height) +
        child->style().computeMarginForAxis(
            FlexDirection::Column, containingBlockWidth);
  }

  calculateLayoutInternal(
      child,
      childWidth,
      childHeight,
      direction,
      SizingMode::StretchFit,
      SizingMode::StretchFit,
      containingBlockWidth,
      containingBlockHeight,
      true,
      depth,
      generationCount);

  positionAbsoluteChild(
      containingNode,
      node,
      child,
      direction,
      mainAxis,
      true /*isMainAxis*/,
      containingBlockWidth,
      containingBlockHeight);
  positionAbsoluteChild(
      containingNode,
      node,
      child,
      direction,
      crossAxis,
      false /*isMainAxis*/,
      containingBlockWidth,
      containingBlockHeight);
}

void layoutAbsoluteDescendants(
    yoga::Node* containingNode,
    yoga::Node* currentNode,
    SizingMode widthSizingMode,
    Direction currentNodeDirection,
    uint32_t currentDepth,
    uint32_t generationCount,
    float currentNodeLeftOffsetFromContainingBlock,
    float currentNodeTopOffsetFromContainingBlock,
    float containingNodeAvailableInnerWidth,
    float containingNodeAvailableInnerHeight) {
  for (auto child : currentNode->getChildren()) {
    if (child->style().display() == Display::None) {
      continue;
    } else if (child->style().positionType() == PositionType::Absolute) {
      const bool absoluteErrata =
          hasErrata(Errata::AbsolutePercentAgainstInnerSize);
      const float containingBlockWidth = absoluteErrata
          ? containingNodeAvailableInnerWidth
          : containingNode->getLayout().measuredDimension(Dimension::Width) -
              containingNode->style().computeBorderForAxis(FlexDirection::Row);
      const float containingBlockHeight = absoluteErrata
          ? containingNodeAvailableInnerHeight
          : containingNode->getLayout().measuredDimension(Dimension::Height) -
              containingNode->style().computeBorderForAxis(
                  FlexDirection::Column);

      layoutAbsoluteChild(
          containingNode,
          currentNode,
          child,
          containingBlockWidth,
          containingBlockHeight,
          widthSizingMode,
          currentNodeDirection,
          currentDepth,
          generationCount);

      /*
       * At this point the child has its position set but only on its the
       * parent's flexStart edge. Additionally, this position should be
       * interpreted relative to the containing block of the child if it had
       * insets defined. So we need to adjust the position by subtracting the
       * the parents offset from the containing block. However, getting that
       * offset is complicated since the two nodes can have different main/cross
       * axes.
       */
      const FlexDirection parentMainAxis = resolveDirection(
          currentNode->style().flexDirection(), currentNodeDirection);
      const FlexDirection parentCrossAxis =
          resolveCrossDirection(parentMainAxis, currentNodeDirection);

      if (needsTrailingPosition(parentMainAxis)) {
        const bool mainInsetsDefined = isRow(parentMainAxis)
            ? child->style().horizontalInsetsDefined()
            : child->style().verticalInsetsDefined();
        setChildTrailingPosition(
            mainInsetsDefined ? containingNode : currentNode,
            child,
            parentMainAxis);
      }
      if (needsTrailingPosition(parentCrossAxis)) {
        const bool crossInsetsDefined = isRow(parentCrossAxis)
            ? child->style().horizontalInsetsDefined()
            : child->style().verticalInsetsDefined();
        setChildTrailingPosition(
            crossInsetsDefined ? containingNode : currentNode,
            child,
            parentCrossAxis);
      }

      /*
       * At this point we know the left and top physical edges of the child are
       * set with positions that are relative to the containing block if insets
       * are defined
       */
      const float childLeftPosition =
          child->getLayout().position(PhysicalEdge::Left);
      const float childTopPosition =
          child->getLayout().position(PhysicalEdge::Top);

      const float childLeftOffsetFromParent =
          child->style().horizontalInsetsDefined()
          ? (childLeftPosition - currentNodeLeftOffsetFromContainingBlock)
          : childLeftPosition;
      const float childTopOffsetFromParent =
          child->style().verticalInsetsDefined()
          ? (childTopPosition - currentNodeTopOffsetFromContainingBlock)
          : childTopPosition;

      child->setLayoutPosition(childLeftOffsetFromParent, PhysicalEdge::Left);
      child->setLayoutPosition(childTopOffsetFromParent, PhysicalEdge::Top);
    } else if (
        child->style().positionType() == PositionType::Static &&
        !child->alwaysFormsContainingBlock()) {
      const Direction childDirection =
          child->resolveDirection(currentNodeDirection);
      // By now all descendants of the containing block that are not absolute
      // will have their positions set for left and top.
      const float childLeftOffsetFromContainingBlock =
          currentNodeLeftOffsetFromContainingBlock +
          child->getLayout().position(PhysicalEdge::Left);
      const float childTopOffsetFromContainingBlock =
          currentNodeTopOffsetFromContainingBlock +
          child->getLayout().position(PhysicalEdge::Top);

      layoutAbsoluteDescendants(
          containingNode,
          child,
          widthSizingMode,
          childDirection,
          currentDepth + 1,
          generationCount,
          childLeftOffsetFromContainingBlock,
          childTopOffsetFromContainingBlock,
          containingNodeAvailableInnerWidth,
          containingNodeAvailableInnerHeight);
    }
  }
}
} // namespace facebook::yoga
