#include <brisk/gui/GuiApplication.hpp>
#include <brisk/widgets/Widgets.hpp>
#include <brisk/window/OsDialogs.hpp>
#include <brisk/gui/Component.hpp>
#include <brisk/graphics/Palette.hpp>
#include <sstream>

namespace Brisk {

enum class Flight {
    OneWayFlight,
    ReturnFlight,
};

static NameValueOrderedList<Flight> flightTypes{
    { "one-way flight", Flight::OneWayFlight },
    { "return flight", Flight::ReturnFlight },
};

static std::optional<std::chrono::year_month_day> parseDate(std::string str) {
    std::istringstream is(str);
    int day, month, year;
    char dash1, dash2;
    // Parse the string: expect DD-MM-YYYY
    is >> day >> dash1 >> month >> dash2 >> year;
    if (is.fail() || !is.eof() || dash1 != '-' || dash2 != '-') {
        return std::nullopt;
    }
    std::chrono::year_month_day ymd{ std::chrono::year{ year },
                                     std::chrono::month{ static_cast<unsigned>(month) },
                                     std::chrono::day{ static_cast<unsigned>(day) } };
    if (!ymd.ok())
        return std::nullopt;
    return ymd;
}

static ColorW dateValidator(std::string date) {
    return parseDate(date) ? Palette::white : 0xFFB0B0_rgb;
}

class FlightBooker final : public Component {
public:
    FlightBooker() {
        m_date1 = m_date2 = fmt::format(
            "{:%d-%m-%Y}", std::chrono::floor<std::chrono::days>(std::chrono::system_clock::now()));
    }

    Rc<Widget> build() final {
        return rcnew VLayout{
            stylesheet = Graphene::stylesheet(),
            Graphene::darkColors(),
            alignItems = Align::Stretch,
            padding    = 2_em,
            gapRow     = 1_em,

            rcnew ComboBox{
                Value{ &m_type },
                notManaged(&flightTypes),
            },
            rcnew TextEditor{
                text            = Value{ &m_date1 },
                backgroundColor = Value{ &m_date1 }.transform(dateValidator),
            },
            rcnew TextEditor{
                text            = Value{ &m_date2 },
                backgroundColor = Value{ &m_date2 }.transform(dateValidator),
                enabled         = Value{ &m_type } == Flight::ReturnFlight,
            },
            rcnew Button{
                "Book"_Text,
                onClick = BindableCallback<>{ this, &FlightBooker::book },
                enabled = transform(
                    [](Flight type, const std::string& str1, const std::string& str2) -> bool {
                        auto date1 = parseDate(str1), date2 = parseDate(str2);
                        return date1.has_value() &&
                               (type != Flight::ReturnFlight || (date2.has_value() && date2 >= date1));
                    },
                    Value{ &m_type }, Value{ &m_date1 }, Value{ &m_date2 }),
            },
        };
    }

    void book() {
        Shell::showMessage("You have booked a {} on {}"_trfmt(valueToKey(flightTypes, m_type), m_date1));
    }

    void configureWindow(Rc<GuiWindow> window) final {
        window->setTitle("Flight booker"_tr);
        window->setStyle(WindowStyle::None);
        window->windowFit = WindowFit::FixedSize;
    }

    Flight m_type = Flight::OneWayFlight;
    std::string m_date1;
    std::string m_date2;
};

} // namespace Brisk

using namespace Brisk;

int briskMain() {
    GuiApplication application;

    return application.run(createComponent<FlightBooker>());
}
