#include <brisk/gui/GuiApplication.hpp>
#include <brisk/gui/GuiWindow.hpp>
#include <brisk/widgets/Widgets.hpp>
#include <brisk/gui/Component.hpp>

namespace Brisk {

class Counter final : public Component {
public:
    using Component::Component;

    // Builds the UI layout with a text display and a button
    Rc<Widget> build() final {
        return rcnew HLayout{
            // Apply Graphene stylesheet for consistent styling
            stylesheet = Graphene::stylesheet(),
            // Use dark theme colors
            Graphene::darkColors(),

            padding = 1_em,

            rcnew Text{
                padding = { 3_em, 0 },
                // Bind text to the m_value variable
                text    = toString(Value{ &m_value }),
            },
            rcnew Button{
                "Count"_Text,
                // Increment m_value when clicked
                onClick = lifetime() |
                          [this]() {
                              ++*bindings->modify(m_value);
                          },
            },
        };
    }

    // Configures the window properties
    void configureWindow(Rc<GuiWindow> window) final {
        window->setTitle("Counter"_tr);
        window->setStyle(WindowStyle::None);
        window->windowFit = WindowFit::FixedSize;
    }

    int m_value{ 0 };
};

} // namespace Brisk

using namespace Brisk;

int briskMain() {
    GuiApplication application;
    return application.run(createComponent<Counter>());
}
