#pragma once

#include <gui/utils/utils.hpp>
#include <gui/debug/gba/palette_panel.hpp>
#include <gui/file_dialog/file_save_dialog.hpp>

class gba_palette_viewer_t {
private:
    gba_t* gba = nullptr;

    file_save_dialog_t file_save;

    gba_palette_panel_t bg_palette;
    gba_palette_panel_t obj_palette;

    int current_palette_index = 0;

    ImVec2 palette_size;

    float palette_scale = image_min_scale;

    int input_scalar_step = 1;
    int input_scalar_step_fast = 100;
    float input_scalar_width = 0.0f;

    bool show_grid = false;

    uint32_t grid_color = 0;
    uint32_t border_color = 0;
    uint32_t border_hovered_color = 0;

    gba_ppu_handler_t ppu_handler = {
        .callback = ppu_callback,
        .userdata = this,
        .scanline = gba_vblank_start_scanline,
        .cycle = 0,
        .next = nullptr
    };

    bool open = false;

    static void ppu_callback(void* userdata);

    static void file_save_callback(void* userdata,std::filesystem::path path);

    void update_palette_size();

    void event();

    void render_tooltip_color(gba_palette_panel_t& palette,int col,int raw);
    void render_palette(const char* id,gba_palette_panel_t& palette);

public:
    void init(gba_t* _gba,SDL_Renderer* renderer);
    void uninit();

    void render();

    void clear();

    void set_open(bool _open) noexcept;

    bool get_open() const noexcept {
        return open;
    }
};