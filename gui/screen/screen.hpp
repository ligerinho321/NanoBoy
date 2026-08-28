#pragma once

#include <gui/utils/utils.hpp>

class screen_t {
private:
    gb_t* gb;

    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;

    SDL_Texture* texture = nullptr;
    
    SDL_Rect embedded_rect = {};

    bool floating = false;
    bool aspect_ratio = false;
    bool interger_scale = false;
    bool bilinear_filtering = false;

    bool _focused = false;

    ImVec2 floating_min_size{0.0f,0.0f};
    ImVec2 floating_max_size{0.0f,0.0f};

    ImVec2 floating_pos{0.0f,0.0f};
    ImVec2 floating_size{0.0f,0.0f};
    ImVec2 last_evail_size{0.0f,0.0f};

    void create_texture();

    void set_embedded_scale(int new_scale);
    
    void update_embedded_size();

    void set_bilinear_filtering(bool new_bilinear_filtering);
    
public:
    screen_t(gb_t* gb,SDL_Window* window,SDL_Renderer* renderer);

    ~screen_t();

    void save(cJSON* settings_object);
    void load(cJSON* settings_object);

    void update_screen();

    void event(SDL_Event& event);

    void render_menu_bar();
    void render_floating();
    void render_embedded();

    void clear(){
        clear_texture(texture,gb_screen_height);
    }

    bool focused(){
        return floating && _focused;
    }
};