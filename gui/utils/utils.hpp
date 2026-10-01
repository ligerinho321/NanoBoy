#pragma once

#include <core/gb.h>

#include <third_party/cJSON/cJSON.h>
#include <third_party/stb_image_write/stb_image_write.h>
#include <third_party/imgui/imgui.h>
#include <third_party/imgui/imgui_impl_sdl2.h>
#include <third_party/imgui/imgui_impl_sdlrenderer2.h>

#include <SDL2/SDL.h>

#include <zstd.h>

#include <iostream>
#include <chrono>
#include <filesystem>
#include <vector>
#include <array>
#include <list>
#include <algorithm>
#include <string>
#include <regex>
#include <mutex>
#include <atomic>
#include <cinttypes>

enum{
    kilobytes = 1024,
    megabytes = 1024 * kilobytes,
    gigabytes = 1024 * megabytes,

    kilohertz = 1000,
    megahertz = 1000 * kilohertz,
    gigahertz = 1000 * megahertz
};


struct palette_t{
    enum{
        texture_max_width = gb_palette_colors * gb_tile_size,
        texture_max_height = gb_cgb_palettes * gb_tile_size,
        
        cgb_texture_height = texture_max_height,

        dmg_bg_texture_height = gb_dmg_bg_palettes * gb_tile_size,

        dmg_obj_texture_height = gb_dmg_obj_palettes * gb_tile_size,
        
        texture_format = SDL_PIXELFORMAT_RGB24,
        texture_bytes_per_pixel = SDL_BYTESPERPIXEL(texture_format),
        texture_access = SDL_TEXTUREACCESS_STREAMING
    };

    SDL_Texture* texture = nullptr;
    bool is_obj;

    void init(bool _is_obj,SDL_Renderer* _renderer){
        is_obj = _is_obj;
        texture = SDL_CreateTexture(_renderer,texture_format,texture_access,texture_max_width,texture_max_height);
    }

    void uninit(){
        SDL_DestroyTexture(texture);
    }

    virtual uint8_t get_dmg_address_color(uint8_t palette_index, uint8_t color_index) const noexcept = 0;

    virtual uint8_t get_cgb_address_color(uint8_t palette_index,uint8_t color_index) const noexcept = 0;


    virtual gb_rgb_t get_dmg_color(uint8_t palette_index, uint8_t color_index) const noexcept = 0;

    virtual gb_rgb_t get_cgb_color(uint8_t palette_index,uint8_t color_index) const noexcept = 0;
    

    virtual void update_data(gb_palette_t* palette) noexcept = 0;

    void update_texture(bool cgb_mode);
};

struct bg_palette_t : public palette_t {
    uint8_t bgp = 0;
    gb_rgb_t colors[gb_cgb_colors] = {};

    void init(SDL_Renderer* _renderer){
        palette_t::init(false,_renderer);
    }

    void uninit(){
        palette_t::uninit();
    }

    void clear();

    uint8_t get_dmg_address_color(uint8_t palette_index, uint8_t color_index) const noexcept override {
        gb_unused(palette_index);
        return (bgp >> ((color_index & 0x03) << 0x01)) & 0x03;
    }

    uint8_t get_cgb_address_color(uint8_t palette_index,uint8_t color_index) const noexcept override {
        return ((palette_index & 0x07) << 0x02) | (color_index & 0x03);
    }


    gb_rgb_t get_dmg_color(uint8_t palette_index, uint8_t color_index) const noexcept override {
        return colors[get_dmg_address_color(palette_index,color_index)];
    }

    gb_rgb_t get_cgb_color(uint8_t palette_index,uint8_t color_index) const noexcept override {
        return colors[get_cgb_address_color(palette_index,color_index)];
    }


    void update_data(gb_palette_t* palette) noexcept override {
        bgp = palette->state.bgp;
        memcpy(colors,palette->state.bg_cram_converted,sizeof(colors));
    }
};

struct obj_palette_t : public palette_t {
    uint8_t obp[2] = {};
    gb_rgb_t colors[gb_cgb_colors] = {};

    void init(SDL_Renderer* _renderer){
        palette_t::init(true,_renderer);
    }

    void uninit(){
        palette_t::uninit();
    }

    void clear();

    uint8_t get_dmg_address_color(uint8_t palette_index,uint8_t color_index) const noexcept override {
        return ((palette_index & 0x01) << 0x02) | ((obp[palette_index & 0x01] >> ((color_index & 0x03) << 0x01)) & 0x03);
    }

    uint8_t get_cgb_address_color(uint8_t palette_index,uint8_t color_index) const noexcept override {
        return ((palette_index & 0x07) << 0x02) | (color_index & 0x03);
    }


    gb_rgb_t get_dmg_color(uint8_t palette_index,uint8_t color_index) const noexcept override {
        return colors[get_dmg_address_color(palette_index,color_index)];
    }

    gb_rgb_t get_cgb_color(uint8_t palette_index,uint8_t color_index) const noexcept override {
        return colors[get_cgb_address_color(palette_index,color_index)];
    }

    void update_data(gb_palette_t* palette) noexcept override {
        obp[0] = palette->state.obp[0];
        obp[1] = palette->state.obp[1];
        memcpy(colors,palette->state.obj_cram_converted,sizeof(colors)); 
    }
};


enum contoller_binding_type_t{
    controller_binding_none,
    controller_binding_button,
    controller_binding_axis,
    controller_binding_count
};

enum binding_type_t{
    binding_none,
    binding_keyboard,
    binding_controller,
    binding_count
};

extern const char* controller_binding_type_names[binding_count];

enum{
    controller_axis_deadzone = 8000
};

struct keyboard_binding_t {
    SDL_Scancode scancode;
    SDL_Keymod modifiers;
};

struct controller_binding_t {
    uint8_t type;
    union{            
        uint8_t button;
        struct{ 
            uint8_t index;
            bool negative;
        }axis;
    };
};

struct binding_state_t {
    bool down;
    bool pressed;
    bool released;
};

struct binding_capture_popup_t {
private:
    bool is_keyboard = false;

    bool _open = false;
    bool open_needed = false;

    ImVec2 start_pos = {};
    ImVec2 size = {};
public:
    void render();

    void open(bool _is_keyboard) noexcept {
        if(!_open){
            is_keyboard = _is_keyboard;
            open_needed = true;
        }
    }

    void close() noexcept {
        _open = false;
        open_needed = false;
    }

    bool get_open() const noexcept {
        return _open;
    }
};

inline SDL_Keymod keyboard_binding_normalize_modifiers(int mod){
    int result = KMOD_NONE;
    if(mod & KMOD_CTRL)  result |= KMOD_CTRL;
    if(mod & KMOD_SHIFT) result |= KMOD_SHIFT;
    if(mod & KMOD_ALT)   result |= KMOD_ALT;
    if(mod & KMOD_GUI)   result |= KMOD_GUI;
    return (SDL_Keymod)result;
}

int get_controller_binding_type_from_string(const char* string);

void save_keyboard_binding(cJSON* keyboard_binding_object,keyboard_binding_t& keyboard);
bool load_keyboard_binding(cJSON* keyboard_binding_object,keyboard_binding_t& keyboard);

void save_controller_binding(cJSON* controller_binding_object,controller_binding_t& controller);
bool load_controller_binding(cJSON* controller_binding_object,controller_binding_t& controller);

std::string get_keyboard_binding_string(const keyboard_binding_t& keyboard);
std::string get_controller_binding_string(const controller_binding_t& controller);


inline float get_input_scalar_width(int digit_count){
    ImGuiStyle& style = ImGui::GetStyle();
    return ImGui::CalcTextSize(std::string(digit_count,'0').c_str()).x + style.FramePadding.x * 2.0f + (ImGui::GetFrameHeight() + style.ItemInnerSpacing.x) * 2.0f;
}

inline bool mouse_in_rect(const ImVec2& m,const ImVec2& p_min,const ImVec2& p_max){
    return (m.x >= p_min.x && m.x < p_max.x) && (m.y >= p_min.y && m.y < p_max.y);
}


void render_size_text(size_t size);

void render_hertz_text(size_t hertz);

time_t get_file_last_write_time(std::filesystem::path file);

const char* get_time_formated(time_t time);

void clear_texture(SDL_Texture* texture,int height);