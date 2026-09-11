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

    SDL_Texture* texture;
    bool is_obj;

    palette_t(SDL_Renderer* renderer,bool _is_obj):
    texture(SDL_CreateTexture(renderer,texture_format,texture_access,texture_max_width,texture_max_height)),
    is_obj(_is_obj)
    {}

    virtual ~palette_t(){
        SDL_DestroyTexture(texture);
    }

    virtual uint8_t get_dmg_address_color(uint8_t palette_index, uint8_t color_index) const noexcept = 0;

    virtual uint8_t get_cgb_address_color(uint8_t palette_index,uint8_t color_index) const noexcept = 0;

    virtual uint8_t get_cgb_dmg_address_color(uint8_t palette_index,uint8_t color_index) const noexcept = 0;


    virtual gb_rgb_t get_dmg_color(uint8_t palette_index, uint8_t color_index) const noexcept = 0;

    virtual gb_rgb_t get_cgb_color(uint8_t palette_index,uint8_t color_index) const noexcept = 0;

    virtual gb_rgb_t get_cgb_dmg_color(uint8_t palette_index,uint8_t color_index) const noexcept = 0;


    virtual void update_data(gb_palette_t* palette) noexcept = 0;

    void update_texture(bool is_cgb,bool cgb_mode);
};

struct bg_palette_t : public palette_t {
    uint8_t bgp = 0;
    gb_rgb_t colors[gb_cgb_colors] = {};

    bg_palette_t(SDL_Renderer* renderer):palette_t(renderer,false){}

    void clear();

    uint8_t get_dmg_address_color(uint8_t palette_index, uint8_t color_index) const noexcept override {
        gb_unused(palette_index);
        return (bgp >> ((color_index & 0x03) << 0x01)) & 0x03;
    }

    uint8_t get_cgb_address_color(uint8_t palette_index,uint8_t color_index) const noexcept override {
        return ((palette_index & 0x07) << 0x02) | (color_index & 0x03);
    }

    uint8_t get_cgb_dmg_address_color(uint8_t palette_index,uint8_t color_index) const noexcept override {
        return get_dmg_address_color(palette_index,color_index);
    }


    gb_rgb_t get_dmg_color(uint8_t palette_index, uint8_t color_index) const noexcept override {
        return dmg_colors[get_dmg_address_color(palette_index,color_index)];
    }

    gb_rgb_t get_cgb_color(uint8_t palette_index,uint8_t color_index) const noexcept override {
        return colors[get_cgb_address_color(palette_index,color_index)];
    }

    gb_rgb_t get_cgb_dmg_color(uint8_t palette_index,uint8_t color_index) const noexcept override {
        return colors[get_dmg_address_color(palette_index,color_index)];
    }

    void update_data(gb_palette_t* palette) noexcept override {
        bgp = palette->state.bgp;
        memcpy(colors,palette->state.bg_cram_converted,sizeof(colors));
    }
};

struct obj_palette_t : public palette_t {
    uint8_t obp[2] = {};
    gb_rgb_t colors[gb_cgb_colors] = {};

    obj_palette_t(SDL_Renderer* renderer):palette_t(renderer,true){}

    void clear();

    uint8_t get_dmg_address_color(uint8_t palette_index,uint8_t color_index) const noexcept override {
        return (obp[palette_index & 0x01] >> ((color_index & 0x03) << 0x01)) & 0x03;
    }

    uint8_t get_cgb_address_color(uint8_t palette_index,uint8_t color_index) const noexcept override {
        return ((palette_index & 0x07) << 0x02) | (color_index & 0x03);
    }

    uint8_t get_cgb_dmg_address_color(uint8_t palette_index,uint8_t color_index) const noexcept override {
        return ((palette_index & 0x01) << 0x02) | ((obp[palette_index & 0x01] >> ((color_index & 0x03) << 0x01)) & 0x03);
    }


    gb_rgb_t get_dmg_color(uint8_t palette_index,uint8_t color_index) const noexcept override {
        return dmg_colors[get_dmg_address_color(palette_index,color_index)];
    }

    gb_rgb_t get_cgb_color(uint8_t palette_index,uint8_t color_index) const noexcept override {
        return colors[get_cgb_address_color(palette_index,color_index)];
    }

    gb_rgb_t get_cgb_dmg_color(uint8_t palette_index,uint8_t color_index) const noexcept override {
        return colors[get_cgb_dmg_address_color(palette_index,color_index)];
    }

    void update_data(gb_palette_t* palette) noexcept override {
        obp[0] = palette->state.obp[0];
        obp[1] = palette->state.obp[1];
        memcpy(colors,palette->state.obj_cram_converted,sizeof(colors)); 
    }
};


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