#include <gui/debug/gb/memory_viewer.hpp>

void gb_memory_viewer_t::init(gb_t* _gb){

    gb = _gb;
    
    file_selector.set_callback(file_selector_callback,this);

    file_save.set_callback(file_save_callback,this);

    update_current_memory_type();

    memory_viewer_base_t::init();
}


void gb_memory_viewer_t::file_selector_callback(void* userdata,std::filesystem::path path){
    gb_memory_viewer_t* memory_viewer = (gb_memory_viewer_t*)userdata;
    gb_memory_type_import(memory_viewer->gb,memory_viewer->current_memory_type,path.u8string().c_str());
}

void gb_memory_viewer_t::file_save_callback(void* userdata,std::filesystem::path path){
    gb_memory_viewer_t* memory_viewer = (gb_memory_viewer_t*)userdata;
    gb_memory_type_export(memory_viewer->gb,memory_viewer->current_memory_type,path.u8string().c_str());
}


size_t gb_memory_viewer_t::get_current_memory_length(){
    return gb_memory_type_length(gb,current_memory_type);
}

void gb_memory_viewer_t::write_byte(uint8_t value,size_t address){
    gb_memory_type_write_byte(gb,current_memory_type,value,address);
}

uint8_t gb_memory_viewer_t::read_byte(size_t address){
    return gb_memory_type_read_byte(gb,current_memory_type,address);
}

bool gb_memory_viewer_t::cartridge_inserted(){
    return gb->cartridge_inserted;
}


void gb_memory_viewer_t::render_memory_type_combo(){

    if(ImGui::BeginCombo("Memory Type",gb_memory_type_names[current_memory_type])){
        for(int i = 0; i < gb_memory_type_count; ++i){
            if(
                (i == gb_memory_rom_type && !gb->cartridge.rom_length) ||
                (i == gb_memory_ram_type && !gb->cartridge.ram_length)
            ){
                continue;
            }

            if(ImGui::Selectable(gb_memory_type_names[i],current_memory_type == i)){
                current_memory_type = i;
                update_current_memory_type();
            }
        }
        ImGui::EndCombo();
    }

}
