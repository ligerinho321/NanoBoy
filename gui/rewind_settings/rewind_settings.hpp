#pragma once

#include <gui/utils/utils.hpp>

class rewind_settings_t {
private:
    gb_t* gb;

    bool temp_enabled;
    uint32_t temp_capacity;
    uint32_t temp_speed;

    ImVec2 window_min_size{};
    ImVec2 window_max_size{};

    bool _open = false;

    void update_window_size_constraints();
    
public:
    rewind_settings_t(gb_t* gb);

    void save(cJSON* object);
    void load(cJSON* object);

    void render();

    void open() noexcept;

    void close(bool discard_changes) noexcept;
};