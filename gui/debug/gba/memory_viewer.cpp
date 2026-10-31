#include <gui/debug/gba/memory_viewer.hpp>

void gba_memory_viewer_t::init(gba_t* _gba){
    gba = _gba;
    
    file_selector.set_callback(file_selector_callback,this);

    file_save.set_callback(file_save_callback,this);

    update_current_memory_type();

    memory_viewer_base_t::init();
}


void gba_memory_viewer_t::file_selector_callback(void* userdata,std::filesystem::path path){
    gba_memory_viewer_t* memory_viewer = (gba_memory_viewer_t*)userdata;
    gba_memory_type_import(memory_viewer->gba,memory_viewer->current_memory_type,path.u8string().c_str());
}

void gba_memory_viewer_t::file_save_callback(void* userdata,std::filesystem::path path){
    gba_memory_viewer_t* memory_viewer = (gba_memory_viewer_t*)userdata;
    gba_memory_type_export(memory_viewer->gba,memory_viewer->current_memory_type,path.u8string().c_str());
}


size_t gba_memory_viewer_t::get_current_memory_length(){
    return gba_memory_type_length(gba,current_memory_type);
}

void gba_memory_viewer_t::write_byte(uint8_t value,size_t address){
    gba_memory_type_write_byte(gba,current_memory_type,value,address);
}

uint8_t gba_memory_viewer_t::read_byte(size_t address){
    return gba_memory_type_read_byte(gba,current_memory_type,address);
}

bool gba_memory_viewer_t::cartridge_inserted(){
    return gba->cartridge_inserted;
}


void gba_memory_viewer_t::render_memory_type_combo(){
    if(ImGui::BeginCombo("Memory Type",gba_memory_type_names[current_memory_type])){
        for(int i = 0; i < gba_memory_type_count; ++i){
            if(
                (i == gba_memory_rom_type && !gba->cartridge.rom_length) ||
                (i == gba_memory_sram_type && !gba->cartridge.sram_length)
            ){
                continue;
            }

            if(ImGui::Selectable(gba_memory_type_names[i],current_memory_type == i)){
                current_memory_type = i;
                update_current_memory_type();
            }
        }
        ImGui::EndCombo();
    }
}
