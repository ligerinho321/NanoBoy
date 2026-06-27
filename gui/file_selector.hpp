#pragma once

#include "utils.hpp"

class nanoboy_t;

class file_selector_t {
public:
    enum{
        gigabytes = 0x01 << 0x1E,
        megabytes = 0x01 << 0x14,
        kilobytes = 0x01 << 0x0A
    };

    enum filter_type_t {
        filter_all_files,
        filter_gb_rom_files
    };

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

        directory_entry_t(std::string _name,std::string _path,uintmax_t _size,std::time_t _last_write_time,bool _is_directory):
        name(_name),
        path(_path),
        size(_size),
        last_write_time(_last_write_time),
        is_directory(_is_directory)
        {}
    };

    nanoboy_t* nanoboy = nullptr;

    bool opened = false;
        
    std::filesystem::path current_path;
    std::vector<path_part_t> current_path_parts;
    std::vector<directory_entry_t> current_directory_entries;
    
    std::chrono::steady_clock::time_point last_update;

    uint8_t sort_column_index;
    bool sort_ascending;

    char name_buffer[256] = {0};
    int current_filter = filter_gb_rom_files;

    bool popup_opened;
    ImVec2 popup_pos;
    std::filesystem::path path_not_exists;

    int window_remaining_content_height;
    ImVec2 window_min;
    ImVec2 window_max;

    file_selector_t(nanoboy_t* nanoboy);


    void set_current_path(std::filesystem::path path);

    uintmax_t number_of_entries_in_directory(std::filesystem::path directory);

    std::time_t entry_last_write_time(std::filesystem::path entry);

    void sort_current_directory_entries();

    void load_current_directory_entries();

    const char* entry_date_formated(const directory_entry_t& entry);


    void render_directory();

    void render();
    
};