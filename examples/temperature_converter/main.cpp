#include <brisk/gui/GuiApplication.hpp>
#include <brisk/gui/GuiWindow.hpp>
#include <brisk/core/Text.hpp>
#include <brisk/widgets/Widgets.hpp>
#include <brisk/gui/Component.hpp>

namespace Brisk {

inline float toCelsius(float fahrenheit) {
    return (fahrenheit - 32.f) * 5.f / 9.f;
}

inline float toFahrenheit(float celsius) {
    return (celsius * 9.f / 5.f) + 32.f;
}

class TemperatureConverter final : public Component {
public:
    TemperatureConverter() {
        bindings->connectBidir(Value{ &m_celsius }.transform(
                                   [](std::string val) -> std::optional<std::string> {
                                       if (auto num = toNumber<float>(val)) {
                                           return fmt::format("{}", toFahrenheit(*num));
                                       }
                                       return {};
                                   },
                                   [](std::string val) -> std::optional<std::string> {
                                       if (auto num = toNumber<float>(val)) {
                                           return fmt::format("{}", toCelsius(*num));
                                       }
                                       return {};
                                   }),
                               Value{ &m_fahrenheit });
    }

    Rc<Widget> build() final {
        return rcnew HLayout{
            stylesheet = Graphene::stylesheet(),
            Graphene::darkColors(),
            padding   = 1_em,
            gapColumn = 0.5_em,
            rcnew TextEditor{
                width = 5_em,
                text  = Value{ &m_celsius },
            },
            "°Celsius = "_Text,
            rcnew TextEditor{
                width = 5_em,
                text  = Value{ &m_fahrenheit },
            },
            "°Fahrenheit"_Text,
        };
    }

    void configureWindow(Rc<GuiWindow> window) final {
        window->setTitle("Temperature converter"_tr);
        window->setStyle(WindowStyle::None);
        window->windowFit = WindowFit::FixedSize;
    }

    std::string m_celsius;
    std::string m_fahrenheit;
};

} // namespace Brisk

using namespace Brisk;

int briskMain() {
    GuiApplication application;

    return application.run(createComponent<TemperatureConverter>());
}
