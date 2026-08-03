#pragma once

#include <gui/utils/utils.hpp>

class input_t {
private:
    enum{
        controller_binding_none,
        controller_binding_button,
        controller_binding_axis,
        controller_binding_count
    };

    enum{
        binding_none,
        binding_keyboard,
        binding_controller,
        binding_count
    };

    enum{
        controller_axis_deadzone = 8000
    };

    struct controller_binding_t{
        
        int type;

        union{            
            uint8_t button;

            struct{
                uint8_t index;
                bool negative;
            }axis;
        };
    };

    SDL_Scancode keyboard_bindings[gb_button_count] = {};
    SDL_Scancode temp_keyboard_bindings[gb_button_count] = {};

    controller_binding_t controller_bindings[gb_button_count] = {};
    controller_binding_t temp_controller_bindings[gb_button_count] = {};
    
    std::list<SDL_GameController*> controller_devices;
    SDL_GameController* current_controller = nullptr;

    ImVec2 window_min_size;
    ImVec2 window_max_size;
    
    int binding_type = input_t::binding_none;
    int binding_button = -1;

    bool open = false;

    void update_window_size_constraints();

    void clear_bindings();

    int get_controller_binding_type_from_string(const char* string);
    const char* get_controller_binding_type_string(int type);

    void save_keyboard_bindings(cJSON* input_object);
    void save_controller_bindings(cJSON* input_object);

    void load_keyboard_bindings(cJSON* input_object);
    void load_controller_bindings(cJSON* input_object);
public:

    input_t();

    ~input_t();

    void save(cJSON* settings_object);

    void load(cJSON* settings_object);

    bool button_pressed(gb_joypad_button_t button);

    void event_settings(SDL_Event& event);

    void render_settings();


    void open_settings() noexcept;

    void close_settings(bool discard_changes) noexcept;

    bool get_open() const noexcept {
        return open;
    }
};