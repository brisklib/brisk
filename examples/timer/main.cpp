#include <brisk/core/Text.hpp>
#include <brisk/gui/GuiApplication.hpp>
#include <brisk/widgets/Widgets.hpp>
#include <brisk/window/OsDialogs.hpp>
#include <brisk/gui/Component.hpp>
#include <brisk/graphics/Palette.hpp>

namespace Brisk {

class Timer final : public Component {
public:
    using Component::Component;

    void reset() {
        m_start = currentTime();
    }

    Rc<Widget> build() final {
        return rcnew VLayout{
            stylesheet = Graphene::stylesheet(),
            Graphene::darkColors(),
            alignItems = Align::Stretch,

            minWidth   = 250_px,

            padding    = 1_em,
            gapRow     = 1_em,

            rcnew HLayout{
                "Elapsed time:"_Text,
                gapColumn = 0.5_em,
                rcnew Progress{
                    minimum  = 0,
                    maximum  = 1,
                    flexGrow = 1,
                    value    = transform(
                        [](double startTime, double duration, double nowTime) -> double {
                            return duration < 0.001 ? 1 : (nowTime - startTime) / duration;
                        },
                        Value{ &m_start }, Value{ &m_duration }, Value{ &frameStartTime }),
                },
            },
            rcnew Text{
                text = transform(
                           [](double startTime, double duration, double nowTime) -> double {
                               return std::min(nowTime - startTime, duration);
                           },
                           Value{ &m_start }, Value{ &m_duration }, Value{ &frameStartTime })
                           .transform(FixedFormatter<"{:.1f}s">{}),
            },
            rcnew HLayout{
                "Duration:"_Text,
                gapColumn = 0.5_em,
                rcnew Slider{
                    value    = Value{ &m_duration },
                    minimum  = 0,
                    maximum  = 30,
                    flexGrow = 1,
                },
            },
            rcnew Button{
                "Reset"_Text,
                onClick = BindableCallback<>{ this, &Timer::reset },
            },
        };
    }

    void configureWindow(Rc<GuiWindow> window) final {
        window->setTitle("Timer"_tr);
        window->setStyle(WindowStyle::None);
        window->windowFit = WindowFit::FixedSize;
    }

    double m_start    = currentTime();
    double m_duration = 10;
};

} // namespace Brisk

using namespace Brisk;

int briskMain() {
    GuiApplication application;

    return application.run(createComponent<Timer>());
}
