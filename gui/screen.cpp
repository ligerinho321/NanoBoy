#include "screen.hpp"

screen_t::screen_t(SDL_Renderer* renderer){
    texture = SDL_CreateTexture(renderer,SDL_PIXELFORMAT_RGB24,SDL_TEXTUREACCESS_STREAMING,gb_screen_width,gb_screen_height);

    ImGuiStyle& style = ImGui::GetStyle();

    floating_min_size.x = gb_screen_width + style.WindowPadding.x * 2.0f;
    floating_min_size.y = ImGui::GetFrameHeight() + gb_screen_height + style.WindowPadding.y * 2.0f;

    floating_max_size.x = FLT_MAX;
    floating_max_size.y = FLT_MAX;
}

screen_t::~screen_t(){
    SDL_DestroyTexture(texture);
}


void screen_t::set_embedded_scale(SDL_Window* window,int new_scale){

    if(new_scale == embedded_scale) return;

    embedded_scale = new_scale;

    uint32_t flags = SDL_GetWindowFlags(window);
    
    if(flags & SDL_WINDOW_MAXIMIZED){
        SDL_RestoreWindow(window);
    }
    else if(flags & SDL_WINDOW_FULLSCREEN){
        SDL_SetWindowFullscreen(window,0);
    }

    int main_menu_bar_height = ImGui::GetFrameHeight();

    embedded_rect.x = 0;
    embedded_rect.y = main_menu_bar_height;
    embedded_rect.w = gb_screen_width * embedded_scale;
    embedded_rect.h = gb_screen_height * embedded_scale;

    SDL_SetWindowSize(window,embedded_rect.w,embedded_rect.h + main_menu_bar_height);
}

void screen_t::update_embedded_size(SDL_Window* window){
    int window_width = 0;
    int window_height = 0;

    SDL_GetWindowSize(window,&window_width,&window_height);

    int main_menu_bar_height = ImGui::GetFrameHeight();

    window_height -= main_menu_bar_height;

    float ratio_scaleX = (float)window_width / gb_screen_width;
    float ratio_scaleY = (float)window_height / gb_screen_height;

    float ratio_scale = gb_min(ratio_scaleX,ratio_scaleY);

    embedded_rect.w = gb_screen_width * ratio_scale;
    embedded_rect.h = gb_screen_height * ratio_scale;

    embedded_rect.x = (window_width - embedded_rect.w) / 2;
    embedded_rect.y = main_menu_bar_height + (window_height - embedded_rect.h) / 2;
}


void screen_t::render(){
    if(mode != floating_mode) return;

    ImGui::SetNextWindowSizeConstraints(floating_min_size,floating_max_size);

    if(ImGui::Begin("Screen",nullptr)){
        
        ImVec2 evail_size = ImGui::GetContentRegionAvail();
        
        if(evail_size.x != last_evail_size.x || evail_size.y != last_evail_size.y){

            float ratio_scale_x = evail_size.x / gb_screen_width;
            float ratio_scale_y = evail_size.y / gb_screen_height;

            float ratio_scale = gb_min(ratio_scale_x,ratio_scale_y);

            floating_size.x = gb_screen_width * ratio_scale;
            floating_size.y = gb_screen_height * ratio_scale;

            ImVec2 cursor = ImGui::GetCursorPos();
            
            floating_pos.x = cursor.x + (evail_size.x - floating_size.x) * 0.5f;
            floating_pos.y = cursor.y + (evail_size.y - floating_size.y) * 0.5f;

            last_evail_size = evail_size;
        }

        ImGui::SetCursorPos(floating_pos);

        ImGui::Image((ImTextureRef)texture,floating_size);
    }

    ImGui::End();
}