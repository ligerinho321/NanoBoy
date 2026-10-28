#pragma once

#include <gb/gb.h>

#include <gui/assets/assets.hpp>

#include <gui/notification_manager/notification_manager.hpp>

#include <gui/console/gb.hpp>
#include <gui/console/gba.hpp>

#include <gui/file_dialog/file_selector_dialog.hpp>
#include <gui/savestate/savestate.hpp>
#include <gui/screen/screen.hpp>

#include <gui/dmg_palette/dmg_palette.hpp>
#include <gui/boot_settings/boot_settings.hpp>
#include <gui/rewind_settings/rewind_settings.hpp>
#include <gui/input_settings/input_settings.hpp>
#include <gui/shortcut_settings/shortcut_settings.hpp>

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

    mouse_cursor_t cursor;

    static void audio_callback(void* userdata,uint8_t* data,int len);
    static void file_selector_callback(void* userdata,std::filesystem::path path);

    void init_directories();
    void init_sdl();
    void init_imgui();

    void update_gb_model();

    void save_window_settings(cJSON* settings_object);
    void load_window_settings(cJSON* settings_object);

    void save_recent_roms(cJSON* settings_object);
    void load_recent_roms(cJSON* settings_object);

    void save_settings();
    void load_settings();
    
    void save_imgui_ini_settings();
    void load_imgui_ini_settings();

    void take_screenshot();

    void file_shortcut_event();
    void game_shortcut_event();
    void settings_shortcut_event();
    void debug_shortcut_event();
    void tools_shortcut_event();

    void event();

public:
    console_t* console;
    
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    SDL_AudioDeviceID audio_device = 0;

    notification_manager_t notification_manager;
    
    dmg_palette_t dmg_palette;
    boot_settings_t boot_settings;
    rewind_settings_t rewind_settings;
    input_settings_t input_settings;
    shortcut_settings_t shortcut_settings;
    
    file_selector_t file_selector;

    savestate_t savestate;

    screen_t screen;

    bool is_cgb = false;
    
    bool running = false;

    nanoboy_t();
    ~nanoboy_t();

    void insert_cartridge(std::filesystem::path path);
    void remove_cartridge();
    void increase_speed();
    void decrease_speed();
    void pause();
    void reset();

    void render_main_menu_bar();
    void imgui_render();
    void sdl_render();

    void run();

private:
    std::string get_rom_save_path() const {
        std::filesystem::path path = saves_path / (rom_name + ".s");
        return path.u8string();
    }

    std::string get_rom_rtc_path() const {
        std::filesystem::path path = saves_path / (rom_name + ".rtc");
        return path.u8string();
    }

    std::string get_rom_cheats_path() const {
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
};
