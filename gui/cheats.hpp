#pragma once

#include "utils.hpp"

class cheats_t {
private:
    enum {
        buffer_length = 128
    };

    enum format_type_t{
        format_game_genie_type = 0,
        format_game_shark_type = 1        
    };

    enum popup_type_t{
        popup_add_cheat_type = 0,
        popup_edit_cheat_type = 1
    };

    enum error_type_t{
        error_description_empty = 0,
        error_invalid_code_format = 1
    };

    struct cheat_t {
        char description_buffer[buffer_length];
        char code_buffer[buffer_length];
        uint8_t format_type;
        bool enabled;
        std::vector<gb_cheat_code_t> codes;
        cheat_t* next;
    };

    gb_t* gb = nullptr;

    bool open = false;

    bool popup_open = false;
    uint8_t popup_type = 0;
    ImVec2 popup_start_pos{0.0f,0.0f};
    ImVec2 popup_start_size{250.0f,200.0f};

    std::regex game_genie_pattern_text;
    std::regex game_genie_pattern_code;
    std::regex game_shark_pattern_text;
    std::regex game_shark_pattern_code;

    char description_buffer[buffer_length] = {0};
    char code_buffer[buffer_length] = {0};
    int code_buffer_length = 0;
    int format_type = 0;
    bool enabled = false;

    int current_error = 0;
    ImVec4 text_error_color{1.0f,0.0f,0.0f,1.0f};

    cheat_t* cheats = nullptr;
    cheat_t* cheat_selected = nullptr;

    void load_cheat_codes(cheat_t* cheat);

    bool cheat_is_valid();

    void copy_code_buffer(char* dst);

    void add_cheat();
    void edit_cheat();
    void delete_cheat_selected();
    void clear_cheats();

    void open_popup(int type);

    void render_popup();

public:
    cheats_t(gb_t* gb);

    ~cheats_t();

    void render();

    void set_open(bool _open){
        open = _open;
    }
    
    bool get_open() const {
        return open;
    }
};