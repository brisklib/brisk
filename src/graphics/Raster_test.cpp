#include "brisk/graphics/Path.hpp"
#include "vector/Raster.hpp"
#include "Catch2Utils.hpp"

namespace Brisk {

TEST_CASE("Rasterizer: Rasterize empty path") {
    Path path;
    Rle rle = rasterize(path);
    CHECK(rle.empty());
    CHECK(rle.boundingRect().empty());
}

TEST_CASE("Rasterizer: Rasterize path with line") {
    Path path;
    path.moveTo(PointF(10, 10));
    path.lineTo(PointF(20, 20));
    Rle rle = rasterize(path, CapStyle::Square, JoinStyle::Miter, 1.0f, 4.0f);
    CHECK(!rle.empty());
    CHECK(rle.boundingRect() == Rectangle(9, 9, 21, 21));
}

TEST_CASE("Rasterizer: Rasterize path with rectangle") {
    Path path;
    path.addRect({ 0, 0, 2, 2 });
    Rle rle = rasterize(path);
    CHECK(!rle.empty());
    CHECK(rle.boundingRect() == Rectangle(0, 0, 2, 2));
    REQUIRE(rle.spans().size() == 2);
    CHECK(rle.spans()[0] == Rle::Span{ 0, 0, 2, 255 });
    CHECK(rle.spans()[1] == Rle::Span{ 0, 1, 2, 255 });
}

TEST_CASE("Rasterizer: Rle binary operations") {
    Path path;
    path.addRect({ 0, 0, 2, 2 });
    Rle rle1 = rasterize(path);
    path.reset();
    path.addRect({ 1, 1, 2, 2 });
    Rle rle2      = rasterize(path);

    Rle andResult = Rle::binary(rle1, rle2, Rle::BinOp::And);
    fmt::println("And Result:\n    {}", fmt::join(andResult.view(), "\n    "));
    CHECK(!andResult.empty());
    CHECK(andResult.boundingRect() == Rectangle(1, 1, 2, 2));
    REQUIRE(andResult.spans().size() == 1);
    CHECK(andResult.spans()[0] == Rle::Span{ 1, 1, 1, 255 });

    Rle andNotResult = Rle::binary(rle1, rle2, Rle::BinOp::AndNot);
    fmt::println("And Not Result:\n    {}", fmt::join(andNotResult.view(), "\n    "));
    CHECK(!andNotResult.empty());
    CHECK(andNotResult.boundingRect() == Rectangle(0, 0, 2, 2));
    REQUIRE(andNotResult.spans().size() == 2);
    CHECK(andNotResult.spans()[0] == Rle::Span{ 0, 0, 2, 255 });
    CHECK(andNotResult.spans()[1] == Rle::Span{ 0, 1, 1, 255 });

    Rle orResult = Rle::binary(rle1, rle2, Rle::BinOp::Or);
    fmt::println("Or Result:\n    {}", fmt::join(orResult.view(), "\n    "));
    CHECK(!orResult.empty());
    CHECK(orResult.boundingRect() == Rectangle(0, 0, 3, 3));
    REQUIRE(orResult.spans().size() == 3);
    CHECK(orResult.spans()[0] == Rle::Span{ 0, 0, 2, 255 });
    CHECK(orResult.spans()[1] == Rle::Span{ 0, 1, 3, 255 });
    CHECK(orResult.spans()[2] == Rle::Span{ 1, 2, 2, 255 });

    Rle xorResult = Rle::binary(rle1, rle2, Rle::BinOp::Xor);
    fmt::println("Xor Result:\n    {}", fmt::join(xorResult.view(), "\n    "));
    CHECK(!xorResult.empty());
    CHECK(xorResult.boundingRect() == Rectangle(0, 0, 3, 3));
    REQUIRE(xorResult.spans().size() == 4);
    CHECK(xorResult.spans()[0] == Rle::Span{ 0, 0, 2, 255 });
    CHECK(xorResult.spans()[1] == Rle::Span{ 0, 1, 1, 255 });
    CHECK(xorResult.spans()[2] == Rle::Span{ 2, 1, 1, 255 });
    CHECK(xorResult.spans()[3] == Rle::Span{ 1, 2, 2, 255 });
}
} // namespace Brisk
