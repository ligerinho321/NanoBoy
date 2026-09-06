#include <gui/notification_manager/notification_manager.hpp>
#include <gui/assets/assets.hpp>

notification_manager_t::notification_manager_t(){
    ImGuiIO& io = ImGui::GetIO();

    ImFontConfig font_config = {};
    font_config.FontDataOwnedByAtlas = false;

    font = io.Fonts->AddFontFromMemoryTTF(font_data,font_size,22.0f,&font_config);
}

void notification_manager_t::push_notification(const char* fmt,...){

    va_list args;
    va_start(args,fmt);
    vsnprintf(buffer,sizeof(buffer),fmt,args);
    va_end(args);

    if(notifications.size() >= notification_manager_t::max_notifications){
        notifications.pop_front();
    }

    notifications.emplace_back(buffer,std::chrono::steady_clock::now());
}

void notification_manager_t::render(){

    if(!notifications.size()) return;

    ImDrawList* draw_list = ImGui::GetForegroundDrawList();
    ImGuiStyle& style = ImGui::GetStyle();

    float main_menu_bar_height = ImGui::GetFrameHeight();

    ImVec2 clip_rect_min(0.0f,main_menu_bar_height);
    ImVec2 clip_rect_max = ImGui::GetMainViewport()->Size;;

    draw_list->PushClipRect(clip_rect_min,clip_rect_max,false);
    
    float line_height = ImGui::GetTextLineHeightWithSpacing();

    ImVec2 pos(style.WindowPadding.x,clip_rect_max.y - (line_height * notifications.size()) - style.WindowPadding.y);

    ImGui::PushFont(font);

    for(auto it = notifications.begin(); it != notifications.end();){

        ImVec2 text_size = ImGui::CalcTextSize(it->message.c_str());

        draw_list->AddRectFilled(pos,ImVec2(pos.x + text_size.x,pos.y + line_height),0x7F000000);

        draw_list->AddText(pos,0xFFFFFFFF,it->message.c_str());

        pos.y += line_height;

        std::chrono::nanoseconds elapsed = std::chrono::steady_clock::now() - it->time;

        if(std::chrono::duration_cast<std::chrono::seconds>(elapsed).count() >= notification_manager_t::notification_duration){
            it = notifications.erase(it);
        }
        else{
            ++it;
        }
    }

    ImGui::PopFont();
    
    draw_list->PopClipRect();
}