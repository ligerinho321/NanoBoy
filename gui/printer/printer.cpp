#include <gui/printer/printer.hpp>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <gui/printer/stb_image_write.h>

const char* image_formats[] = {
    "PNG\0.png",
    "BMP\0.bmp",
    "TGA\0.tga",
    "JPG\0.jpg"
};

const int image_formats_count = sizeof(image_formats) / sizeof(image_formats[0]);

printer_t::printer_t(gb_t* _gb,SDL_Renderer* _renderer):gb(_gb),renderer(_renderer){
    file_save.set_callback(file_save_callback,this);
    file_save.set_formats(image_formats,image_formats_count);
}


void printer_t::gb_printer_callback(void* userdata,const uint8_t* data,int len){
    printer_t* printer = (printer_t*)userdata;

    printer->mutex.lock();

    size_t size = printer->buffer.size();

    if(size + len > printer->buffer.capacity()){

        size_t new_cap = printer->buffer.capacity() + gb_max(printer_t::buffer_expand_size,len);

        printer->buffer.reserve(new_cap);    
    }

    printer->buffer.resize(size + len,0);

    memcpy(printer->buffer.data() + size,data,len);

    printer->update_texture.store(true,std::memory_order_release);

    printer->mutex.unlock();
}


void printer_t::file_save_callback(void* userdata,std::filesystem::path path){
    printer_t* printer = (printer_t*)userdata;

    SDL_Rect rect{0,0,printer_t::texture_width,printer->texture_height};
    uint8_t* pixels = nullptr;
    int pitch = 0;

    if(!SDL_LockTexture(printer->texture,&rect,(void**)&pixels,&pitch)){

        std::string extension = path.extension().u8string();

        if(!strcasecmp(extension.c_str(),".png")){
            if(!stbi_write_png(path.u8string().c_str(),printer_t::texture_width,printer->texture_height,printer_t::texture_bytes_per_pixel,pixels,pitch)){
                printf("stbi_write_png: failed\n");
            }
        }
        else if(!strcasecmp(extension.c_str(),".bmp")){
            if(!stbi_write_bmp(path.u8string().c_str(),printer_t::texture_width,printer->texture_height,printer_t::texture_bytes_per_pixel,pixels)){
                printf("stbi_write_bmp: failed\n");
            }
        }
        else if(!strcasecmp(extension.c_str(),".tga")){
            if(!stbi_write_tga(path.u8string().c_str(),printer_t::texture_width,printer->texture_height,printer_t::texture_bytes_per_pixel,pixels)){
                printf("stbi_write_tga: failed\n");
            }
        }
        else if(!strcasecmp(extension.c_str(),".jpg")){
            if(!stbi_write_jpg(path.u8string().c_str(),printer_t::texture_width,printer->texture_height,printer_t::texture_bytes_per_pixel,pixels,100)){
                printf("stbi_write_jpg: failed\n");
            }
        }

        SDL_UnlockTexture(printer->texture);                
    }
    else{
        printf("SDL_LockTexture: %s\n",SDL_GetError());
    }
}


void printer_t::update(){
    if(!update_texture.load(std::memory_order_acquire)) return;

    mutex.lock();

    texture_height = buffer.size() / gb_printer_image_pitch;

    uint8_t* pixels = nullptr;
    int pitch = 0;
    SDL_Rect rect = {0};

    if(!texture_height) goto end;

    if(texture_height > texture_max_height){
        
        texture_max_height += gb_max(printer_t::texture_expand_height,texture_height);

        if(texture != nullptr){
            SDL_DestroyTexture(texture);
            texture = nullptr;
        }

        texture = SDL_CreateTexture(renderer,printer_t::texture_format,printer_t::texture_access,printer_t::texture_width,texture_max_height);
        
        if(!texture){
            printf("SDL_CreateTexture: %s\n",SDL_GetError());
            goto end;
        }
    }

    rect.w = printer_t::texture_width;
    rect.h = texture_height;

    if(SDL_LockTexture(texture,&rect,(void**)&pixels,&pitch) < 0){
        printf("SDL_LockTexture: %s\n",SDL_GetError());
        goto end;
    }

    memcpy(pixels,buffer.data(),gb_printer_image_pitch * texture_height);

    SDL_UnlockTexture(texture);

    end:
    update_texture.store(false,std::memory_order_relaxed);

    mutex.unlock();
}

void printer_t::render(){

    if(!open) return;

    bool _open = open;

    if(ImGui::Begin("Printer",&_open)){

        ImVec2 child_size(
            0.0f,
            ImGui::GetContentRegionAvail().y - ImGui::GetFrameHeightWithSpacing()
        );

        if(ImGui::BeginChild("PrinterChild",child_size,ImGuiChildFlags_Borders)){

            update();

            if(texture != nullptr){

                ImVec2 texture_size(printer_t::texture_width,texture_height);

                ImVec2 texture_uv0(0.0f,0.0f);

                ImVec2 texture_uv1(1.0f,(float)texture_height / (float)texture_max_height);

                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (ImGui::GetContentRegionAvail().x - texture_size.x) * 0.5f);
                
                ImGui::Image((ImTextureRef)texture,texture_size,texture_uv0,texture_uv1);
            }
        }
        ImGui::EndChild();

        ImGui::BeginDisabled(!texture_height);

        if(ImGui::Button("Save")){
            file_save.set_open(true);
        }

        ImGui::SameLine();

        if(ImGui::Button("Clear")){
            mutex.lock();
            
            buffer.clear();
            
            update_texture.store(false,std::memory_order_relaxed);

            mutex.unlock();

            texture_height = 0;
        }

        ImGui::EndDisabled();
        
        ImGui::SameLine();

        if(ImGui::Button("Accelerate")){
            gb_accelerate_printer(gb);
        }
    }
    ImGui::End();

    set_open<true>(_open);

    file_save.render();
}