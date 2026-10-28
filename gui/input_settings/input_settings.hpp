#pragma once

#include <gui/utils/utils.hpp>

struct input_profile_t {
    const char** names = nullptr;
    int count = 0;

    SDL_Scancode* keyboard_bindings;
    SDL_Scancode* temp_keyboard_bindings;
    binding_state_t* keyboard_bindings_state;

    controller_binding_t* controller_bindings;
    controller_binding_t* temp_controller_bindings;
    binding_state_t* controller_bindings_state;

    void init(const char** _names,int _count);
    void uninit();
    void clear_bindings_state();
};

class input_settings_t {
private:
    input_profile_t profiles[console_count];

    binding_capture_popup_t popup_capture;

    input_profile_t* capture_profile = nullptr;
    bool capture_is_keyboard = false;
    int capture_binding = 0;

    input_profile_t* current_profile = nullptr;

    bool _open = false;

    void load_default();

    void save_keyboard_bindings(input_profile_t& profile,cJSON* console_object);
    void save_controller_bindings(input_profile_t& profile,cJSON* console_object);

    void load_keyboard_bindings(input_profile_t& profile,cJSON* console_object);
    void load_controller_bindings(input_profile_t& profile,cJSON* console_object);

    void render_profile(input_profile_t* profile);
public:
    void init();
    void uninit();

    void save(cJSON* object);
    void load(cJSON* object);


    void set_current_console(int console){
        current_profile = profiles + console;
        current_profile->clear_bindings_state();
    }

    bool down(int button){
        return (
            current_profile != nullptr &&
            (current_profile->keyboard_bindings_state[button].down || 
            current_profile->controller_bindings_state[button].down)
        );
    }

    bool pressed(int button){
        return (
            current_profile != nullptr &&
            (current_profile->keyboard_bindings_state[button].pressed || 
            current_profile->controller_bindings_state[button].pressed)
        );
    }

    bool released(int button){
        return (
            current_profile != nullptr &&
            (current_profile->keyboard_bindings_state[button].released ||
            current_profile->controller_bindings_state[button].released)
        );
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