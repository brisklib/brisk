/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <yoga/algorithm/SizingMode.h>

namespace facebook::yoga {

bool canUseCachedMeasurement(
    SizingMode widthMode,
    float availableWidth,
    SizingMode heightMode,
    float availableHeight,
    SizingMode lastWidthMode,
    float lastAvailableWidth,
    SizingMode lastHeightMode,
    float lastAvailableHeight,
    float lastComputedWidth,
    float lastComputedHeight,
    float marginRow,
    float marginColumn);

} // namespace facebook::yoga
