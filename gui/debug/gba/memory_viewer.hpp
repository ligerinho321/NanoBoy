#pragma once

#include <gui/utils/utils.hpp>
#include <gui/debug/memory_viewer_base.hpp>

class gba_memory_viewer_t : public memory_viewer_base_t {
private:
    gba_t* gba = nullptr;

    int current_memory_type = gba_memory_cpu_type;

    static void file_selector_callback(void* userdata,std::filesystem::path path);
    static void file_save_callback(void* userdata,std::filesystem::path path);
    
    size_t get_current_memory_length() override;

    void write_byte(uint8_t value,size_t address) override;

    uint8_t read_byte(size_t address) override;

    bool cartridge_inserted() override;

    void render_memory_type_combo() override;

public:
    void init(gba_t* _gba);

    void clear(){
        current_memory_type = gba_memory_cpu_type;
        memory_viewer_base_t::clear();
    }
};