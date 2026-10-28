#pragma once

#include <gui/utils/utils.hpp>

class screen_t {
private:
    struct gb_layer_t {
        bool background_disabled = false;
        bool objects_disabled = false;
    };

    nanoboy_t* nanoboy = nullptr;

    int base_width = gba_screen_width;
    int base_height = gba_screen_height;

    bool _floating = false;
    bool aspect_ratio = false;
    bool interger_scale = false;
    bool interframe_blending = false;
    bool bilinear_filtering = false;

    gb_layer_t gb_layer;

    SDL_Rect embedded_rect{};

    ImVec2 floating_min_size{};
    ImVec2 floating_max_size{};

    ImVec2 floating_pos{};
    ImVec2 floating_size{};
    ImVec2 last_evail_size{};

    void set_embedded_scale(int new_scale);

    void set_bilinear_filtering(bool new_bilinear_filtering);
    
    void toggle_fullscreen();
    void toggle_floating();
    void toggle_acpect_ratio();
    void toggle_interger_scale();
    void toggle_interframe_blending();
    void toggle_bilinear_filtering();

    void toggle_disable_gb_background();
    void toggle_disable_gb_objects();

public:
    void init(nanoboy_t* _nanoboy);

    void update_console();

    void save(cJSON* settings_object);
    void load(cJSON* settings_object);

    void update_embedded_size();

    void shortcut_event();

    void render_menu_bar();
    void render_floating();
    void render_embedded();

    bool floating(){
        return _floating;
    }
};