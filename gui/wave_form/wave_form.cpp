#include <gui/wave_form/wave_form.hpp>

const char* audio_formats[] = {
    "WAV\0.wav"
};

const int audio_formats_count = sizeof(audio_formats) / sizeof(audio_formats[0]);


wave_form_t::wave_form_t(gb_t* _gb):gb(_gb){
    file_save.set_callback(file_save_callback,this);
    file_save.set_extensions(audio_formats,audio_formats_count);
}


void wave_form_t::channel_callback(void* userdata){
    wave_form_t* wave_form = (wave_form_t*)userdata;
    gb_apu_t* apu = &wave_form->gb->apu;

    memcpy(wave_form->square1.samples,apu->square1_frame.samples,sizeof(wave_form->square1.samples));
    wave_form->square1.count = apu->square1_frame.samples_count;

    memcpy(wave_form->square2.samples,apu->square2_frame.samples,sizeof(wave_form->square2.samples));
    wave_form->square2.count = apu->square2_frame.samples_count;

    memcpy(wave_form->wave.samples,apu->wave_frame.samples,sizeof(wave_form->wave.samples));
    wave_form->wave.count = apu->wave_frame.samples_count;

    memcpy(wave_form->noise.samples,apu->noise_frame.samples,sizeof(wave_form->noise.samples));
    wave_form->noise.count = apu->noise_frame.samples_count;
}

void wave_form_t::recording_callback(void* userdata){
    wave_form_t* wave_form = (wave_form_t*)userdata;
    gb_apu_t* apu = &wave_form->gb->apu;

    size_t size = wave_form->output.size();
    size_t capacity = wave_form->output.capacity();
    size_t len = apu->mixer_frame.samples_count * gb_audio_bytes_per_sample;

    if(size + len > capacity){
        wave_form->output.reserve(capacity + gb_max(len,capacity));
    }

    wave_form->output.resize(size + len,0);

    memcpy(wave_form->output.data() + size,apu->mixer_frame.samples,len);

    size_t _output_size = wave_form->output.size();
    int _seconds = _output_size / gb_audio_byte_rate;
    int _minutes = _seconds / 60;
    int _hours = _seconds / 3600;

    wave_form->output_size.store(_output_size,std::memory_order_relaxed);
    wave_form->seconds.store(_seconds % 60,std::memory_order_relaxed);
    wave_form->minutes.store(_minutes % 60,std::memory_order_relaxed);
    wave_form->hours.store(_hours,std::memory_order_relaxed);
}

void wave_form_t::file_save_callback(void* userdata,std::filesystem::path path){
    
    wave_form_t* wave_form = (wave_form_t*)userdata;

    FILE* file = fopen(path.u8string().c_str(),"wb");
    if(!file){
        gb_printf_error(fopen);
        return;
    }

    uint32_t chunck_size = SDL_SwapLE32(16);
    uint16_t audio_format = SDL_SwapLE16(1);
    uint16_t channels = SDL_SwapLE16(gb_audio_channels);
    uint32_t sample_rate = SDL_SwapLE32(gb_audio_sample_rate);
    uint32_t byte_rate = SDL_SwapLE32(gb_audio_byte_rate);
    uint16_t block_align = SDL_SwapLE16((gb_audio_bits_per_sample * gb_audio_channels) / 8);
    uint16_t bits_per_sample = SDL_SwapLE16(gb_audio_bits_per_sample);
    uint32_t data_size = SDL_SwapLE32(wave_form->output_size);
    uint32_t file_size = SDL_SwapLE32(36 + wave_form->output_size);

    //RIFF header
    // ID (4bytes): "RIFF"
    fwrite("RIFF",1,4,file);
    // file_size (4bytes): size - 8bytes
    fwrite(&file_size,1,4,file);
    // type_header (4bytes): "WAVE"
    fwrite("WAVE",1,4,file);

    // fmt Header
    // ID (4bytes): "fmt\0"
    fwrite("fmt ",1,4,file);
    // chunck_size (4bytes): 16
    fwrite(&chunck_size,1,4,file);
    // audio_format (2bytes): 1 (PCM)
    fwrite(&audio_format,1,2,file);
    // num_channels (2bytes): *
    fwrite(&channels,1,2,file);
    // sample_rate (4bytes): *
    fwrite(&sample_rate,1,4,file);
    // byte_rate (4bytes): (sample_rate * bits_per_sample * num_channels) / 8
    fwrite(&byte_rate,1,4,file);
    // block_align (2bytes): (bits_per_sample * num_channels) / 8
    fwrite(&block_align,1,2,file);
    // bits_per_sample (2bytes): *
    fwrite(&bits_per_sample,1,2,file);

    // data Header
    // ID (4bytes): "data"
    fwrite("data",1,4,file);
    // Size (4bytes): *
    fwrite(&data_size,1,4,file);

    // data bytes...
    fwrite(wave_form->output.data(),1,data_size,file);

    fclose(file);

    wave_form->clear_recording();
}

float wave_form_t::get_sample_callback(void* data,int idx){
    channel_frame_t* cf = (channel_frame_t*)data;
    return (float)cf->samples[idx]; 
}


void wave_form_t::clear_recording() noexcept {
    
    recording = false;
    paused = false;

    std::vector<uint8_t>().swap(output);

    output_size.store(0,std::memory_order_relaxed);
    
    seconds.store(0,std::memory_order_relaxed);
    minutes.store(0,std::memory_order_relaxed);
    hours.store(0,std::memory_order_relaxed);
}

void wave_form_t::pause_recording(bool _paused) noexcept {
    if(!recording || paused == _paused) return;

    paused = _paused;

    if(paused){
        gb_thread_safe_remove_apu_handler(gb,&recording_handler);

        last_time = time(NULL);
    }
    else{
        gb_thread_safe_add_apu_handler(gb,&recording_handler);
    }
}


const char* wave_form_t::get_recording_file_name() const noexcept {
    static char buffer[128] = {0};
    struct tm* local_time = localtime(&last_time);
    strftime(buffer,sizeof(buffer),"NanoBoy_Recording_%d%m%Y_%H%M%S",local_time);
    return buffer;
}


void wave_form_t::render_popup_modal(){
    static const char* name = "Save Recording?";

    if(request_open_popup_modal){
        request_open_popup_modal = false;

        popup_modal_open = true;
        ImGui::OpenPopup(name);

        ImVec2 size = ImGui::GetMainViewport()->Size;

        popup_modal_start_pos.x = size.x * 0.5f;
        popup_modal_start_pos.y = size.y * 0.5f;
    }

    if(!popup_modal_open) return;

    ImGui::SetNextWindowPos(popup_modal_start_pos,ImGuiCond_Appearing,ImVec2(0.5f,0.5f));

    if(!ImGui::BeginPopupModal(name,&popup_modal_open,ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize)) return;

    ImGuiStyle& style = ImGui::GetStyle();

    ImGui::TextUnformatted("Do you want to save the recording before exiting?");

    const char* save = "Save";
    const char* discard = "Discard";
    const char* cancel = "Cancel";

    float save_button_width = ImGui::CalcTextSize(save).x + style.FramePadding.x * 2.0f;
    float discard_button_width = ImGui::CalcTextSize(discard).x + style.FramePadding.x * 2.0f;
    float cancel_button_width = ImGui::CalcTextSize(cancel).x + style.FramePadding.x * 2.0f;

    float cursor_pos_x = ImGui::GetCursorPosX();
    float content_region_avail_x = ImGui::GetContentRegionAvail().x;

    ImGui::SetCursorPosX(
        cursor_pos_x + content_region_avail_x -
        save_button_width - discard_button_width - cancel_button_width - 
        style.ItemSpacing.x * 2.0f
    );

    if(ImGui::Button(save)){
        popup_modal_open = false;

        file_save.copy_to_name_buffer(get_recording_file_name());

        file_save.set_open(true);
    }

    ImGui::SameLine();

    if(ImGui::Button(discard)){
        popup_modal_open = false;
        set_open(false,true);
    }

    ImGui::SameLine();

    if(ImGui::Button(cancel)){
        popup_modal_open = false;
    }

    ImGui::EndPopup();
}

void wave_form_t::render(){
    if(!open) return;

    if(!gb->cartridge_inserted){
        set_open(false,true);
    }

    bool _open = open;

    if(ImGui::Begin("Wave Form",&_open)){

        ImGuiStyle& style = ImGui::GetStyle();

        ImVec2 table_size(
            0.0f,
            ImGui::GetContentRegionAvail().y - ImGui::GetFrameHeightWithSpacing()
        );

        float row_height = table_size.y / 4.0f;
        ImVec2 graph_size = ImVec2(-FLT_MIN,row_height - style.CellPadding.y * 2.0f);

        ImGui::BeginDisabled(!recording);

        if(ImGui::Button(paused ? "Resume" : "Pause")){
            pause_recording(!paused);
        }

        ImGui::EndDisabled();

        ImGui::SameLine();

        if(ImGui::Button(recording ? "Stop" : "Start")){

            if(recording){
                pause_recording(true);

                file_save.copy_to_name_buffer(get_recording_file_name());

                file_save.set_open(true);
            }
            else{
                recording = true;
                
                paused = false;

                gb_thread_safe_add_apu_handler(gb,&recording_handler);
            }
        }

        ImGui::SameLine();

        ImGui::BeginDisabled((recording && !paused) || !output_size.load(std::memory_order_relaxed));

        if(ImGui::Button("Discard")){
            clear_recording();
        }

        ImGui::EndDisabled();

        ImGui::SameLine(0.0f,0.0f);
        ImGui::TextUnformatted(" | ");
        ImGui::SameLine(0.0f,0.0f);

        ImGui::AlignTextToFramePadding();
        
        ImGui::Text(
            "%02d:%02d:%02d",
            hours.load(std::memory_order_relaxed),
            minutes.load(std::memory_order_relaxed),
            seconds.load(std::memory_order_relaxed)
        );

        ImGui::SameLine(0.0f,0.0f);
        ImGui::TextUnformatted(" | ");
        ImGui::SameLine(0.0f,0.0f);

        render_hertz_text(gb_audio_sample_rate);

        ImGui::SameLine(0.0f,0.0f);
        ImGui::TextUnformatted(" | ");
        ImGui::SameLine(0.0f,0.0f);
        
        ImGui::TextUnformatted("Stereo");

        ImGui::SameLine(0.0f,0.0f);
        ImGui::TextUnformatted(" | ");
        ImGui::SameLine(0.0f,0.0f);

        render_size_text(output_size.load(std::memory_order_relaxed));


        if(ImGui::BeginTable("WavesTable",3,ImGuiTableFlags_None,table_size)){

            ImGui::TableSetupColumn("Channel",ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableSetupColumn("Mute",ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableSetupColumn("Wave Form",ImGuiTableColumnFlags_WidthStretch);

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextUnformatted("Square1");
            ImGui::TableNextColumn();
            ImGui::Checkbox("##CheckboxSquare1",&gb->apu.square1.external_enabled);
            ImGui::TableNextColumn();
            ImGui::PlotLines(
                "##GraphSquare1",
                get_sample_callback,
                &square1,
                square1.count,
                0,
                nullptr,
                gb_audio_channel_min_output,
                gb_audio_channel_max_output,
                graph_size
            );

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextUnformatted("Square2");
            ImGui::TableNextColumn();
            ImGui::Checkbox("##CheckboxSquare2",&gb->apu.square2.external_enabled);
            ImGui::TableNextColumn();
            ImGui::PlotLines(
                "##GraphSquare2",
                get_sample_callback,
                &square2,
                square2.count,
                0,
                nullptr,
                gb_audio_channel_min_output,
                gb_audio_channel_max_output,
                graph_size
            );

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextUnformatted("Wave");
            ImGui::TableNextColumn();
            ImGui::Checkbox("##CheckboxWave",&gb->apu.wave.external_enabled);
            ImGui::TableNextColumn();
            ImGui::PlotLines(
                "##GraphWave",
                get_sample_callback,
                &wave,
                wave.count,
                0,
                nullptr,
                gb_audio_channel_min_output,
                gb_audio_channel_max_output,
                graph_size
            );

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextUnformatted("Noise");
            ImGui::TableNextColumn();
            ImGui::Checkbox("##CheckboxNoise",&gb->apu.noise.external_enabled);
            ImGui::TableNextColumn();
            ImGui::PlotLines(
                "##GraphNoise",
                get_sample_callback,
                &noise,
                noise.count,
                0,
                nullptr,
                gb_audio_channel_min_output,
                gb_audio_channel_max_output,
                graph_size
            );
            
            ImGui::EndTable();
        }

        render_popup_modal();

        file_save.render();
    }

    ImGui::End();

    if(!_open){
        set_open(false,false);
    }
}


void wave_form_t::clear(){
    gb_remove_apu_handler(gb,&channel_handler);

    square1.count = 0;
    square2.count = 0;
    wave.count = 0;
    noise.count = 0;

    gb_remove_apu_handler(gb,&recording_handler);
    
    clear_recording();
}


void wave_form_t::set_open(bool _open,bool force_discarding) noexcept {

    if(open == _open) return;
    
    if(_open){
        open = true;

        gb_thread_safe_add_apu_handler(gb,&channel_handler);
    }
    else{
        pause_recording(true);

        if(recording && !force_discarding){
            request_open_popup_modal = true;
        }
        else{
            open = false;

            gb_thread_safe_remove_apu_handler(gb,&channel_handler);

            clear_recording();
        }
    }
}