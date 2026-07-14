#pragma once

#include <gui/file_dialog/file_base_dialog.hpp>

class file_save_dialog_t : public file_base_dialog_t {
private:
    const char** formats = nullptr;
    int formats_count = 0;
    int current_format = 0;

    bool request_open_popup_modal = false;
    bool popup_modal_open = false;
    ImVec2 popup_modal_start_pos;
    std::string overwrite_file_name;
    std::filesystem::path overwrite_file_path;

    const char* get_current_extension() const noexcept;

    bool extension_supported(std::string extension);

    void format_path_extension(std::filesystem::path& path);

    void load_current_directory_entries() override;

    void send_name_buffer() override;

    void render_directory() override;

    void render_popup_modal();

public:
    void set_formats(const char** _formats,int _formats_count) noexcept {
        formats = _formats;
        formats_count = _formats_count;
    }

    void remove_formats() noexcept {
        formats = nullptr;
        formats_count = 0;
    }

    void render();
};