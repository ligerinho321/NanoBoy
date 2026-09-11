#include <gui/rewind_settings/rewind_settings.hpp>

rewind_settings_t::rewind_settings_t(gb_t* gb):gb(gb){
    gb_rewind_set_enabled(gb,true);
    gb_rewind_set_capacity(gb,1800);
    gb_rewind_set_frame_time(gb,1.0f / 60.0f);

    update_window_size_constraints();
}

void rewind_settings_t::update_window_size_constraints(){
    ImGuiStyle& style = ImGui::GetStyle();

    float title_bar_height = ImGui::GetFrameHeight();

    float content_height = ImGui::GetFrameHeightWithSpacing() * 3.0f;

    float window_height = title_bar_height + content_height + style.WindowPadding.y * 2.0f;

    window_min_size.x = style.WindowMinSize.x;
    window_min_size.y = window_height;

    window_max_size.x = FLT_MAX;
    window_max_size.y = window_height;
}


void rewind_settings_t::save(cJSON* object){
    cJSON* rewind_settings_object = cJSON_CreateObject();
    cJSON_AddItemToObjectCS(object,"Rewind settings",rewind_settings_object);

    cJSON* enabled_bool = cJSON_CreateBool(gb_rewind_get_enabled(gb));
    cJSON_AddItemToObjectCS(rewind_settings_object,"Enabled",enabled_bool);

    cJSON* capacity_number = cJSON_CreateNumber(gb_rewind_get_capacity(gb));
    cJSON_AddItemToObjectCS(rewind_settings_object,"Capacity",capacity_number);

    cJSON* frame_time_number = cJSON_CreateNumber(gb_rewind_get_frame_time(gb));
    cJSON_AddItemToObjectCS(rewind_settings_object,"Frame time",frame_time_number);
}

void rewind_settings_t::load(cJSON* object){
    cJSON* rewind_settings_object = cJSON_GetObjectItemCaseSensitive(object,"Rewind settings");

    if(!rewind_settings_object || !cJSON_IsObject(rewind_settings_object)) return;

    cJSON* enabled_bool = cJSON_GetObjectItemCaseSensitive(rewind_settings_object,"Enabled");

    if(enabled_bool && cJSON_IsBool(enabled_bool)){
        gb_rewind_set_enabled(gb,cJSON_IsTrue(enabled_bool));
    }

    cJSON* capacity_number = cJSON_GetObjectItemCaseSensitive(rewind_settings_object,"Capacity");

    if(capacity_number && cJSON_IsNumber(capacity_number)){
        gb_rewind_set_capacity(gb,cJSON_GetNumberValue(capacity_number));
    }

    cJSON* frame_time_number = cJSON_GetObjectItemCaseSensitive(rewind_settings_object,"Frame time");

    if(frame_time_number && cJSON_IsNumber(frame_time_number)){
        gb_rewind_set_frame_time(gb,cJSON_GetNumberValue(frame_time_number));
    }
}


void rewind_settings_t::render(){
    if(!_open) return;

    ImGui::SetNextWindowSizeConstraints(window_min_size,window_max_size);

    if(ImGui::Begin("Rewind",&_open)){

        ImGuiStyle& style = ImGui::GetStyle();

        uint32_t step = 1;
    
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Buffer Frames:");
        ImGui::SameLine(0.0f,style.ItemInnerSpacing.x);
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        ImGui::InputScalar("##BufferFramesInputScalar",ImGuiDataType_U32,&temp_capacity,&step,nullptr,"%" PRIu32);

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Speed (FPS):");
        ImGui::SameLine(0.0f,style.ItemInnerSpacing.x);
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        ImGui::InputScalar("##SpeedInputScalar",ImGuiDataType_U32,&temp_speed,&step,nullptr,"%" PRIu32);

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

    temp_enabled = gb_rewind_get_enabled(gb);
    temp_capacity = gb_rewind_get_capacity(gb);
    temp_speed = ceilf(1.0f / gb_rewind_get_frame_time(gb));
}

void rewind_settings_t::close(bool discard_changes) noexcept {
    if(!_open) return;

    _open = false;

    if(!discard_changes){
        gb_rewind_set_enabled(gb,temp_enabled);
        gb_rewind_set_capacity(gb,temp_capacity);
        gb_rewind_set_frame_time(gb,1.0f / temp_speed);
    }
}