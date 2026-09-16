#pragma once

#include <gui/utils/utils.hpp>
#include <gui/file_dialog/file_selector_dialog.hpp>

class boot_settings_t {
private:
    enum{
        buffer_length = 256
    };

    gb_t* gb = nullptr;

    file_selector_t file_selector;

    char dmg_path[boot_settings_t::buffer_length] = {0};
    char cgb_path[boot_settings_t::buffer_length] = {0};
    bool skip_enabled = false;

    char temp_dmg_path[boot_settings_t::buffer_length] = {0};
    char temp_cgb_path[boot_settings_t::buffer_length] = {0};
    bool temp_skip_enabled = false;

    ImVec2 window_min_size;
    ImVec2 window_max_size;

    bool _open = false;

    void update_window_size_constraints();

    static void file_selector_dmg_callback(void* userdata,std::filesystem::path path);
    static void file_selector_cgb_callback(void* userdata,std::filesystem::path path);

public:
    void init(gb_t* _gb);
    void uninit();

    void save(cJSON* object);
    void load(cJSON* object);
    
    void render();

    void open() noexcept;

    void close(bool discard_changes) noexcept;
};