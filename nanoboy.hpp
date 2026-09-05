#pragma once

#include <core/gb.h>

#include <gui/assets/assets.hpp>

#include <gui/cheats/cheats.hpp>
#include <gui/file_dialog/file_selector_dialog.hpp>
#include <gui/printer/printer.hpp>
#include <gui/savestate/savestate.hpp>
#include <gui/screen/screen.hpp>

#include <gui/boot_settings/boot_settings.hpp>
#include <gui/input_settings/input_settings.hpp>

#include <gui/debugger/debugger.hpp>
#include <gui/event_viewer/event_viewer.hpp>
#include <gui/memory_viewer/memory_viewer.hpp>
#include <gui/tilemap_viewer/tilemap_viewer.hpp>
#include <gui/tile_viewer/tile_viewer.hpp>
#include <gui/object_viewer/object_viewer.hpp>
#include <gui/palette_viewer/palette_viewer.hpp>
#include <gui/wave_form/wave_form.hpp>

#include <gui/utils/utils.hpp>

class nanoboy_t {
private:
    enum{
        max_recent_roms = 10
    };

    std::filesystem::path main_folder_path;
    std::filesystem::path saves_path;
    std::filesystem::path savestates_path;
    std::filesystem::path cheats_path;
    std::filesystem::path screenshot_path;

    std::filesystem::path rom_path;
    std::string rom_name;

    std::list<std::string> recent_roms;

    std::string get_rom_save_path() const {
        std::filesystem::path path = saves_path / (rom_name + ".s");
        return path.u8string();
    }

    std::string get_rom_rtc_path() const {
        std::filesystem::path path = saves_path / (rom_name + ".rtc");
        return path.u8string();
    }

    std::string get_rom_cheat_path() const {
        std::filesystem::path path = cheats_path / (rom_name + ".json");
        return path.u8string();
    }

    std::string get_imgui_ini_path() const {
        std::filesystem::path path = main_folder_path / "imgui.ini";
        return path.u8string();
    }

    std::string get_settings_path() const {
        std::filesystem::path path = main_folder_path / "settings.json";
        return path.u8string();
    };

    void init_directories();
    void init_sdl();
    void init_imgui();

    void save_window_settings(cJSON* settings_object);
    void load_window_settings(cJSON* settings_object);

    void save_recent_roms(cJSON* settings_object);
    void load_recent_roms(cJSON* settings_object);

    void save_settings();
    void load_settings();
    
    void save_imgui_ini_settings();
    void load_imgui_ini_settings();

    void take_screenshot();

    void event();

    void gb_run();

public:
    gb_t* gb = nullptr;
    
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    SDL_AudioDeviceID audio_device = 0;

    boot_settings_t* boot_settings = nullptr;
    input_settings_t* input_settings = nullptr;
    
    file_selector_t* file_selector = nullptr;
    savestate_t* savestate = nullptr;

    screen_t* screen = nullptr;
    
    cheats_t* cheats = nullptr;
    printer_t* printer = nullptr;

    debugger_t* debugger = nullptr;
    event_viewer_t* event_viewer = nullptr;
    memory_viewer_t* memory_viewer = nullptr;
    tilemap_viewer_t* tilemap_viewer = nullptr;
    tile_viewer_t* tile_viewer = nullptr;
    object_viewer_t* object_viewer = nullptr;
    palette_viewer_t* palette_viewer = nullptr;
    wave_form_t* wave_form = nullptr;

    bool running = false;

    nanoboy_t();
    ~nanoboy_t();

    void insert_cartridge(std::filesystem::path path);
    void remove_cartridge();
    void set_speed(float speed);
    void pause();
    void reset();

    void render_main_menu_bar();
    void imgui_render();
    void sdl_render();

    void run();
};
