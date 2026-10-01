#pragma once

#include <gui/utils/utils.hpp>

class input_settings_t {
private:
    std::array<SDL_Scancode,gb_button_count> keyboard_bindings = {};
    std::array<controller_binding_t,gb_button_count> controller_bindings = {};

    std::array<SDL_Scancode,gb_button_count> temp_keyboard_bindings = {};
    std::array<controller_binding_t,gb_button_count> temp_controller_bindings = {};
    
    std::array<binding_state_t,gb_button_count> keyboard_bindings_state = {};
    std::array<binding_state_t,gb_button_count> controller_bindings_state = {};

    binding_capture_popup_t popup_capture;

    bool capture_is_keyboard = false;
    int capture_binding = 0;

    bool _open = false;

    void save_keyboard_bindings(cJSON* input_settings_object);
    void save_controller_bindings(cJSON* input_settings_object);

    void load_keyboard_bindings(cJSON* input_settings_object);
    void load_controller_bindings(cJSON* input_settings_object);

public:
    
    void save(cJSON* object);
    void load(cJSON* object);

    bool down(gb_joypad_button_t button){
        return keyboard_bindings_state[button].down || controller_bindings_state[button].down;
    }

    bool pressed(gb_joypad_button_t button){
        return keyboard_bindings_state[button].pressed || controller_bindings_state[button].pressed;
    }

    bool released(gb_joypad_button_t button){
        return keyboard_bindings_state[button].released || controller_bindings_state[button].released;
    }

    void init_binding_frame();
    void process_binding_event(SDL_Event& event);
    void process_capture_event(SDL_Event& event);

    void render();

    void open() noexcept;

    void close(bool discard_changes) noexcept;

    bool get_open() const noexcept {
        return _open;
    }
};