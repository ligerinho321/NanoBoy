#pragma once

#include <gui/utils/utils.hpp>
#include <gui/file_dialog/file_save_dialog.hpp>
#include <gui/file_dialog/file_selector_dialog.hpp>

class memory_viewer_base_t {
protected:
    enum{
        min_columns = 4,
        max_columns = 128
    };
    
    file_selector_t file_selector;
    file_save_dialog_t file_save;

    uint32_t columns = min_columns;
    uint64_t rows = 0;

    float scroll_x = 0.0f;
    double scroll_y = 0.0f;

    uint64_t editing_address = 0;
    bool need_focus_editing_address = false;
    int buffer_cursor_pos = 0;
    char buffer[3] = {};

    bool jump_near = false;
    bool jump_far = false;

    size_t memory_length = 0;
    int address_digit_count = 0;

    ImGuiInputTextFlags input_text_flags;
    uint32_t line_color;
    uint32_t text_color;
    uint32_t text_disabled_color;

    bool open = false;

    static int input_text_callback(ImGuiInputTextCallbackData* data);

    virtual size_t get_current_memory_length() = 0;

    virtual void write_byte(uint8_t value,size_t address) = 0;
    virtual uint8_t read_byte(size_t address) = 0;

    void update_current_memory_type();

    void update_rows();

    virtual bool cartridge_inserted() = 0;

    virtual void render_memory_type_combo() = 0;

    void event();

    void render_row_hex(uint64_t address,float pair_width);
    void render_row_chr(uint64_t address);
    void render_dump();
    void render_control();

public:
    void init();
    
    void render();

    void clear();

    void set_open(bool _open) noexcept {
        open = _open;
    }

    bool get_open() const noexcept {
        return open;
    }
};