#pragma once

#include "utils.hpp"

class wave_form_t {
private:
    struct channel_frame_t {
        int16_t samples[gb_audio_frame_samples];
        int count;
    };

    gb_t *gb = nullptr;

    channel_frame_t square1 = {0};
    channel_frame_t square2 = {0};
    channel_frame_t wave = {0};
    channel_frame_t noise = {0};

    bool open = false;

    static void frame_callback(void* data);

    static float get_sample(void* data,int idx);

public:

    wave_form_t(gb_t* gb):gb(gb){}

    ~wave_form_t(){
        gb_remove_apu_callback(gb);
    }

    void render();

    void clear(){
        square1.count = 0;
        square2.count = 0;
        wave.count = 0;
        noise.count = 0;
    }

    void set_open(bool _open){
        if(open == _open) return;
        open = _open;
        if(open){
            gb_set_apu_callback(gb,frame_callback,this);
        }
        else{
            gb_remove_apu_callback(gb);
        }
    }

    bool get_open() const {
        return open;
    }
};
