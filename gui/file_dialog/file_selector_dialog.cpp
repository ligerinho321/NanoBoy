#include <gui/file_dialog/file_selector_dialog.hpp>

bool file_selector_t::is_current_extension(std::string extension) const noexcept {
    
    if(!extensions || extensions_count <= 0 || !extension.length()) return false;

    bool result = false;

    const char* current_extension_ptr = get_current_extension();
    size_t current_extension_length = strlen(current_extension_ptr);

    const char* token = nullptr;
    size_t token_length = 0;

    bool start = true;

    while(true){

        if(!current_extension_length) break;

        if(start){
            if(*current_extension_ptr == '.'){
                start = false;
                token = current_extension_ptr;
                token_length = 1;
            }
            else{
                break;
            }
        }
        else{
            if(*current_extension_ptr == ';' || current_extension_length == 1){
                
                if(current_extension_length == 1){
                    ++token_length;
                }

                if(
                    (token_length == 2 && token[1] == '*') ||
                    (token_length == extension.length() && !strncasecmp(token,extension.c_str(),token_length))
                ){
                    result = true;
                    break;
                }

                start = true;
            }
            else{
                ++token_length;
            }
        }

        ++current_extension_ptr;
        --current_extension_length;
    }

    return result;
}

void file_selector_t::select_file(std::filesystem::path& path){
    if(callback != nullptr){
        callback(userdata,path);
    }
    open = false;
}

void file_selector_t::send_name_buffer(){
    std::filesystem::path path = get_name_buffer_formated();
    
    if(std::filesystem::exists(path)){
        if(std::filesystem::is_directory(path)){
            set_current_path(path);
            load_current_directory_entries();
            sort_current_directory_entries();
        }
        else if(std::filesystem::is_regular_file(path)){
            select_file(path);
        }
    }

    clear_name_buffer();
}