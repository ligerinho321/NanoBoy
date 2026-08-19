#pragma once

#include <gui/utils/utils.hpp>

class debugger_t {
private:
    enum breakpoint_popup_type_t{
        breakpoint_popup_add_type,
        breakpoint_popup_edit_type
    };

    gb_t* gb;

    bool request_open_breakpoint_popup = false;
    
    bool breakpoint_popup_open = false;
    
    uint8_t breakpoint_popup_type;
    
    ImVec2 breakpoint_popup_start_pos;
    ImVec2 breakpoint_popup_min_size;
    ImVec2 breakpoint_popup_max_size;
    
    size_t breakpoint_max_address;
    float breakpoint_max_address_text_width;
    char breakpoint_max_address_text[32];

    gb_breakpoint_t temp_breakpoint;

    std::list<gb_breakpoint_t*> breakpoints;

    gb_breakpoint_t* breakpoint_selected = nullptr;
    
    bool open = false;
    
    void add_breakpoint();
    void edit_breakpoint();
    void delete_breakpoint();

    void update_breakpoint_popup_size_constraints();
    void update_breakpoint_max_address();
    void open_breakpoint_popup(uint8_t type);
    void render_breakpoint_popup();
    void render_breakpoints();

public:
    debugger_t(gb_t* gb);
    ~debugger_t();

    void render_registers();

    void render_flags();

    void render_others();
    
    void render_disassembly();

    void render();

    void set_open(bool _open) noexcept;

    bool get_open() const noexcept {
        return open;
    }
};