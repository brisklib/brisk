#include "ShowcaseComponent.hpp"
#include "brisk/graphics/ImageFormats.hpp"
#include "brisk/widgets/ListBox.hpp"
#include "brisk/widgets/SpinBox.hpp"
#include <brisk/core/Utilities.hpp>
#include <brisk/core/App.hpp>
#include <brisk/widgets/ComboBox.hpp>
#include <brisk/widgets/ImageView.hpp>
#include <brisk/widgets/Table.hpp>
#include <brisk/widgets/TextEditor.hpp>
#include <brisk/widgets/Viewport.hpp>
#include <brisk/widgets/Pages.hpp>
#include <brisk/widgets/ContextPopup.hpp>
#include <brisk/widgets/ScrollBox.hpp>
#include <brisk/widgets/PopupDialog.hpp>
#include <brisk/window/OSDialogs.hpp>
#include <brisk/graphics/Palette.hpp>
#include <brisk/core/internal/Initialization.hpp>
#include <brisk/widgets/Graphene.hpp>
#include <brisk/gui/Icons.hpp>
#include <brisk/widgets/Layouts.hpp>
#include <brisk/widgets/Widgets.hpp>
#include <brisk/graphics/Fonts.hpp>
#include <brisk/window/Clipboard.hpp>
#include <brisk/window/WindowApplication.hpp>
#include <brisk/widgets/Spinner.hpp>
#include <brisk/widgets/Progress.hpp>
#include <brisk/widgets/Spacer.hpp>

namespace Brisk {

inline AsyncValue<int> randomNumber() {
    AsyncOperation<int> op;
    std::thread([op]() mutable {
        std::this_thread::sleep_for(std::chrono::milliseconds(1500));
        static int counter = 0;
        op.execute([&]() -> int {
            if (counter == 10)
                throw std::range_error("counter reached 10");
            return ++counter;
        });
    }).detach();
    return op.value();
}

[[maybe_unused]] static void debugPanel(RawCanvas& canvas, Rectangle rect) {
    auto w = inputQueue->focused.lock();
    if (w) {
        auto&& saved = canvas.save();

        canvas.drawRectangle(w->rect(), 0.f, 0.f, strokeWidth = 0.1,
                             fillColor = ColorF(Palette::Standard::yellow).multiplyAlpha(0.5f));
        Widget* ww = w.get();
        canvas.drawText(rect, 0.5f, 0.5f,
                        fmt::format("{} v:{} iv:{} tree:{}", typeid(*ww).name(), w->visible.get(),
                                    w->isVisible(), (void*)w->tree()),
                        Font{ Monospace, 14_dp }, Palette::white);
    }
}

static RC<Stylesheet> mainStylesheet = rcnew Stylesheet{
    Graphene::stylesheet(),
    Style{
        Selectors::Class{ "section-header" },
        Rules{
            fontSize      = 14_px,
            fontFamily    = Monospace,
            color         = 0x5599ff_rgb,
            margin        = { 0, 10_apx },

            borderColor   = 0x5599ff_rgb,
            borderWidth   = { 0, 0, 0, 1_apx },
            paddingBottom = 2_apx,
        },
    },
    Style{
        Selectors::Type{ "imageview" } && !Selectors::Class{ "zoom" },
        Rules{
            placement  = Placement::Normal,
            dimensions = { auto_, auto_ },
            zorder     = ZOrder::Normal,
        },
    },
    Style{
        Selectors::Type{ "imageview" } && Selectors::Class{ "zoom" },
        Rules{
            placement        = Placement::Window,
            dimensions       = { 100_perc, 100_perc },
            absolutePosition = { 0, 0 },
            anchor           = { 0, 0 },
            zorder           = ZOrder::TopMost,
        },
    },
};

RC<Widget> ShowcaseComponent::build() {
    auto notifications = notManaged(&m_notifications);
    return rcnew VLayout{
        flexGrow   = 1,
        stylesheet = mainStylesheet,
        Graphene::darkColors(),

        new HLayout{
            fontSize = 24_dpx,
            new Button{
                padding = 8_dpx,
                new Text{ ICON_zoom_in },
                borderWidth = 1_dpx,
                onClick     = m_lifetime |
                          []() {
                              windowApplication->uiScale =
                                  std::exp2(std::round(std::log2(windowApplication->uiScale) * 2 + 1) * 0.5);
                          },
            },
            new Button{
                padding = 8_dpx,
                new Text{ ICON_zoom_out },
                borderWidth = 1_dpx,
                onClick     = m_lifetime |
                          []() {
                              windowApplication->uiScale =
                                  std::exp2(std::round(std::log2(windowApplication->uiScale) * 2 - 1) * 0.5);
                          },
            },
            new Button{
                padding = 8_dpx,
                new Text{ ICON_sun_moon },
                borderWidth = 1_dpx,
                onClick     = m_lifetime |
                          [this]() {
                              m_lightTheme = !m_lightTheme;
                              this->tree().disableTransitions();
                              if (m_lightTheme)
                                  this->tree().root()->apply(Graphene::lightColors());
                              else
                                  this->tree().root()->apply(Graphene::darkColors());
                          },
            },
        },
        new Pages{
            Value{ &m_activePage },
            new Tabs{},
            new Page{ "Buttons", new VScrollBox{ flexGrow = 1, m_buttons->build(notifications) } },
            new Page{ "Dropdowns", new VScrollBox{ flexGrow = 1, m_dropdowns->build(notifications) } },
            new Page{ "Editors", new VScrollBox{ flexGrow = 1, m_editors->build(notifications) } },
            new Page{ "Visual", new VScrollBox{ flexGrow = 1, m_visual->build(notifications) } },
            new Page{ "Layout", new VScrollBox{ flexGrow = 1, m_layout->build(notifications) } },
            new Page{ "Dialogs", new VScrollBox{ flexGrow = 1, m_dialogs->build(notifications) } },
            new Page{ "Typography", new VScrollBox{ flexGrow = 1, m_typography->build(notifications) } },
            new Page{ "Messenger", new VScrollBox{ flexGrow = 1, m_messenger->build(notifications) } },
            flexGrow = 1,
        },
        rcnew NotificationContainer(notifications),
    };
}

ShowcaseComponent::ShowcaseComponent() {}

void ShowcaseComponent::unhandledEvent(Event& event) {
    if (event.keyPressed(KeyCode::F2)) {
        Internal::debugShowRenderTimeline = !Internal::debugShowRenderTimeline;
    } else if (event.keyPressed(KeyCode::F3)) {
        Internal::debugBoundaries = !Internal::debugBoundaries;
    } else if (event.keyPressed(KeyCode::F4)) {
        if (auto t = window() ? window()->target() : nullptr)
            t->setVSyncInterval(1 - t->vsyncInterval());
    } else if (event.keyPressed(KeyCode::F5)) {
        tree().root()->dump();
    }
}

void ShowcaseComponent::configureWindow(RC<GUIWindow> window) {
    window->setTitle("Brisk Showcase"_tr);
    window->setSize({ 1050, 740 });
    window->setStyle(WindowStyle::Normal);
}
} // namespace Brisk
