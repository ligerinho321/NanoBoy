#pragma once

#include <gui/file_dialog/file_base_dialog.hpp>

class nanoboy_t;

class file_selector_t : public file_base_dialog_t {
private:
    const char** filters = nullptr;
    int filters_count = 0;
    int current_filter = 0;

    bool extension_is_valid(std::string extension) const noexcept;

    void load_current_directory_entries() override;

    void send_name_buffer() override;

    void render_directory() override;

public:
    void set_filters(const char** _filters,int _filters_count) noexcept {
        filters = _filters;
        filters_count = _filters_count;
    }

    void remove_filters() noexcept {
        filters = nullptr;
        filters_count = 0;
    }

    void render();  
};