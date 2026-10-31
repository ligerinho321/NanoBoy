#pragma once

#include <gui/console/console.hpp>

#include <gui/debug/gb/debugger.hpp>
#include <gui/debug/gb/register_viewer.hpp>
#include <gui/debug/gb/event_viewer.hpp>
#include <gui/debug/gb/memory_viewer.hpp>
#include <gui/debug/gb/tilemap_viewer.hpp>
#include <gui/debug/gb/tile_viewer.hpp>
#include <gui/debug/gb/object_viewer.hpp>
#include <gui/debug/gb/palette_viewer.hpp>
#include <gui/debug/gb/wave_form.hpp>

#include <gui/tools/gb/cheats.hpp>
#include <gui/tools/gb/printer.hpp>

class gb_console_t : public console_t {
private:
    gb_t* gb = nullptr;

    gb_debugger_t debugger;
    gb_register_viewer_t register_viewer;
    gb_event_viewer_t event_viewer;
    gb_memory_viewer_t memory_viewer;
    gb_tilemap_viewer_t tilemap_viewer;
    gb_tile_viewer_t tile_viewer;
    gb_object_viewer_t object_viewer;
    gb_palette_viewer_t palette_viewer;
    gb_wave_form_t wave_form;

    gb_cheats_t cheats;
    gb_printer_window_t printer;

    static void joypad_callback(void* data,gb_joypad_button_state_t* state);

public:
    gb_console_t(nanoboy_t* _nanoboy);

    ~gb_console_t();

    void update_screen_texture() override;

    bool take_screenshot(const char* path) override;
    
    bool insert_cartridge(const char* path) override;
    bool insert_cartridge_from_memory(uint8_t* rom,size_t length) override;
    void remove_cartridge() override;

    void increase_speed() override;
    void decrease_speed() override;
    void set_speed(float speed) override;
    float get_speed() override;
    void pause() override;
    bool paused() override;
    void reset() override;

    void save_ram(const char* path) override;
    void load_ram(const char* path) override;

    void save_rtc(const char* path) override;
    void load_rtc(const char* path) override;

    void save_cheats(const char* path) override;
    void load_cheats(const char* path) override;

    bool savestate_serialize(const char* path) override;
    bool savestate_deserialize(const char* path) override;
    bool savestate_get_info(const char* path,console_savestate_info_t* info) override;

    void set_rewind_enabled(bool enabled) override;
    void set_rewind_capacity(uint32_t capacity) override;
    void set_rewind_recording_interval(uint32_t interval) override;
    void set_rewind_frame_time(float time) override;
    bool start_rewind() override;
    bool end_rewind() override;
    
    void enable_interframe_blending(bool enabled) override {
        gb->ppu.interframe_blending = enabled;
    }


    size_t read_samples(uint8_t* data,int len) override {
        return gb_read_samples(gb,data,len);
    }


    void open_debugger() override {
        debugger.set_open(true);
    }
    
    void open_register_viewer() override {
        register_viewer.set_open(true);
    }

    void open_event_viewer() override {
        event_viewer.set_open(true);
    }

    void open_memory_viewer() override {
        memory_viewer.set_open(true);
    }

    void open_tilemap_viewer() override {
        tilemap_viewer.set_open(true);
    }
    
    void open_tile_viewer() override {
        tile_viewer.set_open(true);
    }

    void open_object_viewer() override {
        object_viewer.set_open(true);
    }

    void open_palette_viewer() override {
        palette_viewer.set_open(true);
    }

    void open_wave_form() override {
        wave_form.set_open(true,false);
    }


    void open_cheats() override {
        cheats.set_open(true);
    }

    void open_printer(){
        printer.set_open(true);
    }

    
    bool cartridge_inserted() const noexcept override {
        return gb->cartridge_inserted;
    }

    float fps() const noexcept override {
        return gb->frame_timer.fps;
    }

    int screen_width() const noexcept override {
        return gb_screen_width;
    }

    int screen_height() const noexcept override {
        return gb_screen_height;
    }

    int screen_bytes_per_pixel() const noexcept override {
        return gb_screen_bytes_per_pixel;
    }

    int screen_pitch() const noexcept override {
        return gb_screen_pitch;
    }

    int screen_length() const noexcept override {
        return gb_screen_length;
    }

    int type() const noexcept override {
        return console_gb;
    }

    void* data() noexcept override {
        return gb;
    }

    
    void execute_frame() override;

    void render_imgui_windows() override;
};