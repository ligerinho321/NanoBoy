#pragma once

#include <gui/file_dialog/file_base_dialog.hpp>

class file_selector_t : public file_base_dialog_t {
private:
    bool is_current_extension(std::string extension) const noexcept override;

    void select_file(std::filesystem::path& path) override;

    void send_name_buffer() override;

    void render_popup_modal() override {}
    
public:
    file_selector_t() = default;
    ~file_selector_t() = default;
    
    void render(){
        file_base_dialog_t::render("File Selector","Ok","Cancel");
    }
};