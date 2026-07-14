#include <gui/object_viewer/object_viewer.hpp>

object_viewer_t::object_viewer_t(gb_t* gb,SDL_Renderer *renderer):gb(gb),obj_palette(renderer){
    
    bg_texture = SDL_CreateTexture(renderer,bg_texture_format,texture_access,bg_texture_width,bg_texture_height);

    load_bg_texture();

    input_scalar_width = get_input_scalar_width();

    ImGuiStyle& style = ImGui::GetStyle();

    oam_table_size = ImVec2(
        ((gb_object_width * oam_table_object_scale) + (style.CellPadding.x * 2.0f)) * oam_table_columns,
        0.0f
    );

    outline_color = IM_COL32(120,120,120,255);
    outline_hovered_color = ImGui::ColorConvertFloat4ToU32(style.Colors[ImGuiCol_NavCursor]);
    obj_bg_color = IM_COL32(128,128,128,255);
    border_color = ImGui::ColorConvertFloat4ToU32(style.Colors[ImGuiCol_Border]);
    border_hovered_color = ImGui::ColorConvertFloat4ToU32(style.Colors[ImGuiCol_NavCursor]);

    update_bg_metrics();

    objects.reserve(gb_oam_objects);
    objects_sorted.reserve(gb_oam_objects);

    for(int i = 0; i < gb_oam_objects; ++i){
        objects_sorted.push_back(&objects.emplace_back(i,renderer));
    }
}

object_viewer_t::~object_viewer_t(){
    gb_thread_safe_remove_ppu_handler(gb,&callback_handler);
    
    SDL_DestroyTexture(bg_texture);
}


void object_viewer_t::callback(void* data){
    object_viewer_t* object_viewer = (object_viewer_t*)data;
    gb_t* gb = object_viewer->gb;
    obj_palette_t& obj_palette = object_viewer->obj_palette;

    object_viewer->cgb_mode = gb->cgb_mode;
    
    object_viewer->obj_priority_mode = gb->obj_priority_mode;

    object_viewer->object_size = gb->ppu.lcdc.object_size;
    
    object_viewer->object_texture_uv1.y = object_viewer->object_size ? 1.0f : 0.5f;

    obj_palette.obp[0] = gb->palette.obp[0];
    obj_palette.obp[1] = gb->palette.obp[1];

    memcpy(obj_palette.colors,gb->palette.obj_cram_converted,sizeof(obj_palette.colors));
    
    memcpy(object_viewer->oam,gb->ppu.oam,sizeof(object_viewer->oam));
    
    memcpy(object_viewer->vram,gb->ppu.vram,sizeof(object_viewer->vram));
}


void object_viewer_t::load_bg_texture(){
    uint8_t* pixels = nullptr;
    int pitch = 0;

    SDL_LockTexture(bg_texture,nullptr,(void**)&pixels,&pitch);

    memset(pixels,0x46,pitch * bg_texture_height);

    gb_rgb_t color[2] = {
        {0,0,0},
        {255,255,255}
    };
    bool color_index = 0;

    uint8_t* on_screen_pixels = pixels + (on_screen_offset_y * pitch) + (on_screen_offset_x * 3);

    for(int row = 0, tile_y = 0; row < gb_screen_rows; ++row, tile_y += gb_tile_size){
        
        for(int col = 0, tile_x = 0; col < gb_screen_columns; ++col, tile_x += gb_tile_size){

            for(int y = 0; y < 8; ++y){

                uint8_t* line_pixel = on_screen_pixels + (tile_y | y) * pitch + tile_x * 3;
                
                for(int x = 0; x < 8; ++x){

                    line_pixel[0] = color[color_index].r;
                    line_pixel[1] = color[color_index].g;
                    line_pixel[2] = color[color_index].b;

                    line_pixel += bg_texture_bytes_per_pixel;
                }
            }

            color_index = !color_index;
        }

        color_index = !color_index;
    }

    SDL_UnlockTexture(bg_texture);
}


void object_viewer_t::update_bg_metrics(){

    if(show_offscreen){

        bg_size = ImVec2(bg_texture_width * scale,bg_texture_height * scale);

        bg_uv0 = ImVec2(0.0f,0.0f);
        bg_uv1 = ImVec2(1.0f,1.0f);

        bg_offset = ImVec2(0.0f,0.0f); 
    }
    else{
        bg_size = ImVec2(gb_screen_width * scale,gb_screen_height * scale);

        bg_uv0 = ImVec2(
            (float)on_screen_offset_x / (float)bg_texture_width,
            (float)on_screen_offset_y / (float)bg_texture_height
        );
        
        bg_uv1 = ImVec2(
            bg_uv0.x + ((float)gb_screen_width / (float)bg_texture_width),
            bg_uv0.y + ((float)gb_screen_height / (float)bg_texture_height)
        );

        bg_offset = ImVec2(-(float)on_screen_offset_x,-(float)on_screen_offset_y);
    }
}

void object_viewer_t::update_object_texture(object_t& object){
    uint8_t* pixels = nullptr;
    int pitch = 0;
    gb_rgb_t color{0};

    uint8_t object_height = object_size ? gb_object_max_height : gb_object_min_height;

    SDL_LockTexture(object.texture,nullptr,(void**)&pixels,&pitch);

    for(uint8_t y = 0; y < object_height; ++y){

        uint16_t address = object.tile_address | ((object.vertical_flip ? (object_size ? 0x0F : 0x07) ^ y : y) << 0x01);

        uint8_t lo = vram[address + 0x00];
        uint8_t hi = vram[address + 0x01];

        for(uint8_t x = 0; x < 8; ++ x){

            uint8_t bit = 0x01 << (object.horizontal_flip ? x : 0x07 ^ x);

            uint8_t color_index = ((hi & bit) ? 0x02 : 0x00) | ((lo & bit) ? 0x01 : 0x00);

            if(gb->type == gb_cgb){
                if(cgb_mode){
                    color = obj_palette.get_cgb_color(object.palette_index,color_index);
                }
                else{
                    color = obj_palette.get_cgb_dmg_color(object.palette_index,color_index);
                }
            }
            else{
                color = obj_palette.get_dmg_color(object.palette_index,color_index);
            }

            uint8_t* pixel = pixels + y * pitch + x * obj_texture_bytes_per_pixel;
            pixel[0] = color.r;
            pixel[1] = color.g;
            pixel[2] = color.b;
            pixel[3] = (color_index != 0) ? 255 : 0;
        }
    }

    SDL_UnlockTexture(object.texture);
}

void object_viewer_t::update_objects(){
    gb_object_t* oam_entry = (gb_object_t*)oam;

    for(auto& object : objects){

        object.y = oam_entry->y;
        object.x = oam_entry->x;
        object.tile_index = oam_entry->tile_index;

        object.tile_address = (cgb_mode && (oam_entry->attribute & 0x08)) ? 0x2000 : 0x0000;
        object.tile_address |= (object.tile_index & (object_size ? 0xFE : 0xFF)) << 0x04;

        object.palette_index = cgb_mode ? oam_entry->attribute & 0x07 : (oam_entry->attribute & 0x10) ? 0x01 : 0x00;

        object.horizontal_flip = oam_entry->attribute & 0x20;
        object.vertical_flip = oam_entry->attribute & 0x40;

        object.priority = oam_entry->attribute & 0x80;

        update_object_texture(object);

        ++oam_entry;
    }

    //DMG priority mode
    if(obj_priority_mode){
        std::sort(objects_sorted.begin(),objects_sorted.end(),[](object_t* a,object_t* b)->bool{
            if(a->x != b->x){
                return a->x > b->x;
            }
            return a->index > b->index;
        });
    }
    //CGB priority mode
    else{
        std::sort(objects_sorted.begin(),objects_sorted.end(),[](object_t* a,object_t* b)->bool{
            return a->index > b->index;
        });
    }
}


void object_viewer_t::render_object_tooltip(object_t* object){

    if(!ImGui::BeginTooltip()) return;

    if(ImGui::BeginTable("ObjectTooltipTable",2)){

        ImDrawList* draw_list = ImGui::GetWindowDrawList();

        //Object
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::TextUnformatted("Object");
        ImGui::TableNextColumn();

        int object_height = object_size ? gb_object_max_height : gb_object_min_height;

        ImVec2 object_texture_size(
            (float)gb_object_width * (float)tooltip_object_scale,
            (float)object_height * (float)tooltip_object_scale
        );

        ImVec2 p_min = ImGui::GetCursorScreenPos();
        ImVec2 p_max = ImVec2(p_min.x + object_texture_size.x,p_min.y + object_texture_size.y);

        draw_list->AddRectFilled(p_min,p_max,obj_bg_color);

        ImGui::Image((ImTextureRef)object->texture,object_texture_size,object_texture_uv0,object_texture_uv1);

        draw_list->AddRect(p_min,p_max,border_color);

        //Palette
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::TextUnformatted("Palette");
        ImGui::TableNextColumn();

        ImVec2 palette_texture_size(
            (float)palette_t::texture_max_width * (float)tooltip_object_palette_scale,
            (float)gb_tile_size * (float)tooltip_object_palette_scale 
        );
        ImVec2 palette_texture_uv0(
            0.0f,
            (cgb_mode ? object->palette_index * gb_tile_size : 0) / (float)palette_t::texture_max_height
        );
        ImVec2 palette_texture_uv1(
            1.0f,
            palette_texture_uv0.y + ((float)gb_tile_size / (float)palette_t::texture_max_height)
        );

        ImGui::Image((ImTextureRef)obj_palette.texture,palette_texture_size,palette_texture_uv0,palette_texture_uv1);

        draw_list->AddRect(ImGui::GetItemRectMin(),ImGui::GetItemRectMax(),border_color);

        //Index
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::TextUnformatted("Index");
        ImGui::TableNextColumn();
        ImGui::Text("%d",object->index);

        //X, Y
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::TextUnformatted("X, Y");
        ImGui::TableNextColumn();
        ImGui::Text("%d, %d",object->x,object->y);

        //Size
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::TextUnformatted("Size");
        ImGui::TableNextColumn();
        ImGui::Text("%dx%d",gb_object_width,object_height);

        //Tile index
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::TextUnformatted("Tile index");
        ImGui::TableNextColumn();
        ImGui::Text("$%02X",object->tile_index);

        //Tile address
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::TextUnformatted("Tile address");
        ImGui::TableNextColumn();
        ImGui::Text("$%04X",object->tile_address);

        //Palette index
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::TextUnformatted("Palette index");
        ImGui::TableNextColumn();
        ImGui::Text("%d",object->palette_index);

        //Horizontal flip
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::TextUnformatted("Horizontal flip");
        ImGui::TableNextColumn();
        (object->horizontal_flip ? ImGui::TextUnformatted("True") : ImGui::TextUnformatted("False"));

        //Vertical flip
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::TextUnformatted("Vertical flip");
        ImGui::TableNextColumn();
        (object->vertical_flip ? ImGui::TextUnformatted("True") : ImGui::TextUnformatted("False"));

        //Priority
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::TextUnformatted("Priority");
        ImGui::TableNextColumn();
        (object->priority ? ImGui::TextUnformatted("True") : ImGui::TextUnformatted("False"));

        ImGui::EndTable();
    }

    ImGui::EndTooltip();
}

void object_viewer_t::render_oam_table(){

    if(!ImGui::BeginTable("OAMTable",oam_table_columns,ImGuiTableFlags_None,oam_table_size)) return;

    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    auto object = objects.begin();

    int object_height = object_size ? gb_object_max_height : gb_object_min_height;

    ImVec2 table_object_size(
        (float)gb_object_width * (float)oam_table_object_scale,
        (float)object_height * (float)oam_table_object_scale
    );

    oam_table_object_hovered = nullptr;

    for(int y = 0; y < oam_table_rows; ++y){
        
        ImGui::TableNextRow();
        
        for(int x = 0; x < oam_table_columns; ++x){

            ImGui::TableNextColumn();

            ImVec2 p_min = ImGui::GetCursorScreenPos();
            ImVec2 p_max(p_min.x + table_object_size.x,p_min.y + table_object_size.y);

            draw_list->AddRectFilled(p_min,p_max,obj_bg_color);

            ImGui::Image((ImTextureRef)object->texture,table_object_size,object_texture_uv0,object_texture_uv1);

            if(ImGui::IsItemHovered()){

                oam_table_object_hovered = &*object;

                draw_list->AddRect(p_min,p_max,border_hovered_color,0.0f,ImDrawFlags_None,2.0f);

                render_object_tooltip(oam_table_object_hovered);
            }
            else{
                draw_list->AddRect(p_min,p_max,border_color);
            }

            ++object;
        }
    }

    ImGui::EndTable();
}

void object_viewer_t::render_oam_screen(){
    if(ImGui::BeginChild("OAMScreen",ImVec2(0.0f,0.0f),ImGuiChildFlags_Borders,ImGuiWindowFlags_HorizontalScrollbar)){
        
        ImGui::Image((ImTextureRef)bg_texture,bg_size,bg_uv0,bg_uv1);

        bool image_hovered = ImGui::IsItemHovered();
        ImVec2 mouse_pos = ImGui::GetMousePos();

        ImVec2 start = ImGui::GetItemRectMin();
        ImVec2 end = ImGui::GetItemRectMax();

        ImDrawList* draw_list = ImGui::GetWindowDrawList();

        draw_list->PushClipRect(start,end,true);

        float object_height = object_size ? gb_object_max_height : gb_object_min_height;

        object_t* oam_screen_object_hovered = nullptr;

        for(auto object : objects_sorted){

            if(object == oam_table_object_hovered) continue;

            ImVec2 rmin (
                start.x + (object->x + bg_offset.x) * scale,
                start.y + (object->y + bg_offset.y) * scale
            );
            
            ImVec2 rmax (
                rmin.x + gb_object_width * scale,
                rmin.y + object_height * scale
            );

            draw_list->AddImage((ImTextureRef)object->texture,rmin,rmax,object_texture_uv0,object_texture_uv1);

            if(image_hovered && mouse_in_rect(mouse_pos,rmin,rmax)){
                oam_screen_object_hovered = object;
            }
            
            if(show_outline){
                draw_list->AddRect(rmin,rmax,outline_color);
            }
        }

        if(oam_screen_object_hovered != nullptr || oam_table_object_hovered != nullptr){

            object_t* object_hovered = oam_table_object_hovered ? oam_table_object_hovered : oam_screen_object_hovered;

            ImVec2 rmin (
                start.x + (object_hovered->x + bg_offset.x) * scale,
                start.y + (object_hovered->y + bg_offset.y) * scale
            );
            
            ImVec2 rmax (
                rmin.x + gb_object_width * scale,
                rmin.y + object_height * scale
            );

            if(object_hovered == oam_table_object_hovered){
                draw_list->AddImage((ImTextureRef)object_hovered->texture,rmin,rmax,object_texture_uv0,object_texture_uv1);
            }

            draw_list->AddRect(rmin,rmax,outline_hovered_color,0.0f,ImDrawFlags_None,2.0f);

            if(object_hovered == oam_screen_object_hovered){
                render_object_tooltip(object_hovered);
            }
        }

        draw_list->PopClipRect();
    }

    ImGui::EndChild();
}

void object_viewer_t::render(){
    if(!open) return;

    bool _open = open;

    if(ImGui::Begin("Object Viewer",&_open)){

        obj_palette.update_texture(gb->type,cgb_mode);
        update_objects();

        if(ImGui::BeginTable("ObjectTable1",2,ImGuiTableFlags_None)){

            ImGui::TableSetupColumn("Left",ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Right",ImGuiTableColumnFlags_WidthFixed);

            ImGui::TableNextRow();

            ImGui::TableNextColumn();

            render_oam_screen();

            ImGui::TableNextColumn();

            render_oam_table();

            if(ImGui::Checkbox("Show offscreen",&show_offscreen)){
                update_bg_metrics();
            }

            ImGui::Checkbox("Show outline",&show_outline);

            ImGui::SetNextItemWidth(input_scalar_width);
            if(ImGui::InputScalar("Refresh on scanline",ImGuiDataType_U8,&callback_handler.scanline,&input_scalar_step,&input_scalar_step_fast)){
                if(callback_handler.scanline >= gb_scanlines){
                    callback_handler.scanline = gb_scanlines - 1;
                }
            }

            ImGui::SetNextItemWidth(input_scalar_width);
            if(ImGui::InputScalar("Refresh on cycle",ImGuiDataType_U16,&callback_handler.cycle,&input_scalar_step,&input_scalar_step_fast)){
                if(callback_handler.cycle >= gb_scanline_cycles){
                    callback_handler.cycle = gb_scanline_cycles - 1;
                }
            }

            ImGui::EndTable();
        }

        if(ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows)){

            if(ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_Equal) && scale < max_scale){
                ++scale;
                update_bg_metrics();
            }
            else if(ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_Minus) && scale > min_scale){
                --scale;
                update_bg_metrics();
            }

            ImGuiIO& io = ImGui::GetIO();
            if(io.KeyCtrl){
                if(io.MouseWheel > 0.0f && scale < max_scale){
                    ++scale;
                    update_bg_metrics();
                }
                else if(io.MouseWheel < 0.0f && scale > min_scale){
                    --scale;
                    update_bg_metrics();
                }
            }
        }
        
    }
    ImGui::End();

    set_open<true>(_open);
}


void object_viewer_t::clear(){
    cgb_mode = false;
    
    obj_priority_mode = false;
    
    object_size = false;

    object_texture_uv1.y = object_size ? 1.0f : 0.5f;
    
    obj_palette.clear();

    memset(oam,0,sizeof(oam));
    memset(vram,0,sizeof(vram));

    for(auto& object : objects){
        object.clear();
    }
}