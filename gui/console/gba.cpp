#include <gui/console/gba.hpp>
#include <gui/nanoboy/nanoboy.hpp>

gba_console_t::gba_console_t(nanoboy_t* _nanoboy):console_t(_nanoboy){
    gba = gba_new();
    gba_keypad_set_callback(gba,keypad_callback,nanoboy);
}

gba_console_t::~gba_console_t(){
    gba_delete(gba);
}


void gba_console_t::keypad_callback(void* data,uint8_t* status){
    nanoboy_t* nanoboy = (nanoboy_t*)data;

    input_settings_t& input_settings = nanoboy->input_settings;

    for(int i = 0; i < gba_button_count; ++i){
        status[i] = input_settings.pressed(i);
    }
}


void gba_console_t::update_screen_texture(){

}


bool gba_console_t::take_screenshot(const char* path){
    gba_unused(path);
    return false;
}


bool gba_console_t::insert_cartridge(const char* path){
    return gba_insert_cartridge(gba,path);
}

bool gba_console_t::insert_cartridge_from_memory(uint8_t* rom,size_t length){
    return gba_insert_cartridge_from_memory(gba,rom,length);
}

void gba_console_t::remove_cartridge(){
    gba_remove_cartridge(gba);
}


void gba_console_t::increase_speed(){

}

void gba_console_t::decrease_speed(){

}

void gba_console_t::set_speed(float speed){
    gba_unused(speed);
}

float gba_console_t::get_speed(){
    return 0.0f;
}

void gba_console_t::pause(){
    gba_pause(gba,!gba->paused);
}

bool gba_console_t::paused(){
    return gba->paused;
}

void gba_console_t::reset(){
    gba_reset(gba);
}


void gba_console_t::save_ram(const char* path){
    gba_unused(path);
}

void gba_console_t::load_ram(const char* path){
    gba_unused(path);
}


void gba_console_t::save_rtc(const char* path){
    gba_unused(path);
}

void gba_console_t::load_rtc(const char* path){
    gba_unused(path);
}


void gba_console_t::save_cheats(const char* path){
    gba_unused(path);
}

void gba_console_t::load_cheats(const char* path){
    gba_unused(path);
}


bool gba_console_t::savestate_serialize(const char* path){
    gba_unused(path);
    return false;
}

bool gba_console_t::savestate_deserialize(const char* path){
    gba_unused(path);
    return false;
}

bool gba_console_t::savestate_get_info(const char* path,console_savestate_info_t* info){
    gba_unused(path);
    gba_unused(info);
    return false;
}


void gba_console_t::set_rewind_enabled(bool enabled){
    gba_unused(enabled);
}

void gba_console_t::set_rewind_capacity(uint32_t capacity){
    gba_unused(capacity);
}

void gba_console_t::set_rewind_recording_interval(uint32_t interval){
    gba_unused(interval);
}

void gba_console_t::set_rewind_frame_time(float time){
    gba_unused(time);
}

bool gba_console_t::start_rewind(){
    return false;
}

bool gba_console_t::end_rewind(){
    return false;
}


void gba_console_t::execute_frame(){
    gba_execute_frame(gba);
}

void gba_console_t::render_imgui_windows(){

}