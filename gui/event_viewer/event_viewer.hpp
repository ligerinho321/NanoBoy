#pragma once

#include <gui/utils/utils.hpp>

class event_viewer_t {
private:
    enum {
        texture_width = gb_scanline_cycles,
        texture_height = gb_scanlines,
        texture_format = SDL_PIXELFORMAT_RGB24,
        texture_access = SDL_TEXTUREACCESS_STREAMING
    };

    struct config_t {
        ImVec4 write_color;
        ImVec4 read_color;
        ImVec4 interrupt_color;
        int flags;
    };

    gb_t* gb;

    SDL_Texture* texture = nullptr;

    ImVec2 texture_size;

    const float min_scale = 1.0f;
    const float max_scale = 10.0f;
    float scale = min_scale;

    std::string event_infos[gb_event_type_count];

    config_t event_configs[gb_event_type_count] = {
        [gb_event_none_type] = {},
        
        [gb_event_halt_type] = {{},{},{0.62f, 0.62f, 0.62f, 1.00f},0},
        [gb_event_stop_type] = {{},{},{1.00f, 0.83f, 0.49f, 1.00f},0},
        
        [gb_event_irq_vblank_type] = {{},{},{0.31f, 0.76f, 0.96f, 1.00f},0},
        [gb_event_irq_lcd_type]    = {{},{},{0.67f, 0.49f, 1.00f, 1.00f},0},
        [gb_event_irq_timer_type]  = {{},{},{1.00f, 0.71f, 0.30f, 1.00f},0},
        [gb_event_irq_serial_type] = {{},{},{0.30f, 0.81f, 0.88f, 1.00f},0},
        [gb_event_irq_joypad_type] = {{},{},{0.94f, 0.38f, 0.57f, 1.00f},0},

        [gb_event_vram_type] = {{0.08f, 0.42f, 0.80f, 1.00f},{0.40f, 0.71f, 0.97f, 1.00f},{},0},
        [gb_event_wram_type] = {{0.18f, 0.55f, 0.22f, 1.00f},{0.50f, 0.78f, 0.52f, 1.00f},{},0},
        [gb_event_oam_type]  = {{0.75f, 0.10f, 0.35f, 1.00f},{0.96f, 0.56f, 0.70f, 1.00f},{},0},
        [gb_event_hram_type] = {{0.00f, 0.45f, 0.40f, 1.00f},{0.30f, 0.72f, 0.68f, 1.00f},{},0},

        [gb_event_joypad_type]       = {{0.76f, 0.20f, 0.38f, 1.00f},{0.95f, 0.60f, 0.69f, 1.00f},{},0},
        [gb_event_serial_type]       = {{0.08f, 0.52f, 0.60f, 1.00f},{0.40f, 0.82f, 0.87f, 1.00f},{},0},
        [gb_event_timer_type]        = {{0.90f, 0.45f, 0.08f, 1.00f},{1.00f, 0.78f, 0.45f, 1.00f},{},0},
        [gb_event_interrupt_type]    = {{0.45f, 0.25f, 0.65f, 1.00f},{0.76f, 0.58f, 0.90f, 1.00f},{},0},
        [gb_event_square1_type]      = {{0.10f, 0.40f, 0.90f, 1.00f},{0.35f, 0.65f, 1.00f, 1.00f},{},0},
        [gb_event_square2_type]      = {{0.15f, 0.65f, 0.40f, 1.00f},{0.45f, 0.85f, 0.65f, 1.00f},{},0},
        [gb_event_wave_type]         = {{0.50f, 0.25f, 0.85f, 1.00f},{0.75f, 0.55f, 1.00f, 1.00f},{},0},
        [gb_event_noise_type]        = {{0.90f, 0.45f, 0.10f, 1.00f},{1.00f, 0.70f, 0.35f, 1.00f},{},0},
        [gb_event_apu_control_type]  = {{0.85f, 0.20f, 0.35f, 1.00f},{1.00f, 0.55f, 0.65f, 1.00f},{},0},
        [gb_event_wave_ram_type]     = {{0.10f, 0.60f, 0.70f, 1.00f},{0.40f, 0.85f, 0.90f, 1.00f},{},0},
        [gb_event_ppu_type]          = {{0.25f, 0.33f, 0.70f, 1.00f},{0.50f, 0.58f, 0.90f, 1.00f},{},0},
        [gb_event_palette_type]      = {{0.70f, 0.15f, 0.15f, 1.00f},{0.95f, 0.45f, 0.45f, 1.00f},{},0},
        [gb_event_dma_type]          = {{0.80f, 0.25f, 0.08f, 1.00f},{1.00f, 0.52f, 0.32f, 1.00f},{},0},
        [gb_event_others_type]       = {{0.60f, 0.40f, 0.08f, 1.00f},{0.90f, 0.70f, 0.30f, 1.00f},{},0}
    };
    
    const gb_event_t** event_selected = nullptr;
    const gb_event_t* event_hovered = nullptr;

    std::vector<const gb_event_t*> event_list;
    bool _update_event_list_scroll = false;
    int event_list_start_previous_frame;

    uint32_t event_selected_border_color;
    bool border_on = false;
    std::chrono::steady_clock::time_point last_border_blink_time;

    bool show_previous_frame_event = false;

    struct{
        float color;
        float scanline;
        float cycle;
        float pc;
        float value;
        float previous_frame;
    } event_list_column_width;

    ImVec2 event_control_child_size;
    float event_control_label_column_width;

    bool open = false;

    void init_event_infos();

    void update_event_control_child_size();
    void update_event_list_column_width();

    void update_texture();
    void update_texture_size();

    void push_frame_in_list(const gb_event_frame_t* frame,int start_cycle,int end_cycle);
    void update_event_list();
    void update_event_list_scroll();

    void event();

    void render_pc_marker();
    
    void render_mouse_marker();

    void render_event_hovered_tooltip();

    void render_event_markers();

    void render_event_list();

    void render_event_info(int type);

    void render_interrupt_event_configs();
    void render_io_event_configs(const char* collapsing_header,const char* table,int start,int end);

    float event_list_item_height(){
        return ImGui::GetFrameHeight() + ImGui::GetStyle().CellPadding.y * 2.0f;
    }

public:

    event_viewer_t(gb_t* gb,SDL_Renderer* renderer);

    ~event_viewer_t();

    void render();

    void clear(){
        clear_texture(texture,event_viewer_t::texture_height);
        event_list.clear();
        event_selected = nullptr;
    }

    void reset(){
        event_selected = nullptr;
    }

    void set_open(bool _open) noexcept;

    bool get_open() const noexcept {
        return open; 
    }
};