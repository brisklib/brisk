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
#pragma once

#include <brisk/gui/Gui.hpp>
#include <brisk/graphics/Svg.hpp>
#include <brisk/graphics/ImageFormats.hpp>

namespace Brisk {

class WIDGET ImageView : public Widget {
    BRISK_DYNAMIC_CLASS(ImageView, Widget)
public:
    using Base                                   = Widget;
    constexpr static std::string_view widgetType = "imageview";

    template <WidgetArgument... Args>
    ImageView(BytesView image, const Args&... args)
        : ImageView(Construction{ widgetType }, imageDecode(image, ImageFormat::RGBA).value(),
                    std::tuple{ args... }) {
        endConstruction();
    }

    template <WidgetArgument... Args>
    ImageView(Rc<Image> image, const Args&... args)
        : ImageView(Construction{ widgetType }, std::move(image), std::tuple{ args... }) {
        endConstruction();
    }

protected:
    Rc<Image> m_image;

    void paint(Canvas& canvas) const override;
    Ptr cloneThis() const override;

    ImageView(Construction construction, Rc<Image> image, ArgumentsView<ImageView> args);
};

class WIDGET SvgImageView final : public Widget {
    BRISK_DYNAMIC_CLASS(SvgImageView, Widget)
public:
    using Base                                   = Widget;
    constexpr static std::string_view widgetType = "svgimageview";

    template <WidgetArgument... Args>
    SvgImageView(SvgImage svg, const Args&... args)
        : Widget(Construction{ widgetType }, std::tuple{ args... }), m_svg(std::move(svg)) {
        endConstruction();
    }

    template <WidgetArgument... Args>
    SvgImageView(std::string_view svg, const Args&... args) : SvgImageView(SvgImage(svg), args...) {}

protected:
    SvgImage m_svg;
    mutable Rc<Image> m_image;

    void paint(Canvas& canvas) const override;
    Ptr cloneThis() const override;

public:

    static const auto& properties() noexcept {
        static constexpr tuplet::tuple props{
            /*0*/ Internal::PropField{ &SvgImageView::m_svg, "svg" },
        };
        return props;
    }

public:
    BRISK_PROPERTIES_BEGIN
    Property<SvgImageView, SvgImage, 0> svg;
    BRISK_PROPERTIES_END
};

} // namespace Brisk
