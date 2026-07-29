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
    uint8_t* thumb_cbuff = NULL;

    gb_state_t state = {0};
    gb_save_state(gb,&state);

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
    state_header.decompressed_crc32 = crc32(state.data,state.length);
    state_header.compressed_size = state_compress_result;

    size_t thumb_cbuff_size = ZSTD_compressBound(gb_screen_length);

    thumb_cbuff = (uint8_t*)malloc(thumb_cbuff_size);

    if(!thumb_cbuff){
        gb_printf_errno(malloc);
        goto fail;
    }

    size_t thumb_compress_result = ZSTD_compress(thumb_cbuff,thumb_cbuff_size,gb_get_render_buffer(gb),gb_screen_length,gb_savestate_zstd_compress_level);

    if(ZSTD_isError(thumb_compress_result)){
        gb_printf_error("ZSTD_compress failed");
        goto fail;
    }

    gb_thumbnail_header_t thumb_header = {0};
    memcpy(thumb_header.magic,gb_thumbnail_header_magic,4);
    thumb_header.compressed_size = thumb_compress_result;

    gb_savestate_header_t header = {0};
    memcpy(header.magic,gb_savestate_header_magic,4);
    header.rom_crc32 = gb->cartridge.rom_crc32;
    header.timestamp = time(NULL);
    header.state_offset = sizeof(gb_savestate_header_t) + sizeof(gb_thumbnail_header_t) + thumb_header.compressed_size;

    fwrite(&header,sizeof(gb_savestate_header_t),1,file);
    
    fwrite(&thumb_header,sizeof(gb_thumbnail_header_t),1,file);
    fwrite(thumb_cbuff,1,thumb_header.compressed_size,file);
    
    fwrite(&state_header,sizeof(gb_state_header_t),1,file);
    fwrite(state_cbuff,1,state_header.compressed_size,file);

    free(state.data);
    free(state_cbuff);
    free(thumb_cbuff);
    
    fclose(file);
    
    return true;

    fail:
    if(state.data != NULL) free(state.data);
    if(state_cbuff != NULL) free(state_cbuff);
    if(thumb_cbuff != NULL) free(thumb_cbuff);
    
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

    if(memcmp(header.magic,"SAST",4)) goto fail;
    
    if(header.rom_crc32 != gb->cartridge.rom_crc32) goto fail;

    if(size < header.state_offset + sizeof(gb_state_header_t)) goto fail;

    fseek(file,header.state_offset,SEEK_SET);

    gb_state_header_t state_header = {0};

    fread(&state_header,sizeof(gb_state_header_t),1,file);

    if(memcmp(state_header.magic,"STAT",4)) goto fail;

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

    if(crc32(state.data,state.length) != state_header.decompressed_crc32) goto fail;

    gb_load_state(gb,&state);

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


bool gb_savestate_thread_safe_serialize(gb_t* gb,const char* filename){
    gb_thread_stop(gb);
    bool result = gb_savestate_serialize(gb,filename);
    gb_thread_start(gb);
    return result;
}

bool gb_savestate_thread_safe_deserialize(gb_t* gb,const char* filename){
    gb_thread_stop(gb);
    bool result = gb_savestate_deserialize(gb,filename);
    gb_thread_start(gb);
    return result;
}


bool gb_savestate_get_info(const char* filename,gb_savestate_info_t* info){
    
    static char thumb_dbuff[gb_screen_length] = {0};

    FILE* file = fopen(filename,"rb");

    if(!file){
        gb_printf_errno(fopen);
        return false;
    }

    fseek(file,0,SEEK_END);
    size_t size = ftell(file);
    fseek(file,0,SEEK_SET);

    uint8_t* thumb_cbuff = NULL;

    if(size < sizeof(gb_savestate_header_t) + sizeof(gb_thumbnail_header_t)) goto fail;

    gb_savestate_header_t header = {0};

    fread(&header,sizeof(gb_savestate_header_t),1,file);

    gb_thumbnail_header_t thumb_header = {0};

    fread(&thumb_header,sizeof(gb_thumbnail_header_t),1,file);

    if(size < ftell(file) + thumb_header.compressed_size) goto fail;

    thumb_cbuff = (uint8_t*)malloc(thumb_header.compressed_size);

    if(!thumb_cbuff){
        gb_printf_errno(malloc);
        goto fail;
    }

    fread(thumb_cbuff,1,thumb_header.compressed_size,file);

    size_t thumb_decompress_size = ZSTD_getFrameContentSize(thumb_cbuff,thumb_header.compressed_size);

    if(thumb_decompress_size != sizeof(thumb_dbuff)) goto fail;

    size_t thumb_decompress_result = ZSTD_decompress(thumb_dbuff,sizeof(thumb_dbuff),thumb_cbuff,thumb_header.compressed_size);

    if(thumb_decompress_result != sizeof(thumb_dbuff)) goto fail;

    info->timestamp = header.timestamp;
    info->thumbnail = thumb_dbuff;

    free(thumb_cbuff);
    
    fclose(file);

    return true;

    fail:
    if(thumb_cbuff != NULL) free(thumb_cbuff);

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