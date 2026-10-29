#pragma once

#include <gui/utils/utils.hpp>

struct gba_palette_panel_t {
    enum{
        texture_width = 128,
        texture_height = 128,
        texture_format = SDL_PIXELFORMAT_RGB24,
        texture_bytes_per_pixel = SDL_BYTESPERPIXEL(texture_format),
        texture_access = SDL_TEXTUREACCESS_STREAMING
    };

    SDL_Texture* texture = nullptr;
    uint16_t colors[256] = {};

    void init(SDL_Renderer* renderer);

    void uninit();

    void update_data(const uint8_t* src);

    void update_texture();

    void clear();
};