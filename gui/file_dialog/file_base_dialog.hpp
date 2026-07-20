#pragma once

#include <gui/utils/utils.hpp>

class file_base_dialog_t {
protected:
    using file_dialog_callback_t = void (*)(void* userdata,std::filesystem::path);
    
    struct path_part_t {
        std::string name;
        std::filesystem::path path;

        path_part_t(std::string _name,std::filesystem::path _path):
        name(_name),
        path(_path)
        {}
    };

    struct directory_entry_t {
        std::string name;
        std::filesystem::path path;
        uintmax_t size;
        std::time_t last_write_time;
        bool is_directory;

        directory_entry_t(std::string _name,std::filesystem::path _path,uintmax_t _size,std::time_t _last_write_time):
        name(_name),
        path(_path),
        size(_size),
        last_write_time(_last_write_time)
        {}
    };
    
    std::filesystem::path current_path;
    std::vector<path_part_t> current_path_parts;
    std::vector<directory_entry_t> current_directory_entries;
    
    std::chrono::steady_clock::time_point last_update;

    char name_buffer[256] = {0};
    int name_buffer_length = 0;

    uint8_t sort_column_index = 0;
    bool sort_ascending = true;

    int window_remaining_content_height;
    ImVec2 window_min;
    ImVec2 window_max;
    ImVec2 window_start_pos;

    const char** extensions;
    int extensions_count;
    int current_extension;

    file_dialog_callback_t callback;
    void* userdata;

    bool request_open = false;
    bool open = false;

    void set_current_path(std::filesystem::path path);

    std::uintmax_t number_of_entries_in_directory(const std::filesystem::path directory);

    std::time_t get_entry_last_write_time(const std::filesystem::path entry);
    
    const char* get_entry_date_formated(const directory_entry_t& entry);

    const char* get_current_extension() const noexcept;

    virtual bool is_current_extension(std::string extension) const noexcept = 0;

    void sort_current_directory_entries();

    void load_current_directory_entries();

    std::filesystem::path get_name_buffer_formated();

    virtual void send_name_buffer() = 0;
    
    virtual void select_file(std::filesystem::path& path) = 0;

    void update_directory_entries();

    virtual void render_popup_modal() = 0;

    void render_current_path_parts();
    void render_directory();
    void render_browser_table(ImVec2 size);
    void render_name_input();
    void render_extension_combo();
    void render_send_and_cancel(const char* send,const char* cancel);
    void render(const char* name,const char* send,const char* cancel);

    void clear_name_buffer(){
        name_buffer[0] = '\0';
        name_buffer_length = 0;
    }
    
public:
    file_base_dialog_t();

    void copy_to_name_buffer(const char* src) noexcept {
        size_t len = strlen(src);
        if(len >= sizeof(name_buffer)) return;
        strcpy(name_buffer,src);
        name_buffer_length = len;
    }

    void set_current_extension(int _current_extension) noexcept {
        if(_current_extension < 0 || _current_extension >= extensions_count) return;
        current_extension = _current_extension;
    }

    void set_extensions(const char** _extensions,int _extensions_count) noexcept {
        extensions = _extensions;
        extensions_count = _extensions_count;
    }

    void remove_extensions() noexcept {
        extensions = nullptr;
        extensions_count = 0;
    }

    void set_callback(file_dialog_callback_t _callback,void* _userdata) noexcept {
        callback = _callback;
        userdata = _userdata;
    }

    void remove_callback() noexcept {
        callback = nullptr;
        userdata = nullptr;
    }

    void set_open(bool _open) noexcept {
        if(open == _open) return;

        if(_open){
            request_open = true;
        }
        else{
            open = false;
            clear_name_buffer();
        }
    }

    bool get_open() const noexcept {
        return open;
    }
};