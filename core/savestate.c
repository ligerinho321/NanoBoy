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

    uint8_t* snapshot_cbuff = NULL;
    uint8_t* screen_cbuff = NULL;

    size_t snapshot_length = sizeof(gb_snapshot_t) + gb->cartridge.ram_length + gb->cartridge.mapper.data_length;

    gb_snapshot_t* snapshot = (gb_snapshot_t*)malloc(snapshot_length);
    
    if(!snapshot){
        gb_printf_errno(malloc);
        goto fail;
    }

    gb_save_snapshot(gb,snapshot);

    size_t snapshot_cbuff_size = ZSTD_compressBound(snapshot_length);
    
    snapshot_cbuff = (uint8_t*)malloc(snapshot_cbuff_size);

    if(!snapshot_cbuff){
        gb_printf_errno(malloc);
        goto fail;
    }
    
    size_t snapshot_compress_result = ZSTD_compress(snapshot_cbuff,snapshot_cbuff_size,snapshot,snapshot_length,gb_savestate_zstd_compress_level);

    if(ZSTD_isError(snapshot_compress_result)){
        gb_printf_error("ZSTD_compress failed");
        goto fail;
    }

    gb_snapshot_header_t snapshot_header = {0};
    memcpy(snapshot_header.magic,gb_snapshot_header_magic,4);
    snapshot_header.decompressed_crc32 = gb_crc32((uint8_t*)snapshot,snapshot_length);
    snapshot_header.compressed_size = snapshot_compress_result;

    size_t screen_cbuff_size = ZSTD_compressBound(gb_screen_length);

    screen_cbuff = (uint8_t*)malloc(screen_cbuff_size);

    if(!screen_cbuff){
        gb_printf_errno(malloc);
        goto fail;
    }

    size_t screen_compress_result = ZSTD_compress(screen_cbuff,screen_cbuff_size,gb_ppu_get_render_buffer(gb),gb_screen_length,gb_savestate_zstd_compress_level);

    if(ZSTD_isError(screen_compress_result)){
        gb_printf_error("ZSTD_compress failed");
        goto fail;
    }

    gb_screenshot_header_t screen_header = {0};
    memcpy(screen_header.magic,gb_screenshot_header_magic,4);
    screen_header.compressed_size = screen_compress_result;

    gb_savestate_header_t header = {0};
    
    memcpy(header.magic,gb_savestate_header_magic,4);

    header.boot_rom_crc32 = gb->state.is_cgb ? gb->boot.cgb_rom_crc32 : gb->boot.dmg_rom_crc32;
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

    fwrite(&snapshot_header,sizeof(gb_snapshot_header_t),1,file);
    fwrite(snapshot_cbuff,1,snapshot_header.compressed_size,file);

    free(snapshot);
    free(snapshot_cbuff);
    free(screen_cbuff);
    
    fclose(file);
    
    return true;

    fail:
    if(snapshot != NULL) free(snapshot);
    if(snapshot_cbuff != NULL) free(snapshot_cbuff);
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

    uint8_t* snapshot_cbuff = NULL;
    uint8_t* snapshot_dbuff = NULL;

    if(size < sizeof(gb_savestate_header_t)) goto fail;


    gb_savestate_header_t header = {0};

    fread(&header,sizeof(gb_savestate_header_t),1,file);

    if(memcmp(header.magic,gb_savestate_header_magic,4)) goto fail;

    if(header.rom_crc32 != gb->cartridge.rom_crc32) goto fail;

    if(size < header.state_offset + sizeof(gb_snapshot_header_t)) goto fail;

    fseek(file,header.state_offset,SEEK_SET);
    

    gb_snapshot_header_t snapshot_header = {0};

    fread(&snapshot_header,sizeof(gb_snapshot_header_t),1,file);

    if(memcmp(snapshot_header.magic,gb_snapshot_header_magic,4)) goto fail;

    if(size < header.state_offset + sizeof(gb_snapshot_header_t) + snapshot_header.compressed_size) goto fail;

    snapshot_cbuff = (uint8_t*)malloc(snapshot_header.compressed_size);

    if(!snapshot_cbuff){
        gb_printf_errno(malloc);
        goto fail;
    }

    fread(snapshot_cbuff,1,snapshot_header.compressed_size,file);

    size_t dsize = ZSTD_getFrameContentSize(snapshot_cbuff,snapshot_header.compressed_size);

    if(dsize == ZSTD_CONTENTSIZE_UNKNOWN || dsize == ZSTD_CONTENTSIZE_ERROR){
        gb_printf_error("ZSTD_getFrameContentSize failed");
        goto fail;
    }

    snapshot_dbuff = (uint8_t*)malloc(dsize);

    if(!snapshot_dbuff){
        gb_printf_errno(malloc);
        goto fail;
    }

    size_t snapshot_decompress_result = ZSTD_decompress(snapshot_dbuff,dsize,snapshot_cbuff,snapshot_header.compressed_size);

    if(ZSTD_isError(snapshot_decompress_result)){
        gb_printf_error("ZSTD_decompress failed");
        goto fail;
    }

    size_t snapshot_length = sizeof(gb_snapshot_t) + gb->cartridge.ram_length + gb->cartridge.mapper.data_length;

    if(snapshot_length != snapshot_decompress_result) goto fail;

    if(gb_crc32(snapshot_dbuff,snapshot_decompress_result) != snapshot_header.decompressed_crc32) goto fail;

    gb_snapshot_t* snapshot = (gb_snapshot_t*)snapshot_dbuff;

    if(snapshot->boot.mapped){

        gb_boot_update_roms(&gb->boot);

        if(snapshot->gb.is_cgb){
            if(!gb->boot.cgb_rom_inserted || gb->boot.cgb_rom_crc32 != header.boot_rom_crc32) goto fail;
        }
        else{
            if(!gb->boot.dmg_rom_inserted || gb->boot.dmg_rom_crc32 != header.boot_rom_crc32) goto fail;
        }
    }

    gb_load_snapshot(gb,snapshot);

    gb_rewind_reset(&gb->rewind);

    gb_breakpoint_manager_reset(&gb->breakpoint_manager);
    gb_event_manager_reset(&gb->event_manager);
    
    gb_update_mapping(gb);

    free(snapshot_cbuff);
    free(snapshot_dbuff);
    
    fclose(file);

    return true;

    fail:
    if(snapshot_cbuff != NULL) free(snapshot_cbuff);
    if(snapshot_dbuff != NULL) free(snapshot_dbuff);
    
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
