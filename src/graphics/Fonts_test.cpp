/*
 * Brisk
 *
 * Copyright (C) 2025 Brisk Developers
 *
 * This file is part of the Brisk library.
 */
#include <catch2/catch_all.hpp>

#include <brisk/core/Io.hpp>
#include <brisk/graphics/Fonts.hpp>

namespace Brisk {

TEST_CASE("FontManager document preparation is canonical", "[text-layout]") {
    REQUIRE(fonts.has_value());
    REQUIRE(fonts
                ->addFontFromFile(fs::path(PROJECT_SOURCE_DIR) / "resources" / "fonts" / "Lato-Medium.ttf",
                                  "FontsTest")
                .has_value());

    const ShapedText document =
        fonts->shapeText(Font{ "FontsTest", 20.f }, TextWithOptions{ U"Hello, world!" });
    const TextLayout layout = document.layout();
    REQUIRE_FALSE(document.empty());
    REQUIRE_FALSE(layout.empty());
    REQUIRE(layout.bounds().width() > 0.f);
}

} // namespace Brisk
