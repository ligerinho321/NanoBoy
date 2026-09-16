#pragma once

#include <gui/utils/utils.hpp>

class dmg_palette_t {
private:
    gb_t* gb = nullptr;

    int current_preset = 0;

    ImVec4 temp_bgp[gb_palette_colors];
    ImVec4 temp_obp[gb_dmg_obj_palettes][gb_palette_colors];
    
    ImVec2 window_min_size{};
    ImVec2 window_max_size{};
    
    bool _open = false;

    gb_rgb_t vec4_to_rgb(ImVec4 color){
        return (gb_rgb_t){(uint8_t)(color.x * 255.0f),(uint8_t)(color.y * 255.0f),(uint8_t)(color.z * 255.0f)};
    }

    ImVec4 rgb_to_vec4(gb_rgb_t color){
        return (ImVec4){color.r / 255.0f,color.g / 255.0f,color.b / 255.0f,1.0f};
    }

    void update_window_size_constraints();

public:
    void init(gb_t* _gb);

    void save(cJSON* object);
    void load(cJSON* object);

    void render();

    void open() noexcept;

    void close(bool discard_changes) noexcept;
};