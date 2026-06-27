#include "wave_form.hpp"

void wave_form_t::frame_callback(void* data){
    wave_form_t* wf = (wave_form_t*)data;
    gb_apu_t* apu = &wf->gb->apu;

    memcpy(wf->square1.samples,apu->square1_frame.samples,sizeof(wf->square1.samples));
    wf->square1.count = apu->square1_frame.samples_count;

    memcpy(wf->square2.samples,apu->square2_frame.samples,sizeof(wf->square2.samples));
    wf->square2.count = apu->square2_frame.samples_count;

    memcpy(wf->wave.samples,apu->wave_frame.samples,sizeof(wf->wave.samples));
    wf->wave.count = apu->wave_frame.samples_count;

    memcpy(wf->noise.samples,apu->noise_frame.samples,sizeof(wf->noise.samples));
    wf->noise.count = apu->noise_frame.samples_count;
}

float wave_form_t::get_sample(void* data,int idx){
    channel_frame_t* cf = (channel_frame_t*)data;
    return (float)cf->samples[idx]; 
}

void wave_form_t::render(){
    if(!open) return;

    bool _open = open;

    if(ImGui::Begin("Wave Form",&_open)){

        ImGuiStyle& style = ImGui::GetStyle();
        ImVec2 table_size = ImGui::GetContentRegionAvail();
        float row_height = table_size.y / 2.0f;
        ImVec2 graph_size = ImVec2(-FLT_MIN,row_height - ImGui::GetFrameHeight() - style.CellPadding.y * 2.0f - style.ItemSpacing.y);

        if(ImGui::BeginTable("WavesTable",2,ImGuiTableFlags_None,table_size)){

            ImGui::TableNextRow();

            ImGui::TableNextColumn();
            ImGui::Checkbox("Square1",&gb->apu.square1.external_enabled);
            ImGui::PlotLines(
                "##GraphSquare1",
                get_sample,
                &square1,
                square1.count,
                0,
                nullptr,
                gb_audio_channel_min_output,
                gb_audio_channel_max_output,
                graph_size
            );

            ImGui::TableNextColumn();
            ImGui::Checkbox("Square2",&gb->apu.square2.external_enabled);
            ImGui::PlotLines(
                "##GraphSquare2",
                get_sample,
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
            ImGui::Checkbox("Wave",&gb->apu.wave.external_enabled);
            ImGui::PlotLines(
                "##GraphWave",
                get_sample,
                &wave,
                wave.count,
                0,
                nullptr,
                gb_audio_channel_min_output,
                gb_audio_channel_max_output,
                graph_size
            );

            ImGui::TableNextColumn();
            ImGui::Checkbox("Noise",&gb->apu.noise.external_enabled);
            ImGui::PlotLines(
                "##GraphNoise",
                get_sample,
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

    }
    ImGui::End();

    set_open(_open);
}
