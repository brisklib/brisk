/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

// @generated by enums.py
// clang-format off
#pragma once

#include <cstdint>
#include <yoga/enums/YogaEnums.h>

namespace facebook::yoga {

enum class Display : uint8_t {
  Flex,
  None,
};

template <>
constexpr int32_t ordinalCount<Display>() {
  return 2;
}

const char* toString(Display e);
} // namespace facebook::yoga
