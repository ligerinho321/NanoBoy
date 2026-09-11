#pragma once

#include "utils.h"

typedef struct _gb_rewind_t {
    gb_t* gb;

    uint8_t* data;
    
    size_t snapshot_length;

    size_t length;

    size_t logical_capacity;
    size_t physical_capacity;

    size_t head;
    size_t tail;

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

bool gb_rewind_set_capacity(gb_t* gb,size_t new_logical_capacity);
size_t gb_rewind_get_capacity(gb_t* gb);

void gb_rewind_set_frame_time(gb_t* gb,float frame_time);
float gb_rewind_get_frame_time(gb_t* gb);

bool gb_rewind_load(gb_rewind_t* rewind);
void gb_rewind_unload(gb_rewind_t* rewind);
void gb_rewind_reset(gb_rewind_t* rewind);

void gb_rewind_free(gb_rewind_t* rewind);

#ifdef __cplusplus
}
#endif