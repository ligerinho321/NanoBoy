#pragma once

#include "utils.h"

typedef struct _gb_rewind_buffer_t {
    uint8_t* data;
    uint32_t length;
    uint32_t capacity;
} gb_rewind_buffer_t;

typedef struct _gb_rewind_entry_t {
    uint32_t offset;
    uint32_t length;
} gb_rewind_entry_t;

typedef struct _gb_rewind_entries_t {
    gb_rewind_entry_t* data;

    uint32_t length;

    uint32_t head;
    uint32_t tail;

    bool pending_logical_capacity;
    uint32_t new_logical_capacity;

    uint32_t logical_capacity;
    uint32_t physical_capacity;
} gb_rewind_entries_t;

typedef struct _gb_rewind_deltas_t {
    uint8_t* data;

    uint32_t length;
    
    uint32_t head;
    uint32_t tail;

    uint32_t capacity;
} gb_rewind_deltas_t;

typedef struct _gb_rewind_t {
    gb_t* gb;

    uint32_t snapshot_length;

    uint8_t* last_snapshot;
    uint8_t* current_snapshot;
    uint8_t* delta;

    gb_rewind_buffer_t compression;

    gb_rewind_entries_t entries;
    
    gb_rewind_deltas_t deltas;

    bool enabled;
    bool rewinding;

    float frame_time;
    float remaining_time;
    
#ifdef _WIN32
    LARGE_INTEGER freq_time;
    LARGE_INTEGER last_time;
#else
    struct timespec last_time;
#endif
} gb_rewind_t;

#ifdef __cplusplus
extern "C" {
#endif

void gb_rewind_init(gb_rewind_t* rewind,gb_t* gb);

void gb_rewind_push(gb_rewind_t* rewind);

void gb_rewind_execute(gb_rewind_t* rewind);

bool gb_rewind_start(gb_t* gb);
bool gb_rewind_end(gb_t* gb);

void gb_rewind_set_enabled(gb_t* gb,bool enabled);
bool gb_rewind_get_enabled(gb_t* gb);

void gb_rewind_set_capacity(gb_t* gb,uint32_t new_logical_capacity);
uint32_t gb_rewind_get_capacity(gb_t* gb);

void gb_rewind_set_frame_time(gb_t* gb,float frame_time);
float gb_rewind_get_frame_time(gb_t* gb);

bool gb_rewind_load(gb_rewind_t* rewind);
void gb_rewind_unload(gb_rewind_t* rewind);

void gb_rewind_reset(gb_rewind_t* rewind);

#ifdef __cplusplus
}
#endif