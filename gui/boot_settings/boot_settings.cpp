#include <gui/boot_settings/boot_settings.hpp>
#include <gui/nanoboy/nanoboy.hpp>

static const char* file_extension = "All files\0";

void boot_settings_t::init(nanoboy_t* _nanoboy){

    nanoboy = _nanoboy;

    file_selector.init();
    file_selector.set_extensions(&file_extension,1);

    update_window_size_constraints();
}


void boot_settings_t::apply_to_console(){
    if(!nanoboy->console) return;

    switch(nanoboy->console->type()){
        case console_gb:{
            gb_t* gb = (gb_t*)nanoboy->console->data();
            gb_boot_set_dmg_path_reference(gb,dmg_path);
            gb_boot_set_cgb_path_reference(gb,cgb_path);
            gb_boot_enable_skip(gb,skip_enabled);
            break;
        }
        case console_gba:{
            gba_t* gba = (gba_t*)nanoboy->console->data();
            gba_boot_set_path_reference(gba,gba_path);
            gba_boot_enable_skip(gba,skip_enabled);
            break;
        }
    }
}


void boot_settings_t::update_window_size_constraints(){
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


void boot_settings_t::file_selector_dmg_callback(void* userdata,std::filesystem::path path){
    boot_settings_t* boot_settings = (boot_settings_t*)userdata;
    
    std::string string = path.u8string();
    
    if(string.length() >= boot_settings_t::buffer_length) return;

    strcpy(boot_settings->temp_dmg_path,string.c_str());
}

void boot_settings_t::file_selector_cgb_callback(void* userdata,std::filesystem::path path){
    boot_settings_t* boot_settings = (boot_settings_t*)userdata;

    std::string string = path.u8string();

    if(string.length() >= boot_settings_t::buffer_length) return;

    strcpy(boot_settings->temp_cgb_path,string.c_str());
}

void boot_settings_t::file_selector_gba_callback(void* userdata,std::filesystem::path path){
    boot_settings_t* boot_settings = (boot_settings_t*)userdata;

    std::string string = path.u8string();

    if(string.length() >= boot_settings_t::buffer_length) return;

    strcpy(boot_settings->temp_gba_path,string.c_str());
}


void boot_settings_t::save(cJSON* object){
    cJSON* boot_settings_object = cJSON_CreateObject();
    cJSON_AddItemToObjectCS(object,"Boot Settings",boot_settings_object);

    cJSON* dmg_path_string = cJSON_CreateStringReference(dmg_path);
    cJSON_AddItemToObjectCS(boot_settings_object,"DMG Path",dmg_path_string);

    cJSON* cgb_path_string = cJSON_CreateStringReference(cgb_path);
    cJSON_AddItemToObjectCS(boot_settings_object,"CGB Path",cgb_path_string);

    cJSON* gba_path_string = cJSON_CreateStringReference(gba_path);
    cJSON_AddItemToObjectCS(boot_settings_object,"GBA Path",gba_path_string);

    cJSON* skip_enabled_bool = cJSON_CreateBool(skip_enabled);
    cJSON_AddItemToObjectCS(boot_settings_object,"Skip Enabled",skip_enabled_bool);

    file_selector.save(boot_settings_object);
}

void boot_settings_t::load(cJSON* object){
    cJSON* boot_settings_object = cJSON_GetObjectItemCaseSensitive(object,"Boot Settings");
    
    if(!cJSON_IsObject(boot_settings_object)) return;

    cJSON* dmg_path_string = cJSON_GetObjectItemCaseSensitive(boot_settings_object,"DMG Path");

    if(cJSON_IsString(dmg_path_string)){
        
        char* string = cJSON_GetStringValue(dmg_path_string);
        
        size_t len = strlen(string);

        if(len < sizeof(dmg_path)){
            strcpy(dmg_path,string);
        }
    }

    cJSON* cgb_path_string = cJSON_GetObjectItemCaseSensitive(boot_settings_object,"CGB Path");

    if(cJSON_IsString(cgb_path_string)){
        
        char* string = cJSON_GetStringValue(cgb_path_string);
        
        size_t len = strlen(string);

        if(len < sizeof(cgb_path)){
            strcpy(cgb_path,string);
        }
    }

    cJSON* gba_path_string = cJSON_GetObjectItemCaseSensitive(boot_settings_object,"GBA Path");

    if(cJSON_IsString(gba_path_string)){
        char* string = cJSON_GetStringValue(gba_path_string);

        size_t len = strlen(string);

        if(len < sizeof(gba_path)){
            strcpy(gba_path,string);
        }
    }

    cJSON* skip_enabled_bool = cJSON_GetObjectItemCaseSensitive(boot_settings_object,"Skip Enabled");
    
    if(cJSON_IsBool(skip_enabled_bool)){
        skip_enabled = cJSON_IsTrue(skip_enabled_bool);
    }

    file_selector.load(boot_settings_object);
}


void boot_settings_t::render(){
    if(!_open) return;

    ImGui::SetNextWindowSizeConstraints(window_min_size,window_max_size);

    if(ImGui::Begin("Boot",&_open)){

        ImGuiStyle& style = ImGui::GetStyle();

        if(ImGui::BeginTable("BootTable",2)){

            ImGui::TableSetupColumn(nullptr,ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableSetupColumn(nullptr,ImGuiTableColumnFlags_WidthStretch);
            
            float browser_button_width = ImGui::CalcTextSize("...").x + style.FramePadding.x * 2.0f;

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("DMG Boot Path");
            ImGui::TableNextColumn();

            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - browser_button_width - style.ItemInnerSpacing.x);
            
            ImGui::InputText("##DMGBootPathTextInput",temp_dmg_path,sizeof(temp_dmg_path));
            
            ImGui::SameLine(0.0f,style.ItemInnerSpacing.x);

            if(ImGui::Button("...##DMGBootPathBrowserButton")){
                file_selector.set_callback(file_selector_dmg_callback,this);
                file_selector.set_open(true);
            }

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("CGB Boot Path");
            ImGui::TableNextColumn();

            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - browser_button_width - style.ItemInnerSpacing.x);
            
            ImGui::InputText("##CGBBootPathTextInput",temp_cgb_path,sizeof(temp_cgb_path));
            
            ImGui::SameLine(0.0f,style.ItemInnerSpacing.x);

            if(ImGui::Button("...##CGBBootPathBrowserButton")){
                file_selector.set_callback(file_selector_cgb_callback,this);
                file_selector.set_open(true);
            }

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("GBA Boot Path");
            ImGui::TableNextColumn();

            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - browser_button_width - style.ItemInnerSpacing.x);
            
            ImGui::InputText("##GBABootPathTextInput",temp_gba_path,sizeof(temp_gba_path));
            
            ImGui::SameLine(0.0f,style.ItemInnerSpacing.x);

            if(ImGui::Button("...##GBABootPathBrowserButton")){
                file_selector.set_callback(file_selector_gba_callback,this);
                file_selector.set_open(true);
            }

            ImGui::EndTable();
        }

        ImGui::Checkbox("Skip Boot",&temp_skip_enabled);

        ImGui::SameLine();
        
        const char* str_ok = "Ok";
        const char* str_cancel = "Cancel";

        float ok_button_width = ImGui::CalcTextSize(str_ok).x + style.FramePadding.x * 2.0f;
        float cancel_button_width = ImGui::CalcTextSize(str_cancel).x + style.FramePadding.x * 2.0f;

        ImGui::SetCursorPosX((ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x) - (ok_button_width + cancel_button_width + style.ItemSpacing.x));

        if(ImGui::Button(str_ok)) close(false);

        ImGui::SameLine();
        
        if(ImGui::Button(str_cancel)) close(true);

        file_selector.render();
    }
    ImGui::End();
}


void boot_settings_t::open() noexcept {
    if(_open) return;

    _open = true;

    memcpy(temp_dmg_path,dmg_path,sizeof(temp_dmg_path));
    memcpy(temp_cgb_path,cgb_path,sizeof(temp_cgb_path));
    memcpy(temp_gba_path,gba_path,sizeof(temp_gba_path));
    temp_skip_enabled = skip_enabled;
}

void boot_settings_t::close(bool discard_changes) noexcept {
    if(!_open) return;

    _open = false;

    if(!discard_changes){
        memcpy(dmg_path,temp_dmg_path,sizeof(dmg_path));
        memcpy(cgb_path,temp_cgb_path,sizeof(cgb_path));
        memcpy(gba_path,temp_gba_path,sizeof(gba_path));
        skip_enabled = temp_skip_enabled;

        apply_to_console();
    }
}