#include <gui/rewind_settings/rewind_settings.hpp>
#include <gui/nanoboy/nanoboy.hpp>

void rewind_settings_t::init(nanoboy_t* _nanoboy){
    nanoboy = _nanoboy;

    load_default();

    update_window_size_constraints();
}


void rewind_settings_t::apply_to_console(){
    if(!nanoboy->console) return;

    nanoboy->console->set_rewind_enabled(enabled);
    nanoboy->console->set_rewind_capacity(buffer_frames);
    nanoboy->console->set_rewind_recording_interval(recording_interval);
    nanoboy->console->set_rewind_frame_time(1.0f / (float)speed);
}


void rewind_settings_t::update_window_size_constraints(){
    ImGuiStyle& style = ImGui::GetStyle();

    float title_bar_height = ImGui::GetFrameHeight();

    float table_height = (ImGui::GetFrameHeight() + style.CellPadding.y * 2.0f) * 3.0f;

    float button_height = ImGui::GetFrameHeightWithSpacing();

    float window_height = title_bar_height + table_height + button_height + style.WindowPadding.y * 2.0f;

    window_min_size.x = style.WindowMinSize.x;
    window_min_size.y = window_height;

    window_max_size.x = FLT_MAX;
    window_max_size.y = window_height;
}


void rewind_settings_t::load_default(){
    enabled = true;
    buffer_frames = 1800;
    recording_interval = 1;
    speed = 60;
}

void rewind_settings_t::save(cJSON* object){
    cJSON* rewind_settings_object = cJSON_CreateObject();
    cJSON_AddItemToObjectCS(object,"Rewind Settings",rewind_settings_object);

    cJSON* enabled_bool = cJSON_CreateBool(enabled);
    cJSON_AddItemToObjectCS(rewind_settings_object,"Enabled",enabled_bool);

    cJSON* buffer_frames_number = cJSON_CreateNumber(buffer_frames);
    cJSON_AddItemToObjectCS(rewind_settings_object,"Buffer Frames",buffer_frames_number);

    cJSON* recording_interval_number = cJSON_CreateNumber(recording_interval);
    cJSON_AddItemToObjectCS(rewind_settings_object,"Recording Interval",recording_interval_number);

    cJSON* speed_number = cJSON_CreateNumber(speed);
    cJSON_AddItemToObjectCS(rewind_settings_object,"Speed",speed_number);
}

void rewind_settings_t::load(cJSON* object){
    cJSON* rewind_settings_object = cJSON_GetObjectItemCaseSensitive(object,"Rewind Settings");

    if(!cJSON_IsObject(rewind_settings_object)) return;

    cJSON* enabled_bool = cJSON_GetObjectItemCaseSensitive(rewind_settings_object,"Enabled");

    if(cJSON_IsBool(enabled_bool)){
        enabled = cJSON_IsTrue(enabled_bool);
    }

    cJSON* buffer_frames_number = cJSON_GetObjectItemCaseSensitive(rewind_settings_object,"Buffer Frames");

    if(cJSON_IsNumber(buffer_frames_number)){
        buffer_frames = cJSON_GetNumberValue(buffer_frames_number);
    }

    cJSON* recording_interval_number = cJSON_GetObjectItemCaseSensitive(rewind_settings_object,"Recording Interval");

    if(cJSON_IsNumber(recording_interval_number)){
        recording_interval = cJSON_GetNumberValue(recording_interval_number);
    }

    cJSON* speed_number = cJSON_GetObjectItemCaseSensitive(rewind_settings_object,"Speed");

    if(cJSON_IsNumber(speed_number)){
        speed = cJSON_GetNumberValue(speed_number);
    }
}


void rewind_settings_t::render(){
    if(!_open) return;

    ImGui::SetNextWindowSizeConstraints(window_min_size,window_max_size);

    if(ImGui::Begin("Rewind",&_open)){

        ImGuiStyle& style = ImGui::GetStyle();

        if(ImGui::BeginTable("RewindTable",2)){
            
            ImGui::TableSetupColumn(nullptr,ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableSetupColumn(nullptr,ImGuiTableColumnFlags_WidthStretch);

            uint32_t step = 1;
            
            ImGui::TableNextRow();

            ImGui::TableNextColumn();
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Buffer Frames");

            ImGui::TableNextColumn();
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            ImGui::InputScalar("##BufferFramesInputScalar",ImGuiDataType_U32,&temp_buffer_frames,&step,nullptr,"%" PRIu32);

            ImGui::TableNextRow();

            ImGui::TableNextColumn();
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Recording Interval");

            ImGui::TableNextColumn();
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            ImGui::InputScalar("RecordingIntervalInputScalar",ImGuiDataType_U32,&temp_recording_interval,&step,nullptr,"%" PRIu32);

            ImGui::TableNextRow();

            ImGui::TableNextColumn();
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Speed");

            ImGui::TableNextColumn();
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            ImGui::InputScalar("##SpeedInputScalar",ImGuiDataType_U32,&temp_speed,&step,nullptr,"%" PRIu32);
            
            ImGui::EndTable();
        }
        
    
        ImGui::Checkbox("Enabled",&temp_enabled);

        ImGui::SameLine();

        const char* str_ok = "Ok";
        const char* str_cancel = "Cancel";

        float button_padding = style.FramePadding.x * 2.0f;
        float ok_button_width = ImGui::CalcTextSize(str_ok).x + button_padding;
        float cancel_button_width = ImGui::CalcTextSize(str_cancel).x + button_padding;

        ImGui::SetCursorPosX((ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x) - (ok_button_width + cancel_button_width + style.ItemSpacing.x));

        if(ImGui::Button(str_ok)) close(false);

        ImGui::SameLine();

        if(ImGui::Button(str_cancel)) close(true);

    }
    ImGui::End();
}


void rewind_settings_t::open() noexcept {
    if(_open) return;

    _open = true;

    temp_enabled = enabled;
    temp_buffer_frames = buffer_frames;
    temp_recording_interval = recording_interval;
    temp_speed = speed;
}

void rewind_settings_t::close(bool discard_changes) noexcept {
    if(!_open) return;

    _open = false;

    if(!discard_changes){
        enabled = temp_enabled;
        buffer_frames = temp_buffer_frames;
        recording_interval = temp_recording_interval;
        speed = temp_speed;

        apply_to_console();
    }
}