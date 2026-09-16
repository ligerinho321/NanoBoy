#pragma once

#include <gui/utils/utils.hpp>

class nanoboy_t;

class screen_t {
private:
    nanoboy_t* nanoboy = nullptr;
    gb_t* gb = nullptr;
    
    SDL_Texture* texture = nullptr;

    bool _floating = false;
    bool aspect_ratio = false;
    bool interger_scale = false;
    bool bilinear_filtering = false;

    SDL_Rect embedded_rect = {};

    ImVec2 floating_min_size{};
    ImVec2 floating_max_size{};

    ImVec2 floating_pos{};
    ImVec2 floating_size{};
    ImVec2 last_evail_size{};

    void create_texture();

    void set_embedded_scale(int new_scale);
    
    void update_embedded_size();

    void set_bilinear_filtering(bool new_bilinear_filtering);
    
public:
    void init(nanoboy_t* _nanoboy);
    void uninit();

    void save(cJSON* settings_object);
    void load(cJSON* settings_object);

    void update_screen();

    void event(SDL_Event& event);

    void render_menu_bar();
    void render_floating();
    void render_embedded();
    void render_dmg_palette();

    void clear(){
        clear_texture(texture,gb_screen_height);
    }

    bool floating(){
        return _floating;
    }
};