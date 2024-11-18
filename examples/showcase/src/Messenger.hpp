#pragma once

#include <brisk/widgets/Notifications.hpp>

namespace Brisk {

class ShowcaseMessenger : public BindingObject<ShowcaseMessenger, &uiThread> {
public:
    ShowcaseMessenger();

    RC<Widget> build(RC<Notifications> notifications);

private:
    enum class Status {
        Sent,
        Read,
    };

    struct Message {
        Status status;
        std::chrono::system_clock::time_point date;
        std::variant<std::string, RC<Image>> content;
        std::string reaction;
    };

    void send();
    void messagesBuilder(Widget* target);

    std::vector<Message> m_messages;
    Trigger<> m_messagesChanged;
    std::string m_chatMessage;
    RC<Image> m_zoomImage;
};
} // namespace Brisk
