#pragma once

#include <gui/utils/utils.hpp>

class rewind_settings_t {
private:
    nanoboy_t* nanoboy = nullptr;

    bool enabled;
    bool temp_enabled;

    uint32_t buffer_frames;
    uint32_t temp_buffer_frames;

    uint32_t recording_interval;
    uint32_t temp_recording_interval;

    uint32_t speed;
    uint32_t temp_speed;

    ImVec2 window_min_size{};
    ImVec2 window_max_size{};

    bool _open = false;

    void update_window_size_constraints();

    void load_default();
    
public:
    void init(nanoboy_t* _nanoboy);

    void apply_to_console();

    void save(cJSON* object);
    void load(cJSON* object);

    void render();

    void open() noexcept;

    void close(bool discard_changes) noexcept;
};