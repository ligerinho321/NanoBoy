#pragma once

#include "utils.h"

typedef struct _gb_t gb_t;

#define gb_savestate_header_magic "SAVE"
#define gb_screenshot_header_magic "SCRE"
#define gb_snapshot_header_magic "SNAP"

typedef struct _gb_savestate_header_t {
    char magic[4];
    uint32_t boot_rom_crc32;
    uint32_t rom_crc32;
    uint64_t timestamp;
    uint32_t state_offset;
} gb_savestate_header_t;

typedef struct _gb_screenshot_header_t {
    char magic[4];
    uint32_t compressed_size;
} gb_screenshot_header_t;

typedef struct _gb_snapshot_header_t {
    char magic[4];
    uint32_t decompressed_crc32;
    uint32_t compressed_size;
} gb_snapshot_header_t;

typedef struct _gb_savestate_info_t {
    uint32_t rom_crc32;
    uint64_t timestamp;
    const uint8_t* screenshot;
    size_t screenshot_length;
} gb_savestate_info_t;


#ifdef __cplusplus
extern "C" {
#endif

bool gb_savestate_serialize(gb_t* gb,const char* filename);
bool gb_savestate_deserialize(gb_t* gb,const char* filename);

bool gb_savestate_get_info(const char* filename,gb_savestate_info_t* info);

#ifdef __cplusplus
}
#endif