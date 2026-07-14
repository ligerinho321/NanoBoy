#pragma once

#include <gui/utils/utils.hpp>

class screen_t {
private:
    ImVec2 floating_min_size{0.0f,0.0f};
    ImVec2 floating_max_size{0.0f,0.0f};

    ImVec2 floating_pos{0.0f,0.0f};
    ImVec2 floating_size{0.0f,0.0f};
    ImVec2 last_evail_size{0.0f,0.0f};
public:
    enum {
        embedded_mode = 0,
        floating_mode = 1,
    };
        
    SDL_Texture* texture = nullptr;
    
    SDL_Rect embedded_rect{0};

    bool mode = embedded_mode;

    int embedded_scale = 0;

    screen_t(SDL_Renderer* renderer);

    ~screen_t();

    void set_embedded_scale(SDL_Window* window,int new_scale);

    void update_embedded_size(SDL_Window* window);

    void render();

    void clear(){
        clear_texture(texture,gb_screen_height);
    }
};