#pragma once

#include <gui/utils/utils.hpp>
#include <gui/file_dialog/file_save_dialog.hpp>

class wave_form_t {
private:
    struct channel_frame_t {
        int16_t samples[gb_audio_frame_samples];
        int count;
    };

    gb_t *gb;

    file_save_dialog_t file_save;

    channel_frame_t square1 = {};
    channel_frame_t square2 = {};
    channel_frame_t wave = {};
    channel_frame_t noise = {};

    std::vector<uint8_t> output;
    int seconds;
    int minutes;
    int hours;
    time_t last_time;
    bool paused = false;
    bool recording = false;

    gb_apu_handler_t channel_handler{channel_callback,this,nullptr};
    gb_apu_handler_t recording_handler{recording_callback,this,nullptr};

    bool request_open_popup_modal = false;
    bool popup_modal_open = false;
    ImVec2 popup_modal_start_pos;

    bool open = false;

    static void channel_callback(void* userdata);
    static void recording_callback(void* userdata);
    static void file_save_callback(void* userdata,std::filesystem::path);
    static float get_sample_callback(void* data,int idx);

    void clear_recording() noexcept;
    void pause_recording(bool _paused) noexcept;

    const char* get_recording_file_name() const noexcept;

    void render_popup_modal();
public:

    wave_form_t(gb_t* _gb);

    ~wave_form_t();

    void render();

    void clear();
    
    void set_open(bool _open,bool force_discarding) noexcept;

    bool get_open() const noexcept {
        return open;
    }
};
