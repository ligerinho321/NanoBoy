#include "savestate.h"
#include "gb.h"

#include <zstd.h>

#define gb_savestate_zstd_compress_level ZSTD_CLEVEL_DEFAULT


bool gb_savestate_serialize(gb_t* gb,const char* filename){
    FILE* file = fopen(filename,"wb");

    if(!file){
        gb_printf_errno(fopen);
        return false;
    }

    uint8_t* state_cbuff = NULL;
    uint8_t* screen_cbuff = NULL;

    gb_state_t state = {0};

    gb_state_write(&state,gb->cgb_mode);
    gb_state_write(&state,gb->double_speed);
    gb_state_write(&state,gb->speed_switch_needed);
    gb_state_write(&state,gb->obj_priority_mode);
    gb_state_write_ex(&state,gb->undocumented_registers,sizeof(gb->undocumented_registers));
    gb_state_write(&state,gb->cycle);

    gb_cpu_save_state(&gb->cpu,&state);
    gb_ppu_save_state(&gb->ppu,&state);
    gb_apu_save_state(&gb->apu,&state);
    gb_joypad_save_state(&gb->joypad,&state);
    gb_interrupt_save_state(&gb->interrupt,&state);
    gb_timer_save_state(&gb->timer,&state);
    gb_dma_save_state(&gb->dma,&state);
    gb_palette_save_state(&gb->palette,&state);
    gb_serial_save_state(&gb->serial,&state);
    gb_infrared_save_state(&gb->infrared,&state);
    gb_memory_save_state(&gb->memory,&state);
    gb_cartridge_save_state(&gb->cartridge,&state);

    if(!state.length) goto fail;

    size_t state_cbuff_size = ZSTD_compressBound(state.length);
    
    state_cbuff = (uint8_t*)malloc(state_cbuff_size);

    if(!state_cbuff){
        gb_printf_errno(malloc);
        goto fail;
    }
    
    size_t state_compress_result = ZSTD_compress(state_cbuff,state_cbuff_size,state.data,state.length,gb_savestate_zstd_compress_level);

    if(ZSTD_isError(state_compress_result)){
        gb_printf_error("ZSTD_compress failed");
        goto fail;
    }

    gb_state_header_t state_header = {0};
    memcpy(state_header.magic,gb_state_header_magic,4);
    state_header.decompressed_crc32 = gb_crc32(state.data,state.length);
    state_header.compressed_size = state_compress_result;

    size_t screen_cbuff_size = ZSTD_compressBound(gb_screen_length);

    screen_cbuff = (uint8_t*)malloc(screen_cbuff_size);

    if(!screen_cbuff){
        gb_printf_errno(malloc);
        goto fail;
    }

    size_t screen_compress_result = ZSTD_compress(screen_cbuff,screen_cbuff_size,gb_get_render_buffer(gb),gb_screen_length,gb_savestate_zstd_compress_level);

    if(ZSTD_isError(screen_compress_result)){
        gb_printf_error("ZSTD_compress failed");
        goto fail;
    }

    gb_screenshot_header_t screen_header = {0};
    memcpy(screen_header.magic,gb_screenshot_header_magic,4);
    screen_header.compressed_size = screen_compress_result;

    gb_savestate_header_t header = {0};
    
    memcpy(header.magic,gb_savestate_header_magic,4);

    header.is_cgb = gb->is_cgb;

    if(gb->boot.mapped){
        header.boot_mapped = true;
        header.boot_rom_crc32 = header.is_cgb ? gb->boot.cgb_rom_crc32 : gb->boot.dmg_rom_crc32;
    }

    header.rom_crc32 = gb->cartridge.rom_crc32;
    
    header.timestamp = time(NULL);
    
    header.state_offset = (
        sizeof(gb_savestate_header_t) +
        sizeof(gb_screenshot_header_t) +
        screen_header.compressed_size
    );

    fwrite(&header,sizeof(gb_savestate_header_t),1,file);
    
    fwrite(&screen_header,sizeof(gb_screenshot_header_t),1,file);
    fwrite(screen_cbuff,1,screen_header.compressed_size,file);

    fwrite(&state_header,sizeof(gb_state_header_t),1,file);
    fwrite(state_cbuff,1,state_header.compressed_size,file);

    free(state.data);
    free(state_cbuff);
    free(screen_cbuff);
    
    fclose(file);
    
    return true;

    fail:
    if(state.data != NULL) free(state.data);
    if(state_cbuff != NULL) free(state_cbuff);
    if(screen_cbuff != NULL) free(screen_cbuff);
    
    fclose(file);

    return false;
}

bool gb_savestate_deserialize(gb_t* gb,const char* filename){
    FILE* file = fopen(filename,"rb");

    if(!file){
        gb_printf_errno(fopen);
        return false;
    }

    fseek(file,0,SEEK_END);
    size_t size = ftell(file);
    fseek(file,0,SEEK_SET);

    uint8_t* state_cbuff = NULL;
    uint8_t* state_dbuff = NULL;

    if(size < sizeof(gb_savestate_header_t)) goto fail;


    gb_savestate_header_t header = {0};

    fread(&header,sizeof(gb_savestate_header_t),1,file);

    if(memcmp(header.magic,gb_savestate_header_magic,4)) goto fail;

    if(header.boot_mapped){

        gb_boot_update_roms(&gb->boot);

        if(header.is_cgb){
            if(!gb->boot.cgb_rom_inserted || gb->boot.cgb_rom_crc32 != header.boot_rom_crc32) goto fail;
        }
        else{
            if(!gb->boot.dmg_rom_inserted || gb->boot.dmg_rom_crc32 != header.boot_rom_crc32) goto fail;
        }

    }

    if(header.rom_crc32 != gb->cartridge.rom_crc32) goto fail;

    if(size < header.state_offset + sizeof(gb_state_header_t)) goto fail;

    fseek(file,header.state_offset,SEEK_SET);
    

    gb_state_header_t state_header = {0};

    fread(&state_header,sizeof(gb_state_header_t),1,file);

    if(memcmp(state_header.magic,gb_state_header_magic,4)) goto fail;

    if(size < header.state_offset + sizeof(gb_state_header_t) + state_header.compressed_size) goto fail;

    state_cbuff = (uint8_t*)malloc(state_header.compressed_size);

    if(!state_cbuff){
        gb_printf_errno(malloc);
        goto fail;
    }

    fread(state_cbuff,1,state_header.compressed_size,file);

    size_t dsize = ZSTD_getFrameContentSize(state_cbuff,state_header.compressed_size);

    if(dsize == ZSTD_CONTENTSIZE_UNKNOWN || dsize == ZSTD_CONTENTSIZE_ERROR){
        gb_printf_error("ZSTD_getFrameContentSize failed");
        goto fail;
    }

    state_dbuff = (uint8_t*)malloc(dsize);

    if(!state_dbuff){
        gb_printf_errno(malloc);
        goto fail;
    }

    size_t result = ZSTD_decompress(state_dbuff,dsize,state_cbuff,state_header.compressed_size);

    if(ZSTD_isError(result)){
        gb_printf_error("ZSTD_decompress failed");
        goto fail;
    }

    gb_state_t state = {0};
    state.data = state_dbuff;
    state.capacity = dsize;
    state.length = result;

    if(gb_crc32(state.data,state.length) != state_header.decompressed_crc32) goto fail;

    gb->is_cgb = header.is_cgb;
    
    if(header.boot_mapped){
        gb_boot_map(&gb->boot);
    }
    else{
        gb_boot_unmap(&gb->boot);
    }

    gb_state_read(&state,gb->cgb_mode);
    gb_state_read(&state,gb->double_speed);
    gb_state_read(&state,gb->speed_switch_needed);
    gb_state_read(&state,gb->obj_priority_mode);
    gb_state_read_ex(&state,gb->undocumented_registers,sizeof(gb->undocumented_registers));
    gb_state_read(&state,gb->cycle);

    gb_cpu_load_state(&gb->cpu,&state);
    gb_ppu_load_state(&gb->ppu,&state);
    gb_apu_load_state(&gb->apu,&state);
    gb_joypad_load_state(&gb->joypad,&state);
    gb_interrupt_load_state(&gb->interrupt,&state);
    gb_timer_load_state(&gb->timer,&state);
    gb_dma_load_state(&gb->dma,&state);
    gb_palette_load_state(&gb->palette,&state);
    gb_serial_load_state(&gb->serial,&state);
    gb_infrared_load_state(&gb->infrared,&state);
    gb_memory_load_state(&gb->memory,&state);
    gb_cartridge_load_state(&gb->cartridge,&state);

    gb_event_manager_reset(&gb->event_manager);

    gb->breakpoint_manager.last_check_address = -1;
    
    free(state_cbuff);
    free(state_dbuff);
    
    fclose(file);

    return true;

    fail:
    if(state_cbuff != NULL) free(state_cbuff);
    if(state_dbuff != NULL) free(state_dbuff);
    
    fclose(file);

    return false;
}


bool gb_savestate_get_info(const char* filename,gb_savestate_info_t* info){
    
    static uint8_t screen_dbuff[gb_screen_length] = {0};

    FILE* file = fopen(filename,"rb");

    if(!file){
        gb_printf_errno(fopen);
        return false;
    }

    fseek(file,0,SEEK_END);
    size_t size = ftell(file);
    fseek(file,0,SEEK_SET);

    uint8_t* screen_cbuff = NULL;

    if(size < sizeof(gb_savestate_header_t) + sizeof(gb_screenshot_header_t)) goto fail;


    gb_savestate_header_t header = {0};

    fread(&header,sizeof(gb_savestate_header_t),1,file);

    if(memcmp(header.magic,gb_savestate_header_magic,4)) goto fail;


    gb_screenshot_header_t screen_header = {0};
    
    fread(&screen_header,sizeof(gb_screenshot_header_t),1,file);

    if(memcmp(screen_header.magic,gb_screenshot_header_magic,4)) goto fail;


    if(size < (size_t)(ftell(file) + screen_header.compressed_size)) goto fail;


    screen_cbuff = (uint8_t*)malloc(screen_header.compressed_size);

    if(!screen_cbuff){
        gb_printf_errno(malloc);
        goto fail;
    }

    fread(screen_cbuff,1,screen_header.compressed_size,file);


    size_t screen_decompress_size = ZSTD_getFrameContentSize(screen_cbuff,screen_header.compressed_size);

    if(screen_decompress_size != sizeof(screen_dbuff)) goto fail;

    size_t screen_decompress_result = ZSTD_decompress(screen_dbuff,sizeof(screen_dbuff),screen_cbuff,screen_header.compressed_size);

    if(screen_decompress_result != sizeof(screen_dbuff)) goto fail;


    info->rom_crc32 = header.rom_crc32;
    info->timestamp = header.timestamp;
    info->screenshot = screen_dbuff;
    info->screenshot_length = sizeof(screen_dbuff);

    free(screen_cbuff);
    
    fclose(file);

    return true;

    fail:
    if(screen_cbuff != NULL) free(screen_cbuff);

    fclose(file);
    
    return false;
}


static bool gb_state_expand_capacity(gb_state_t* state,size_t new_capacity){

    if(new_capacity <= state->capacity) return true;

    void* ptr = realloc(state->data,new_capacity);

    if(!ptr){
        gb_printf_errno(realloc);
        return false;
    }

    state->data = (uint8_t*)ptr;
    state->capacity = new_capacity;

    return true;
}

void gb_state_write_ex(gb_state_t* state,const void* src,size_t len){
    if(state->length + len > state->capacity){
        gb_state_expand_capacity(state,state->capacity + gb_max(state->capacity,len));
    }
    
    len = gb_min(len,state->capacity - state->length);

    memcpy(state->data + state->length,src,len);
    
    state->length += len;
}

void gb_state_read_ex(gb_state_t* state,void* dst,size_t len){
    if(state->read + len > state->length){
        memset(dst,0x00,len);
        return;
    }
    
    memcpy(dst,state->data + state->read,len);
    
    state->read += len;
}