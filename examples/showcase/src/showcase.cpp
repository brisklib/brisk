#include "ShowcaseComponent.hpp"
#include <brisk/gui/GuiApplication.hpp>

int briskMain() {
    using namespace Brisk;

    GuiApplication application;
    return application.run(createComponent<ShowcaseComponent>());
}
