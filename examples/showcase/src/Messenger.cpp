#include "Messenger.hpp"
#include <brisk/core/Resources.hpp>
#include <brisk/gui/Icons.hpp>
#include <fmt/chrono.h>

namespace Brisk {

static void backgroundPainter(Canvas& canvas, const Widget& widget) {
    static auto img = imageDecode(loadResource("wp1.webp"), ImageFormat::RGBA).value();
    float x         = static_cast<float>(img->width()) / widget.rect().width(),
          y         = static_cast<float>(img->height()) / widget.rect().height();
    float m         = std::min(x, y);
    x /= m;
    y /= m;
    canvas.raw().drawTexture(widget.rect(), img,
                             Matrix::scaling(x, y).translate(0.5f * (1 - x) * widget.rect().width(),
                                                             0.5f * (1 - y) * widget.rect().height()));
}

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
                               imageContent,
                               aspect  = imageAspect,
                               classes = Value{ &m_zoomImage }.transform([imageContent](RC<Image> img) {
                                   return img == imageContent ? Classes{ "zoom" } : Classes{};
                               }),

                               onClick = m_lifetime |
                                         [this, imageContent]() {
                                             if (m_zoomImage)
                                                 bindings->assign(m_zoomImage, nullptr);
                                             else
                                                 bindings->assign(m_zoomImage, imageContent);
                                         },
                           };
                       },
                   },
                   msg.content);
        target->apply(rcnew VLayout{
            alignSelf = AlignSelf::FlexEnd,
            padding   = { 8, 6 },
            std::move(content),
            rcnew Text{ fmt::format("{:%H:%M}   {}", msg.date, statusIcon), marginTop = 4_apx,
                        textAlign = TextAlign::End, opacity = 0.5f },
            width           = 360_apx,
            backgroundColor = 0xe5f7df'F0_rgba,
            borderWidth     = 1_apx,
            borderRadius    = -12,
        });
    }
}

RC<Widget> ShowcaseMessenger::build(RC<Notifications> notifications) {
    return rcnew VLayout{
        flexGrow      = 1,
        padding       = 16_apx,
        alignSelf     = AlignSelf::Stretch,

        color         = 0x080808_rgb,

        selectedColor = 0x32a852_rgb,

        painter       = Painter(&backgroundPainter),

        rcnew VLayout{
            flexGrow       = 1,
            alignSelf      = AlignSelf::Stretch,
            scrollBarColor = 0x32a852_rgb,
            rcnew VScrollBox{
                flexGrow  = 1,
                alignSelf = AlignSelf::Stretch,
                rcnew VLayout{
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
            rcnew HLayout{
                backgroundColor = Palette::white,
                borderRadius    = -5.f,
                rcnew Button{
                    rcnew Text{ ICON_paperclip },
                    classes = { "flat" },
                    color   = 0x373737_rgb,
                },
                rcnew TextEditor{
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
                rcnew Button{
                    rcnew Text{ ICON_send_horizontal },
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
            imageDecode(loadResource("hot-air-balloons.jpg"), ImageFormat::RGBA).value(),
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
