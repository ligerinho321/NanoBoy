#pragma once

#include <gui/file_dialog/file_base_dialog.hpp>

class file_save_dialog_t : public file_base_dialog_t {
private:
    bool request_open_popup_modal = false;
    bool popup_modal_open = false;
    ImVec2 popup_modal_start_pos;
    std::string overwrite_file_name;
    std::filesystem::path overwrite_file_path;

    bool is_current_extension(std::string extension) const noexcept override;

    bool is_extension_supported(std::string extension) const noexcept;

    void format_path_extension(std::filesystem::path& path);

    void select_file(std::filesystem::path& path) override;

    void send_name_buffer() override;

    void render_popup_modal() override;

public:
    void render(){
        file_base_dialog_t::render("File Save","Save","Cancel");
    }
};