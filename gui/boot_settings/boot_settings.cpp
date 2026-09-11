#include <gui/boot_settings/boot_settings.hpp>

static const char* file_extension = "All files\0";

boot_settings_t::boot_settings_t(gb_t* gb):gb(gb){

    file_selector.set_extensions(&file_extension,1);

    gb_set_dmg_rom_path_reference(gb,dmg_path);
    gb_set_cgb_rom_path_reference(gb,cgb_path);

    gb_enable_skip_boot(gb,skip_enabled);

    update_window_size_constraints();
}

boot_settings_t::~boot_settings_t(){
    gb_remove_dmg_rom_path_reference(gb);
    gb_remove_cgb_rom_path_reference(gb);
}


void boot_settings_t::update_window_size_constraints(){
    ImGuiStyle& style = ImGui::GetStyle();

    float title_bar_height = ImGui::GetFrameHeight();

    float content_height = ImGui::GetFrameHeightWithSpacing() * 3.0f;

    float window_height = title_bar_height +  content_height + style.WindowPadding.y * 2.0f;

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


void boot_settings_t::save(cJSON* object){
    cJSON* boot_settings_object = cJSON_CreateObject();
    cJSON_AddItemToObjectCS(object,"Boot settings",boot_settings_object);

    cJSON* dmg_path_string = cJSON_CreateStringReference(dmg_path);
    cJSON_AddItemToObjectCS(boot_settings_object,"DMG path",dmg_path_string);

    cJSON* cgb_path_string = cJSON_CreateStringReference(cgb_path);
    cJSON_AddItemToObjectCS(boot_settings_object,"CGB path",cgb_path_string);

    cJSON* skip_enabled_bool = cJSON_CreateBool(skip_enabled);
    cJSON_AddItemToObjectCS(boot_settings_object,"Skip enabled",skip_enabled_bool);

    file_selector.save(boot_settings_object);
}

void boot_settings_t::load(cJSON* object){
    cJSON* boot_settings_object = cJSON_GetObjectItemCaseSensitive(object,"Boot settings");
    
    if(!boot_settings_object && !cJSON_IsObject(boot_settings_object)) return;

    cJSON* dmg_path_string = cJSON_GetObjectItemCaseSensitive(boot_settings_object,"DMG path");

    if(dmg_path_string && cJSON_IsString(dmg_path_string)){
        
        char* string = cJSON_GetStringValue(dmg_path_string);
        
        size_t len = strlen(string);

        if(len < sizeof(dmg_path)){
            strcpy(dmg_path,string);
        }
    }

    cJSON* cgb_path_string = cJSON_GetObjectItemCaseSensitive(boot_settings_object,"CGB path");

    if(cgb_path_string && cJSON_IsString(cgb_path_string)){
        
        char* string = cJSON_GetStringValue(cgb_path_string);
        
        size_t len = strlen(string);

        if(len < sizeof(cgb_path)){
            strcpy(cgb_path,string);
        }
    }

    cJSON* skip_enabled_bool = cJSON_GetObjectItemCaseSensitive(boot_settings_object,"Skip enabled");
    
    if(skip_enabled_bool && cJSON_IsBool(skip_enabled_bool)){
        
        skip_enabled = cJSON_IsTrue(skip_enabled_bool);

        gb_enable_skip_boot(gb,skip_enabled);
    }

    file_selector.load(boot_settings_object);
}


void boot_settings_t::render(){
    if(!_open) return;

    ImGui::SetNextWindowSizeConstraints(window_min_size,window_max_size);

    if(ImGui::Begin("Boot",&_open)){

        ImGuiStyle& style = ImGui::GetStyle();
        
        float browser_button_width = ImGui::CalcTextSize("...").x + style.FramePadding.x * 2.0f;

        ImGui::AlignTextToFramePadding();
        
        ImGui::TextUnformatted("DMG boot path:");
        
        ImGui::SameLine(0.0f,style.ItemInnerSpacing.x);
        
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - browser_button_width - style.ItemInnerSpacing.x);
        
        ImGui::InputText("##DMGBootPathTextInput",temp_dmg_path,sizeof(temp_dmg_path));
        
        ImGui::SameLine(0.0f,style.ItemInnerSpacing.x);

        if(ImGui::Button("...##DMGBootPathBrowserButton")){
            file_selector.set_callback(file_selector_dmg_callback,this);
            file_selector.set_open(true);
        }


        ImGui::AlignTextToFramePadding();
        
        ImGui::TextUnformatted("CGB boot path:");

        ImGui::SameLine(0.0f,style.ItemInnerSpacing.x);

        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - browser_button_width - style.ItemInnerSpacing.x);
        
        ImGui::InputText("##CGBBootPathTextInput",temp_cgb_path,sizeof(temp_cgb_path));
        
        ImGui::SameLine(0.0f,style.ItemInnerSpacing.x);

        if(ImGui::Button("...##CGBBootPathBrowserButton")){
            file_selector.set_callback(file_selector_cgb_callback,this);
            file_selector.set_open(true);
        }

        ImGui::Checkbox("Skip boot",&temp_skip_enabled);

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
    temp_skip_enabled = skip_enabled;
}

void boot_settings_t::close(bool discard_changes) noexcept {
    if(!_open) return;

    _open = false;

    if(!discard_changes){
        memcpy(dmg_path,temp_dmg_path,sizeof(dmg_path));
        memcpy(cgb_path,temp_cgb_path,sizeof(cgb_path));
        skip_enabled = temp_skip_enabled;

        gb_enable_skip_boot(gb,skip_enabled);
    }
}