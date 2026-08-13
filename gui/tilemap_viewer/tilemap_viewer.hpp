#pragma once

#include <gui/utils/utils.hpp>

class tilemap_viewer_t {
private:
    enum{
        texture_format = SDL_PIXELFORMAT_RGB24,
        texture_bytes_per_pixel = SDL_BYTESPERPIXEL(texture_format),
        texture_access = SDL_TEXTUREACCESS_STREAMING,

        tilemap_texture_width = gb_tilemap_columns * gb_tile_size,
        tilemap_texture_height = gb_tilemap_rows * gb_tile_size,

        tooltip_tile_scale = 8,
        tooltip_palette_scale = 2
    };

    gb_t* gb = nullptr;

    SDL_Texture* tilemap_texture[2] = {nullptr};

    uint32_t grid_color = 0;
    uint32_t scroll_overlay_border_color = 0;
    uint32_t scroll_overlay_background_color = 0;
    uint32_t border_color = 0;
    uint32_t border_hovered_color = 0;

    bool show_tile_grid = false;
    bool show_scroll_overlay = false;

    const float tilemap_min_scale = 1.0f;
    const float tilemap_max_scale = 10.0f;
    float tilemap_scale = tilemap_min_scale;

    ImVec2 tilemap_size{0.0f,0.0f};

    float input_scalar_width = 0.0f;
    int input_scalar_step = 1;
    int input_scalar_step_fast = 100;

    bool cgb_mode = false;
    bool tiledata_area = false;
    uint8_t scx = 0;
    uint8_t scy = 0;
    bg_palette_t bg_palette;
    uint8_t vram[gb_vram_length] = {0};

    gb_ppu_handler_t callback_handler = {callback,this,gb_vblank_scanline,0,nullptr};

    bool open = false;

    static void callback(void* data);

    void update_tilemap_texture(uint8_t map_index);

    void event();
    
    void render_grid(ImVec2 tilemap_start,ImVec2 tilemap_end);
    void render_scroll_overlay(ImVec2 tilemap_start,ImVec2 tilemap_end);
    void render_tile_tooltip(bool tilemap,uint8_t col,uint8_t row);
    void render_tilemap(const char* str_id,bool tilemap);

    void update_tilemap_size(){
        tilemap_size.x = tilemap_texture_width * tilemap_scale;
        tilemap_size.y = tilemap_texture_height * tilemap_scale;
    }
    
public:

    tilemap_viewer_t(gb_t* gb,SDL_Renderer* renderer);

    ~tilemap_viewer_t();

    void render();

    void clear();

    void set_open(bool _open) noexcept;

    bool get_open() const noexcept {
        return open;
    }
};