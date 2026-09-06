#include <gui/utils/utils.hpp>

class notification_manager_t {
private:
    enum{
        max_notifications = 5,
        notification_duration = 2
    };

    struct notification_t {
        std::string message;
        std::chrono::steady_clock::time_point time;

        notification_t(const char* m,std::chrono::steady_clock::time_point t):message(m),time(t){}
    };

    std::list<notification_t> notifications;

    ImFont* font = nullptr;
    
    char buffer[256] = {};

public:
    notification_manager_t();

    void push_notification(const char* fmt,...);

    void render();
};