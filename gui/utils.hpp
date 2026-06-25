#pragma once

#include "../core/gb.h"

#include "../imgui/imgui.h"
#include "../imgui/imgui_impl_sdl2.h"
#include "../imgui/imgui_impl_sdlrenderer2.h"

#include <SDL2/SDL.h>

#include <iostream>
#include <chrono>
#include <filesystem>
#include <vector>
#include <algorithm>
#include <string>
#include <regex>

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
    bool is_obj = false;

    palette_t(SDL_Renderer* renderer,bool _is_obj):
    is_obj(_is_obj),
    texture(SDL_CreateTexture(renderer,texture_format,texture_access,texture_max_width,texture_max_height))
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


    void update_texture(gb_type_t type,bool cgb_mode);
};

struct bg_palette_t : public palette_t {
    uint8_t bgp = 0;
    gb_rgb_t colors[gb_cgb_colors] = {0};

    bg_palette_t(SDL_Renderer* renderer):palette_t(renderer,false){}


    uint8_t get_dmg_address_color(uint8_t palette_index, uint8_t color_index) const noexcept override {
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
};

struct obj_palette_t : public palette_t {
    uint8_t obp[2] = {0};
    gb_rgb_t colors[gb_cgb_colors] = {0};

    obj_palette_t(SDL_Renderer* renderer):palette_t(renderer,true){}

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
};


float get_input_scalar_width();

bool mouse_in_rect(ImVec2 m,ImVec2 rmin,ImVec2 rmax);