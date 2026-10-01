#pragma once

#include <gui/utils/utils.hpp>

class shortcut_settings_t {
public:
    enum shortcut_id_t{
        open_file,

        take_screenshot,

        savestate_slot1,
        savestate_slot2,
        savestate_slot3,
        savestate_slot4,
        savestate_slot5,
        savestate_slot6,
        savestate_slot7,
        savestate_slot8,
        savestate_slot9,
        savestate_slot10,

        loadstate_slot1,
        loadstate_slot2,
        loadstate_slot3,
        loadstate_slot4,
        loadstate_slot5,
        loadstate_slot6,
        loadstate_slot7,
        loadstate_slot8,
        loadstate_slot9,
        loadstate_slot10,

        savestate_menu,

        exit,

        pause,
        reset,
        increase_speed,
        decrease_speed,
        rewind_held,
        cheats,
        printer,
        power_off,

        set_scale_1x,
        set_scale_2x,
        set_scale_3x,
        set_scale_4x,
        set_scale_5x,
        set_scale_6x,
        set_scale_7x,
        set_scale_8x,
        set_scale_9x,
        toggle_fullscreen,

        toggle_floating,
        toggle_aspect_ratio,
        toggle_interger_scale,
        toggle_interframe_blending,
        toggle_bilinear_filtering,
        toggle_disable_background,
        toggle_disable_objects,

        dmg_palette_settings,
        boot_settings,
        rewind_settings,
        input_settings,
        shortcut_settings,

        debugger,
        register_viewer,
        event_viewer,
        memory_viewer,
        tilemap_viewer,
        tile_viewer,
        object_viewer,
        palette_viewer,
        wave_form,

        id_count
    };
    
    struct shortcut_t {
        keyboard_binding_t keyboard;
        std::string keyboard_string;

        controller_binding_t controller;
        std::string controller_string;
    };

    struct shortcut_state_t {
        binding_state_t keyboard;
        binding_state_t controller;
    };

private:
    std::array<shortcut_t,shortcut_settings_t::id_count> shortcuts = {};
    
    std::array<shortcut_state_t,shortcut_settings_t::id_count> shortcuts_state = {};

    std::array<shortcut_t,shortcut_settings_t::id_count> temp_shortcuts = {};

    binding_capture_popup_t popup_capture;

    bool capture_is_keyboard = false;
    int capture_shortcut = 0;

    bool _open = false;

    void load_default();
public:

    void save(cJSON* object);
    void load(cJSON* object);

    void init_binding_frame();
    void process_binding_event(SDL_Event& event);
    void process_capture_event(SDL_Event& event);

    void render();

    void open() noexcept;
    void close(bool discard_changes) noexcept;
    
    const char* str_keyboard(int id) const noexcept {
        return shortcuts[id].keyboard_string.c_str();
    }

    const char* str_controller(int id) const noexcept {
        return shortcuts[id].controller_string.c_str();
    }

    bool down(int id) const noexcept {
        const shortcut_state_t& state = shortcuts_state[id];
        return state.keyboard.down || state.controller.down;
    }

    bool pressed(int id) const noexcept {
        const shortcut_state_t& state = shortcuts_state[id];
        return state.keyboard.pressed || state.controller.pressed;
    }

    bool released(int id) const noexcept {
        const shortcut_state_t& state = shortcuts_state[id];
        return state.keyboard.released || state.controller.released;
    }

    bool get_open() const noexcept {
        return _open;
    }
};