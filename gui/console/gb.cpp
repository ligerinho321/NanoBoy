#include <gui/console/gb.hpp>
#include <gui/nanoboy/nanoboy.hpp>

gb_console_t::gb_console_t(nanoboy_t* _nanoboy):console_t(_nanoboy){
    gb = gb_new();
    gb_joypad_set_callback(gb,joypad_callback,nanoboy);

    debugger.init(gb);
    register_viewer.init(gb);
    event_viewer.init(gb,nanoboy->renderer);
    memory_viewer.init(gb);
    tilemap_viewer.init(gb,nanoboy->renderer);
    tile_viewer.init(gb,nanoboy->renderer);
    object_viewer.init(gb,nanoboy->renderer);
    palette_viewer.init(gb,nanoboy->renderer);
    wave_form.init(gb);

    cheats.init(gb);
    printer.init(gb,nanoboy->renderer);
}

gb_console_t::~gb_console_t(){
    debugger.uninit();
    event_viewer.uninit();
    tilemap_viewer.uninit();
    tile_viewer.uninit();
    object_viewer.uninit();
    palette_viewer.uninit();
    wave_form.uninit();

    cheats.uninit();
    printer.uninit();

    gb_delete(gb);
}


void gb_console_t::joypad_callback(void* data,gb_joypad_button_state_t* button_state){
    nanoboy_t* nanoboy = (nanoboy_t*)data;

    input_settings_t& input_settings = nanoboy->input_settings;

    button_state->down = input_settings.pressed(gb_button_down);
    button_state->up = input_settings.pressed(gb_button_up);
    button_state->left = input_settings.pressed(gb_button_left);
    button_state->right = input_settings.pressed(gb_button_right);
    button_state->start = input_settings.pressed(gb_button_start);
    button_state->select = input_settings.pressed(gb_button_select);
    button_state->a = input_settings.pressed(gb_button_a);
    button_state->b = input_settings.pressed(gb_button_b);
}


void gb_console_t::update_screen_texture(){
    uint8_t* pixels = nullptr;
    int pitch = 0;
    SDL_LockTexture(screen_texture,nullptr,(void**)&pixels,&pitch);

    memcpy(pixels,gb_ppu_get_screen_buffer(gb),gb_screen_length);

    SDL_UnlockTexture(screen_texture);
}


bool gb_console_t::take_screenshot(const char* path){
    return stbi_write_png(path,gb_screen_width,gb_screen_height,gb_screen_bytes_per_pixel,gb_ppu_get_screen_buffer(gb),gb_screen_pitch);
}


bool gb_console_t::insert_cartridge(const char* path){
    return gb_insert_cartridge(gb,path);
}

bool gb_console_t::insert_cartridge_from_memory(uint8_t* rom,size_t length){
    return gb_insert_cartridge_from_memory(gb,rom,length);
}

void gb_console_t::remove_cartridge(){

    event_viewer.clear();
    memory_viewer.clear();
    tilemap_viewer.clear();
    tile_viewer.clear();
    object_viewer.clear();
    palette_viewer.clear();
    wave_form.clear();

    cheats.clear();
    printer.clear();

    clear_texture(screen_texture,gb_screen_height);

    gb_remove_cartridge(gb);
}


void gb_console_t::increase_speed(){
    gb_set_speed(gb,gb->speed + gb_speed_step);
}

void gb_console_t::decrease_speed(){
    gb_set_speed(gb,gb->speed - gb_speed_step);
}

void gb_console_t::set_speed(float speed){
    gb_set_speed(gb,speed);
}

float gb_console_t::get_speed(){
    return gb->speed;
}

void gb_console_t::pause(){
    gb_pause(gb,!gb->paused);
}

bool gb_console_t::paused(){
    return gb->paused;
}

void gb_console_t::reset(){
    gb_reset(gb);

    event_viewer.reset();
}


void gb_console_t::save_ram(const char* path){
    gb_cartridge_save_ram(gb,path);
}

void gb_console_t::load_ram(const char* path){
    gb_cartridge_load_ram(gb,path);
}


void gb_console_t::save_rtc(const char* path){
    gb_cartridge_save_rtc(gb,path);
}

void gb_console_t::load_rtc(const char* path){
    gb_cartridge_load_rtc(gb,path);
}


void gb_console_t::save_cheats(const char* path){
    cheats.save(path);
}

void gb_console_t::load_cheats(const char* path){
    cheats.load(path);
}


bool gb_console_t::savestate_serialize(const char* path){
    return gb_savestate_serialize(gb,path);
}

bool gb_console_t::savestate_deserialize(const char* path){
    return gb_savestate_deserialize(gb,path);
}

bool gb_console_t::savestate_get_info(const char* path,console_savestate_info_t* info){

    gb_savestate_info_t result = {};

    if(gb_savestate_get_info(path,&result)){

        info->timestamp = result.timestamp;
        info->screenshot = result.screenshot;

        return true;
    }

    return false;
}


void gb_console_t::set_rewind_enabled(bool enabled){
    gb_rewind_set_enabled(gb,enabled);
}

void gb_console_t::set_rewind_capacity(uint32_t capacity){
    gb_rewind_set_capacity(gb,capacity);
}

void gb_console_t::set_rewind_recording_interval(uint32_t interval){
    gb_rewind_set_recording_interval(gb,interval);
}

void gb_console_t::set_rewind_frame_time(float time){
    gb_rewind_set_frame_time(gb,time);
}

bool gb_console_t::start_rewind(){
    return gb_rewind_start(gb);
}

bool gb_console_t::end_rewind(){
    return gb_rewind_end(gb);
}


void gb_console_t::execute_frame(){
    gb_execute_frame(gb);
}

void gb_console_t::render_imgui_windows(){
    debugger.render();
    register_viewer.render();
    event_viewer.render();
    memory_viewer.render();
    tilemap_viewer.render();
    tile_viewer.render();
    object_viewer.render();
    palette_viewer.render();
    wave_form.render();

    cheats.render();
    printer.render();
}