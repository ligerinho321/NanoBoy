#include <gui/dmg_palette/dmg_palette.hpp>

struct palette_preset_t {
    const char* name;
    gb_rgb_t bgp[gb_dmg_colors];
    gb_rgb_t obp0[gb_dmg_colors];
    gb_rgb_t obp1[gb_dmg_colors];
};

static const palette_preset_t palette_presets[] = {
    {
        "Gray",
        {{0xE8,0xE8,0xE8},{0xA0,0xA0,0xA0},{0x58,0x58,0x58},{0x10,0x10,0x10}},
        {{0xE8,0xE8,0xE8},{0xA0,0xA0,0xA0},{0x58,0x58,0x58},{0x10,0x10,0x10}},
        {{0xE8,0xE8,0xE8},{0xA0,0xA0,0xA0},{0x58,0x58,0x58},{0x10,0x10,0x10}}
    },
    {
        "Green",
        {{0xE0,0xF8,0xD0},{0x88,0xC0,0x70},{0x34,0x68,0x56},{0x08,0x18,0x20}},
        {{0xE0,0xF8,0xD0},{0x88,0xC0,0x70},{0x34,0x68,0x56},{0x08,0x18,0x20}},
        {{0xE0,0xF8,0xD0},{0x88,0xC0,0x70},{0x34,0x68,0x56},{0x08,0x18,0x20}}
    },
    {
        "GBC Up",
        {{0xFF,0xFF,0xFF},{0xFF,0xAD,0x03},{0x84,0x31,0x00},{0x00,0x00,0x00}},
        {{0xFF,0xFF,0xFF},{0xFF,0xAD,0x03},{0x84,0x31,0x00},{0x00,0x00,0x00}},
        {{0xFF,0xFF,0xFF},{0xFF,0xAD,0x03},{0x84,0x31,0x00},{0x00,0x00,0x00}}
    },
    {
        "GBC A + Up",
        {{0xFF,0xFF,0xFF},{0xFF,0xB4,0x84},{0x94,0x3A,0x3A},{0x00,0x00,0x00}},
        {{0xFF,0xFF,0xFF},{0x7B,0xFF,0x31},{0x00,0xB4,0x00},{0x00,0x00,0x00}},
        {{0xFF,0xFF,0xFF},{0x63,0xA5,0xFF},{0x00,0x00,0xFF},{0x00,0x00,0x00}}
    },
    {
        "GBC B + Up",
        {{0xFF,0xEB,0xC5},{0xCE,0x9C,0x84},{0x84,0x88,0x29},{0x84,0x31,0x08}},
        {{0xFF,0xFF,0xFF},{0xFF,0xAD,0x03},{0x84,0x31,0x00},{0x00,0x00,0x00}},
        {{0xFF,0xFF,0xFF},{0xFF,0xAD,0x03},{0x84,0x31,0x00},{0x00,0x00,0x00}}
    },
    {
        "GBC Left",
        {{0xFF,0xFF,0xFF},{0x63,0xA5,0xFF},{0x00,0x00,0xFF},{0x00,0x00,0x00}},
        {{0xFF,0xFF,0xFF},{0xFF,0xB4,0x84},{0x94,0x3A,0x3A},{0x00,0x00,0x00}},
        {{0xFF,0xFF,0xFF},{0x7B,0xFF,0x31},{0x00,0xB4,0x00},{0x00,0x00,0x00}}
    },
    {
        "GBC A + Left",
        {{0xFF,0xFF,0xFF},{0x3C,0x8C,0xDE},{0x52,0x52,0x8C},{0x00,0x00,0x00}},
        {{0xFF,0xFF,0xFF},{0xFF,0xB4,0x84},{0x94,0x3A,0x3A},{0x00,0x00,0x00}},
        {{0xFF,0xFF,0xFF},{0xFF,0xAD,0x03},{0x84,0x31,0x00},{0x00,0x00,0x00}}
    },
    {
        "GBC B + Left",
        {{0xFF,0xFF,0xFF},{0xA5,0xA5,0xA5},{0x52,0x52,0x52},{0x00,0x00,0x00}},
        {{0xFF,0xFF,0xFF},{0xA5,0xA5,0xA5},{0x52,0x52,0x52},{0x00,0x00,0x00}},
        {{0xFF,0xFF,0xFF},{0xA5,0xA5,0xA5},{0x52,0x52,0x52},{0x00,0x00,0x00}}
    },
    {
        "GBC Down",
        {{0xFF,0xFF,0xA5},{0xFF,0x94,0x94},{0x94,0x94,0xFF},{0x00,0x00,0x00}},
        {{0xFF,0xFF,0xA5},{0xFF,0x94,0x94},{0x94,0x94,0xFF},{0x00,0x00,0x00}},
        {{0xFF,0xFF,0xA5},{0xFF,0x94,0x94},{0x94,0x94,0xFF},{0x00,0x00,0x00}}
    },
    {
        "GBC A + Down",
        {{0xFF,0xFF,0xFF},{0xFF,0xFF,0x00},{0xFF,0x00,0x00},{0x00,0x00,0x00}},
        {{0xFF,0xFF,0xFF},{0xFF,0xFF,0x00},{0xFF,0x00,0x00},{0x00,0x00,0x00}},
        {{0xFF,0xFF,0xFF},{0xFF,0xFF,0x00},{0xFF,0x00,0x00},{0x00,0x00,0x00}}
    },
    {
        "GBC B + Down",
        {{0xFF,0xFF,0xFF},{0xFF,0xFF,0x00},{0x7B,0x4A,0x00},{0x00,0x00,0x00}},
        {{0xFF,0xFF,0xFF},{0x63,0xA5,0xFF},{0x00,0x00,0xFF},{0x00,0x00,0x00}},
        {{0xFF,0xFF,0xFF},{0x7B,0xFF,0x31},{0x00,0xB4,0x00},{0x00,0x00,0x00}}
    },
    {
        "GBC Right",
        {{0xFF,0xFF,0xFF},{0x52,0xFF,0x00},{0xFF,0x42,0x00},{0x00,0x00,0x00}},
        {{0xFF,0xFF,0xFF},{0x52,0xFF,0x00},{0xFF,0x42,0x00},{0x00,0x00,0x00}},
        {{0xFF,0xFF,0xFF},{0x52,0xFF,0x00},{0xFF,0x42,0x00},{0x00,0x00,0x00}}
    },
    {
        "GBC A + Right",
        {{0xFF,0xFF,0xFF},{0x7B,0xFF,0x31},{0x00,0x03,0xC5},{0x00,0x00,0x00}},
        {{0xFF,0xFF,0xFF},{0xFF,0xB4,0x84},{0x94,0x3A,0x3A},{0x00,0x00,0x00}},
        {{0xFF,0xFF,0xFF},{0xFF,0xB4,0x84},{0x94,0x3A,0x3A},{0x00,0x00,0x00}}
    },
    {
        "GBC B + Right",
        {{0x00,0x00,0x00},{0x00,0x84,0x84},{0xFF,0xDE,0x00},{0xFF,0xFF,0xFF}},
        {{0x00,0x00,0x00},{0x00,0x84,0x84},{0xFF,0xDE,0x00},{0xFF,0xFF,0xFF}},
        {{0x00,0x00,0x00},{0x00,0x84,0x84},{0xFF,0xDE,0x00},{0xFF,0xFF,0xFF}}
    },
};

static const int palette_presets_count = sizeof(palette_presets) / sizeof(palette_presets[0]);


void dmg_palette_t::init(gb_t* _gb){
    
    gb = _gb;
    
    const palette_preset_t* preset = palette_presets + current_preset;

    for(int i = 0; i < gb_palette_colors; ++i){
        gb->palette.bgp_colors[i] = preset->bgp[i];
        gb->palette.obp_colors[0][i] = preset->obp0[i];
        gb->palette.obp_colors[1][i] = preset->obp1[i];
    }

    update_window_size_constraints();
}


void dmg_palette_t::update_window_size_constraints(){
    ImGuiStyle& style = ImGui::GetStyle();

    float title_bar_height = ImGui::GetFrameHeight();

    float table_height = (ImGui::GetFrameHeight() + style.CellPadding.y * 2.0f) * 4.0f;

    float button_height = ImGui::GetFrameHeightWithSpacing();

    float window_height = title_bar_height + table_height + button_height + style.WindowPadding.y * 2.0f;

    window_max_size.x = FLT_MAX;
    window_max_size.y = window_height;

    window_min_size.x = style.WindowMinSize.x;
    window_min_size.y = window_height;
}


void dmg_palette_t::save(cJSON* object){

    cJSON* dmg_palette_object = cJSON_CreateObject();
    cJSON_AddItemToObjectCS(object,"DMG Palette",dmg_palette_object);

    cJSON* preset_number = cJSON_CreateNumber(current_preset);
    cJSON_AddItemToObject(dmg_palette_object,"Preset",preset_number);

    cJSON* bgp_array = cJSON_CreateArray();
    cJSON_AddItemToObjectCS(dmg_palette_object,"Background",bgp_array);

    cJSON* obj0_array = cJSON_CreateArray();
    cJSON_AddItemToObjectCS(dmg_palette_object,"Object 0",obj0_array);

    cJSON* obj1_array = cJSON_CreateArray();
    cJSON_AddItemToObjectCS(dmg_palette_object,"Object 1",obj1_array);

    for(int i = 0; i < gb_palette_colors; ++i){

        cJSON* bg_color_array = cJSON_CreateArray();
        cJSON_AddItemToArray(bgp_array,bg_color_array);

        cJSON_AddItemToArray(bg_color_array,cJSON_CreateNumber(gb->palette.bgp_colors[i].r));
        cJSON_AddItemToArray(bg_color_array,cJSON_CreateNumber(gb->palette.bgp_colors[i].g));
        cJSON_AddItemToArray(bg_color_array,cJSON_CreateNumber(gb->palette.bgp_colors[i].b));

        cJSON* obj0_color_array = cJSON_CreateArray();
        cJSON_AddItemToArray(obj0_array,obj0_color_array);

        cJSON_AddItemToArray(obj0_color_array,cJSON_CreateNumber(gb->palette.obp_colors[0][i].r));
        cJSON_AddItemToArray(obj0_color_array,cJSON_CreateNumber(gb->palette.obp_colors[0][i].g));
        cJSON_AddItemToArray(obj0_color_array,cJSON_CreateNumber(gb->palette.obp_colors[0][i].b));

        cJSON* obj1_color_array = cJSON_CreateArray();
        cJSON_AddItemToArray(obj1_array,obj1_color_array);

        cJSON_AddItemToArray(obj1_color_array,cJSON_CreateNumber(gb->palette.obp_colors[1][i].r));
        cJSON_AddItemToArray(obj1_color_array,cJSON_CreateNumber(gb->palette.obp_colors[1][i].g));
        cJSON_AddItemToArray(obj1_color_array,cJSON_CreateNumber(gb->palette.obp_colors[1][i].b));
    }
}

void dmg_palette_t::load(cJSON* object){
    cJSON* dmg_palette_object = cJSON_GetObjectItemCaseSensitive(object,"DMG Palette");

    if(!dmg_palette_object || !cJSON_IsObject(dmg_palette_object)) return;

    cJSON* preset_number = cJSON_GetObjectItemCaseSensitive(dmg_palette_object,"Preset");

    if(preset_number && cJSON_IsNumber(preset_number)){
        
        double value = cJSON_GetNumberValue(preset_number);
        
        if(value >= 0 && value < palette_presets_count){
            current_preset = value;
        }
    }

    cJSON* bgp_array = cJSON_GetObjectItemCaseSensitive(dmg_palette_object,"Background");

    if(bgp_array && cJSON_IsArray(bgp_array)){

        for(int i = 0; i < gb_palette_colors; ++i){
            
            cJSON* bg_color_array = cJSON_GetArrayItem(bgp_array,i);

            if(bg_color_array && cJSON_IsArray(bg_color_array)){

                cJSON* r_number = cJSON_GetArrayItem(bg_color_array,0);
                
                if(r_number && cJSON_IsNumber(r_number)){
                    gb->palette.bgp_colors[i].r = cJSON_GetNumberValue(r_number);
                }

                cJSON* g_number = cJSON_GetArrayItem(bg_color_array,1);

                if(g_number && cJSON_IsNumber(g_number)){
                    gb->palette.bgp_colors[i].g = cJSON_GetNumberValue(g_number);
                }

                cJSON* b_number = cJSON_GetArrayItem(bg_color_array,2);

                if(b_number && cJSON_IsNumber(b_number)){
                    gb->palette.bgp_colors[i].b = cJSON_GetNumberValue(b_number);
                }
            }
        }
    }

    cJSON* obj0_array = cJSON_GetObjectItemCaseSensitive(dmg_palette_object,"Object 0");

    if(obj0_array && cJSON_IsArray(obj0_array)){

        for(int i = 0; i < gb_palette_colors; ++i){
            
            cJSON* obj0_color_array = cJSON_GetArrayItem(obj0_array,i);

            if(obj0_color_array && cJSON_IsArray(obj0_color_array)){

                cJSON* r_number = cJSON_GetArrayItem(obj0_color_array,0);
                
                if(r_number && cJSON_IsNumber(r_number)){
                    gb->palette.obp_colors[0][i].r = cJSON_GetNumberValue(r_number);
                }

                cJSON* g_number = cJSON_GetArrayItem(obj0_color_array,1);

                if(g_number && cJSON_IsNumber(g_number)){
                    gb->palette.obp_colors[0][i].g = cJSON_GetNumberValue(g_number);
                }

                cJSON* b_number = cJSON_GetArrayItem(obj0_color_array,2);

                if(b_number && cJSON_IsNumber(b_number)){
                    gb->palette.obp_colors[0][i].b = cJSON_GetNumberValue(b_number);
                }
            }
        }
    }

    cJSON* obj1_array = cJSON_GetObjectItemCaseSensitive(dmg_palette_object,"Object 1");

    if(obj1_array && cJSON_IsArray(obj1_array)){

        for(int i = 0; i < gb_palette_colors; ++i){
            
            cJSON* obj1_color_array = cJSON_GetArrayItem(obj1_array,i);

            if(obj1_color_array && cJSON_IsArray(obj1_color_array)){

                cJSON* r_number = cJSON_GetArrayItem(obj1_color_array,0);
                
                if(r_number && cJSON_IsNumber(r_number)){
                    gb->palette.obp_colors[1][i].r = cJSON_GetNumberValue(r_number);
                }

                cJSON* g_number = cJSON_GetArrayItem(obj1_color_array,1);

                if(g_number && cJSON_IsNumber(g_number)){
                    gb->palette.obp_colors[1][i].g = cJSON_GetNumberValue(g_number);
                }

                cJSON* b_number = cJSON_GetArrayItem(obj1_color_array,2);

                if(b_number && cJSON_IsNumber(b_number)){
                    gb->palette.obp_colors[1][i].b = cJSON_GetNumberValue(b_number);
                }
            }
        }
    }
}


void dmg_palette_t::render(){
    if(!_open) return;

    ImGui::SetNextWindowSizeConstraints(window_min_size,window_max_size);

    if(ImGui::Begin("DMG Palette",&_open)){
        
        ImGuiStyle& style = ImGui::GetStyle();

        if(ImGui::BeginTable("Table",2)){

            ImGui::TableSetupColumn(nullptr,ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableSetupColumn(nullptr,ImGuiTableColumnFlags_WidthStretch);

            ImGui::TableNextRow();

            ImGui::TableNextColumn();
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Presets");

            ImGui::TableNextColumn();

            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            if(ImGui::BeginCombo("##PresetsCombo",palette_presets[current_preset].name)){

                for(int i = 0; i < palette_presets_count; ++i){
                    
                    if(ImGui::Selectable(palette_presets[i].name,current_preset == i)){

                        current_preset = i;

                        const palette_preset_t* preset = palette_presets + current_preset;

                        for(int i = 0; i < gb_palette_colors; ++i){

                            temp_bgp[i] = rgb_to_vec4(preset->bgp[i]);
                            temp_obp[0][i] = rgb_to_vec4(preset->obp0[i]);
                            temp_obp[1][i] = rgb_to_vec4(preset->obp1[i]);
                        }
                    }
                }

                ImGui::EndCombo();
            }

            ImGui::TableNextRow();

            ImGui::TableNextColumn();
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Background");

            ImGui::TableNextColumn();

            for(int i = 0; i < gb_palette_colors; ++i){
                ImGui::PushID(i);
                ImGui::ColorEdit3("##BackgroundColor",(float*)(temp_bgp + i),ImGuiColorEditFlags_NoInputs);
                ImGui::SameLine(0.0f,style.ItemInnerSpacing.x);
                ImGui::PopID();
            }

            ImGui::TableNextRow();

            ImGui::TableNextColumn();
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Object 0");

            ImGui::TableNextColumn();

            for(int i = 0; i < gb_palette_colors; ++i){
                ImGui::PushID(i);
                ImGui::ColorEdit3("##Object0Color",(float*)(temp_obp[0] + i),ImGuiColorEditFlags_NoInputs);
                ImGui::SameLine(0.0f,style.ItemInnerSpacing.x);
                ImGui::PopID();
            }

            ImGui::TableNextRow();

            ImGui::TableNextColumn();
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Object 1");

            ImGui::TableNextColumn();

            for(int i = 0; i < gb_palette_colors; ++i){
                ImGui::PushID(i);
                ImGui::ColorEdit3("##Object1Color",(float*)(temp_obp[1] + i),ImGuiColorEditFlags_NoInputs);
                ImGui::SameLine(0.0f,style.ItemInnerSpacing.x);
                ImGui::PopID();
            }

            ImGui::EndTable();
        }

        const char* str_ok = "Ok";
        const char* str_cancel = "Cancel";

        float ok_button_width = ImGui::CalcTextSize(str_ok).x + style.FramePadding.x * 2.0f;
        float cancel_button_width = ImGui::CalcTextSize(str_cancel).x + style.FramePadding.x * 2.0f;

        ImGui::SetCursorPosX((ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x) - (ok_button_width + cancel_button_width + style.ItemSpacing.x));

        if(ImGui::Button(str_ok)) close(false);

        ImGui::SameLine();

        if(ImGui::Button(str_cancel)) close(true);
    }

    ImGui::End();
}


void dmg_palette_t::open() noexcept {
    if(_open) return;

    _open = true;

    for(int i = 0; i < gb_palette_colors; ++i){

        temp_bgp[i] = rgb_to_vec4(gb->palette.bgp_colors[i]);
        
        for(int j = 0; j < gb_dmg_obj_palettes; ++j){

            temp_obp[j][i] = rgb_to_vec4(gb->palette.obp_colors[j][i]);
        }
    }
}

void dmg_palette_t::close(bool discard_changes) noexcept {
    if(!_open) return;

    _open = false;

    if(!discard_changes){
        for(int i = 0; i < gb_palette_colors; ++i){

            gb->palette.bgp_colors[i] = vec4_to_rgb(temp_bgp[i]);
            
            for(int j = 0; j < gb_dmg_obj_palettes; ++j){

                gb->palette.obp_colors[j][i] = vec4_to_rgb(temp_obp[j][i]);
            }
        }

        gb_palette_update_dmg_colors(gb);
    }
}