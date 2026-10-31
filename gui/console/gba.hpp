#pragma once

#include <gui/console/console.hpp>

#include <gui/debug/gba/memory_viewer.hpp>
#include <gui/debug/gba/palette_viewer.hpp>

class gba_console_t : public console_t {
private:
    gba_t* gba = nullptr;

    gba_memory_viewer_t memory_viewer;
    gba_palette_viewer_t palette_viewer;
    
    static void keypad_callback(void* data,uint8_t* status);
    
public:
    gba_console_t(nanoboy_t* _nanoboy);
    
    ~gba_console_t();
    
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
        gba_unused(enabled);
    }


    size_t read_samples(uint8_t* data,int len) override {
        gba_unused(data);
        gba_unused(len);
        return 0;
    }
    

    void open_debugger() override {

    }

    void open_register_viewer() override {

    }

    void open_event_viewer() override {

    }

    void open_memory_viewer() override {
        memory_viewer.set_open(true);
    }

    void open_tilemap_viewer() override {

    }

    void open_tile_viewer() override {

    }

    void open_object_viewer() override {

    }

    void open_palette_viewer() override {
        palette_viewer.set_open(true);
    }

    void open_wave_form() override {

    }

    
    void open_cheats() override {

    }
    
    
    bool cartridge_inserted() const noexcept override {
        return gba->cartridge_inserted;
    }

    float fps() const noexcept override {
        return 0.0f;
    }

    int screen_width() const noexcept override {
        return gba_screen_width;
    }

    int screen_height() const noexcept override {
        return gba_screen_height;
    }

    int screen_bytes_per_pixel() const noexcept override {
        return gba_screen_bytes_per_pixel;
    }

    int screen_pitch() const noexcept override {
        return gba_screen_pitch;
    }

    int screen_length() const noexcept override {
        return gba_screen_length;
    }

    int type() const noexcept override {
        return console_gba;
    }

    void* data() noexcept override {
        return gba;
    }

    
    void execute_frame() override;
    
    void render_imgui_windows() override;
};