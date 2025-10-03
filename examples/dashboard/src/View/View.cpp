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
#include "View.hpp"
#include <brisk/widgets/Widgets.hpp>
#include <brisk/graphics/Palette.hpp>
#include <deque>

namespace App {

class Plot final : public Widget {
    BRISK_DYNAMIC_CLASS(Plot, Widget)
public:
    template <WidgetArgument... Args>
    Plot(const Args&... args) : Plot{ Construction{ "plot" }, std::tuple{ args... } } {
        endConstruction();
    }

    void addValue(double value, size_t maxValues) {
        m_values.push_back(value);
        while (m_values.size() > maxValues)
            m_values.pop_front();

        invalidate();
    }

protected:
    Plot(Construction c, ArgumentsView<Plot> args) : Widget{ Construction{ "plot" }, nullptr } {
        args.apply(this);
    }

    std::deque<double> m_values;
    ColorW m_lineColor;

    void paint(Canvas& canvas) const override {
        canvas.setFillColor(m_lineColor.multiplyAlpha(0.1f));
        canvas.fillRect(m_rect);
        canvas.setStrokeColor(m_lineColor);
        canvas.setStrokeWidth(1_dp);
        Path path;
        for (size_t i = 0; i < m_values.size(); ++i) {
            PointF p(-dp(m_values.size() - 1 - i), (1 - m_values[i]) * m_rect.height());
            if (i == 0)
                path.moveTo(p);
            else
                path.lineTo(p);
        }
        path.lineTo(PointF(0, m_rect.height()));
        path.lineTo(PointF(-dp(m_values.size() - 1), m_rect.height()));
        path.close();
        path.transform(Matrix::translation(m_rect.x2, m_rect.y1));
        auto&& clipRect = canvas.saveScissor();
        *clipRect       = clipRect->intersection(m_rect); // New clip rect
        canvas.setFillColor(m_lineColor.multiplyAlpha(0.3f));
        canvas.drawPath(std::move(path));

        canvas.setFont(font());
        canvas.setFillColor(Palette::white.multiplyAlpha(0.75f));
        Rectangle rect = m_rect.withPadding(3_idp, -2_idp);
        canvas.fillText("100%", rect.at(1.f, 0.f), { 1.f, 0.f });
        canvas.fillText("0%", rect.at(1.f, 1.f), { 1.f, 1.f });
    }

private:
public:
    static const auto& properties() noexcept {
        static constexpr tuplet::tuple props{
            Internal::PropField{ &Plot::m_lineColor, "lineColor" },
        };
        return props;
    }

public:
    BRISK_PROPERTIES_BEGIN
    Property<Plot, ColorW, 0> lineColor;
    BRISK_PROPERTIES_END
};

inline namespace Arg {
constexpr inline PropArgument<decltype(Plot::lineColor)> lineColor{};
}

static Rc<Widget> plot(Value<Trigger<>> updated, Value<Normalized> value, ColorW color, Value<bool> showPlot,
                       size_t capacity = 200) {
    Rc<Plot> plot = rcnew Plot{
        lineColor = color,
        fontSize  = 75_perc,
        aspect    = 1.f,
        visible   = showPlot,
    };
    bindings->listen(
        updated, plot->lifetime() | [plot, value, capacity]() {
            plot->addValue(value.get(), capacity);
        });
    return plot;
}

static Rc<Widget> dataEntryView(Rc<DataSourceViewModel> viewModel, int index, Value<bool> showPlots) {
    return rcnew Widget{
        backgroundColor = 0x2D313D_rgb,
        shadowColor     = 0x000000'30_rgba,
        shadowSize      = 9,
        layout          = Layout::Vertical,
        padding         = 8_apx,
        gap             = 10_apx,
        minWidth        = 50_apx,
        flexBasis       = 5_em,
        textAlign       = TextAlign::Center,
        rcnew Text{
            text = viewModel->label(index),
        },
        rcnew Text{
            fontFamily = Font::Monospace,
            text       = viewModel->value(index).transform([](double v) {
                return fmt::format("{:5.1f}%", std::clamp(v * 100.0, 0., 100.));
            }),
        },
        plot(viewModel->updated(), viewModel->value(index), Palette::Standard::index(index), showPlots),
    };
}

Rc<Widget> dataView(Rc<DataSourceViewModel> viewModel, Value<bool> showPlots) {
    return rcnew Widget{
        gap              = 8_apx,
        layout           = Layout::Vertical,
        flexGrow         = 1,
        contentOverflowX = ContentOverflow::Allow,
        rcnew Text{
            viewModel->caption(),
            fontWeight = FontWeight::Bold,
            marginLeft = 16_apx,
        },
        rcnew Widget{
            contentOverflowY = ContentOverflow::Allow,
            overflowScrollY  = OverflowScroll::Enable,
            flexGrow         = 1,
            padding          = 8_apx,
            gap              = 8_apx,
            layout           = Layout::Horizontal,
            flexWrap         = Wrap::Wrap,
            Builder{
                [viewModel, showPlots](Widget* w) {
                    for (int i = 0; i < viewModel->count(); ++i)
                        w->apply(dataEntryView(viewModel, i, showPlots));
                },
            },
        },
    };
}
} // namespace App
