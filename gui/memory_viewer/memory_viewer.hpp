#pragma once

#include <gui/utils/utils.hpp>
#include <gui/file_dialog/file_save_dialog.hpp>
#include <gui/file_dialog/file_selector_dialog.hpp>

class memory_viewer_t {
private:
    enum{
        min_columns = 4,
        max_columns = 128
    };

    gb_t* gb;

    file_selector_t file_selector;
    file_save_dialog_t file_save;

    int current_memory_type = gb_memory_cpu_type;

    int columns = min_columns;
    int rows = 0;

    ImU64 editing_address = 0;
    bool need_focus_editing_address = false;
    int buffer_cursor_pos = 0;
    char buffer[3] = {};

    bool update_scroll_y = false;

    size_t memory_length = 0;
    int address_digit_count = 0;

    ImGuiInputTextFlags input_text_flags;
    uint32_t line_color;
    uint32_t text_color;
    uint32_t text_disabled_color;

    bool open = false;

    static int input_text_callback(ImGuiInputTextCallbackData* data);
    static void file_selector_callback(void* userdata,std::filesystem::path path);
    static void file_save_callback(void* userdata,std::filesystem::path path);
    
    void update_current_memory_type();
    void update_rows();
public:

    memory_viewer_t(gb_t* gb);

    void render_control();

    void render();


    void set_open(bool _open) noexcept {
        open = _open;
    }

    bool get_open() const noexcept {
        return open;
    }
};