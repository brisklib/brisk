/*
 * Brisk
 *
 * Copyright (C) 2025 Brisk Developers
 *
 * This file is part of the Brisk library.
 */
#include <brisk/graphics/Fonts.hpp>
#include <brisk/core/Io.hpp>
#include <catch2/catch_all.hpp>

namespace Brisk {

TEST_CASE("FontManager document preparation is canonical", "[text-layout]") {
    REQUIRE(fonts.has_value());
    const auto fontData = readBytes(fs::path(PROJECT_SOURCE_DIR) / "resources" / "fonts" / "Lato-Medium.ttf");
    REQUIRE(fontData.has_value());
    Internal::registerTextLayoutFont(*fonts, *fontData, "FontsTest");

    const PreparedDocument document =
        fonts->prepareDocument(Font{ "FontsTest", 20.f }, TextWithOptions{ U"Hello, world!" });
    const DocumentLayout layout = document.layout();
    REQUIRE_FALSE(document.empty());
    REQUIRE_FALSE(layout.empty());
    REQUIRE(layout.bounds().width() > 0.f);
}

} // namespace Brisk
