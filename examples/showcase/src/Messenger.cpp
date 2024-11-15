#include "Messenger.hpp"
#include <resources/hot_air_balloons.hpp>
#include <brisk/gui/Icons.hpp>
#include <fmt/chrono.h>

namespace Brisk {

void ShowcaseMessenger::messagesBuilder(Widget* target) {
    for (const Message& msg : m_messages) {
        std::string statusIcon = msg.status == Status::Read ? ICON_check_check : ICON_check;
        RC<Widget> content;
        std::visit(Overload{
                       [&](std::string textContent) {
                           content = rcnew Text{ std::move(textContent), wordWrap = true };
                       },
                       [&](RC<Image> imageContent) {
                           float imageAspect =
                               static_cast<float>(imageContent->width()) / imageContent->height();
                           content = rcnew ImageView{
                               std::move(imageContent),
                               width  = auto_,
                               height = auto_,
                               aspect = imageAspect,
                           };
                       },
                   },
                   msg.content);
        target->apply(new VLayout{
            alignSelf = AlignSelf::FlexEnd,
            padding   = { 8, 6 },
            std::move(content),
            new Text{ fmt::format("{:%H:%M}   {}", msg.date, statusIcon), marginTop = 4_apx,
                      textAlign = TextAlign::End, opacity = 0.5f },
            width           = 360_apx,
            backgroundColor = 0x454545_rgb,
            borderWidth     = 1_apx,
            borderRadius    = -12,
        });
    }
}

RC<Widget> ShowcaseMessenger::build(RC<Notifications> notifications) {
    return rcnew VLayout{
        flexGrow  = 1,
        padding   = 16_apx,
        alignSelf = AlignSelf::Stretch,

        new VLayout{
            flexGrow  = 1,
            alignSelf = AlignSelf::Stretch,
            new VScrollBox{
                flexGrow  = 1,
                alignSelf = AlignSelf::Stretch,
                new VLayout{
                    gapRow  = 8,
                    padding = 4,
                    depends = Value{ &m_messagesChanged }, // Rebuild if triggered

                    Builder{
                        [this](Widget* target) {
                            messagesBuilder(target);
                        },
                    },
                },
            },
            new HLayout{
                backgroundColor = Palette::white,
                borderRadius    = -5.f,
                new Button{
                    new Text{ ICON_paperclip },
                    classes = { "flat" },
                    color   = 0x373737_rgb,
                },
                new TextEditor{
                    Value{ &m_chatMessage },
                    flexGrow        = 1,
                    padding         = 8,
                    backgroundColor = Palette::transparent,
                    borderWidth     = 0,
                    onEnter         = m_lifetime |
                              [this]() {
                                  send();
                              },
                },
                new Button{
                    new Text{ ICON_send_horizontal },
                    classes = { "flat" },
                    color   = 0x373737_rgb,
                    onClick = m_lifetime |
                              [this]() {
                                  send();
                              },
                },
            },
        },
    };
}

ShowcaseMessenger::ShowcaseMessenger() {
    auto date  = std::chrono::system_clock::now();
    m_messages = {
        Message{
            Status::Read,
            date - std::chrono::minutes(122),
            "Proin vitae facilisis nisi. Nullam sodales vel turpis tincidunt "
            "pulvinar. "
            "Duis mattis venenatis nisi eget lacinia. In hac habitasse platea "
            "dictumst. "
            "Vestibulum lacinia tortor sit amet arcu ornare, eget pulvinar odio "
            "fringilla. "
            "Praesent volutpat sed erat quis ornare. Suspendisse potenti. "
            "Nunc vel venenatis velit. Nunc purus ipsum, auctor vitae enim at, "
            "fermentum "
            " luctus dolor.Aliquam ex enim, dignissim in dignissim vitae, "
            " pretium vestibulum ligula.",
            ICON_heart,
        },
        Message{
            Status::Read,
            date - std::chrono::minutes(71),
            imageDecode(hot_air_balloons(), ImageFormat::RGBA).value(),
            ICON_heart,
        },
        Message{
            Status::Sent,
            date - std::chrono::minutes(12),
            "Sed semper leo pulvinar cursus luctus. Cras nec  sapien non mauris "
            "suscipit blandit.Donec elit sem",
            ICON_heart,
        },
    };
}

void ShowcaseMessenger::send() {
    if (!m_chatMessage.empty()) {
        m_messages.push_back(Message{
            Status::Sent,
            std::chrono::system_clock::now(),
            m_chatMessage,
            "",
        });
        m_chatMessage = "";
        bindings->notify(&m_chatMessage);
        bindings->notify(&m_messagesChanged);
    }
}
} // namespace Brisk
