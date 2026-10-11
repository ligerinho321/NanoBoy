#include "rewind.h"
#include "gb.h"

void gb_rewind_init(gb_rewind_t* rewind,gb_t* gb){
    rewind->gb = gb;

    rewind->recording_interval = 1;

#ifdef _WIN32
    if(!QueryPerformanceFrequency(&rewind->freq_time)){
        gb_printf_error("QueryPerformanceFrequency failed");
    }
#endif
}


static bool gb_rewind_deltas_reserve(gb_rewind_t* rewind){

    gb_rewind_deltas_t* deltas = &rewind->deltas;

    if(deltas->length + rewind->compression.length <= deltas->capacity) return true;

    uint32_t increase = gb_max(rewind->compression.length,deltas->capacity);
    uint32_t new_capacity = deltas->capacity + increase;

    uint8_t* new_data = (uint8_t*)realloc(deltas->data,new_capacity);

    if(!new_data){
        gb_printf_errno(realloc);
        return false;
    }

    if((deltas->tail < deltas->head) || (deltas->length == deltas->capacity)){
        
        if(deltas->tail > 0){
            uint32_t n = gb_min(deltas->tail,increase);

            memcpy(new_data + deltas->capacity,new_data,n);

            if(deltas->tail > increase){
                uint32_t r = deltas->tail - increase;

                memcpy(new_data,new_data + increase,r);
            }
        }
        
        deltas->tail += deltas->capacity;

        if(deltas->tail >= new_capacity){
            deltas->tail -= new_capacity;
        }

        if(rewind->entries.length > 1){
            
            gb_rewind_entries_t* entries = &rewind->entries;

            gb_rewind_entry_t* head = entries->data + entries->head;

            for(uint32_t i = 1; i < entries->length; ++i){
                
                uint32_t index = (entries->head + i) % entries->physical_capacity;

                gb_rewind_entry_t* entry = entries->data + index;

                if(entry->offset < head->offset){
                    
                    entry->offset += deltas->capacity;

                    if(entry->offset >= new_capacity){
                        entry->offset -= new_capacity;
                    }

                }
            }
        }
    }

    deltas->data = new_data;
    deltas->capacity = new_capacity;

    return true;
}


static void gb_rewind_write_delta(gb_rewind_t* rewind){

    uint8_t* current = rewind->current_snapshot;
    uint8_t* last = rewind->last_snapshot;

    uint8_t* src = rewind->delta;
    uint8_t* end = rewind->delta + rewind->snapshot_length;

    while(src < end) *src++ = *current++ ^ *last++;

    size_t result = ZSTD_compress(rewind->compression.data,rewind->compression.capacity,rewind->delta,rewind->snapshot_length,ZSTD_CLEVEL_DEFAULT);

    if(ZSTD_isError(result)){
        gb_printf_error("ZSTD_compress failed");
        return;
    }

    rewind->compression.length = result;
}

static void gb_rewind_read_delta(gb_rewind_t* rewind){

    size_t result = ZSTD_decompress(rewind->delta,rewind->snapshot_length,rewind->compression.data,rewind->compression.length);

    if(ZSTD_isError(result)){
        gb_printf_error("ZSTD_decompress failed");
        return;
    }

    uint8_t* dst = rewind->last_snapshot;
    uint8_t* src = rewind->delta;
    uint8_t* end = rewind->delta + rewind->snapshot_length;

    while(src < end) *dst++ ^= *src++;
}


static void gb_rewind_pop_delta(gb_rewind_t* rewind){
    gb_rewind_deltas_t* deltas = &rewind->deltas;

    gb_rewind_entry_t* entry = rewind->entries.data + rewind->entries.tail;

    if(entry->offset + entry->length > deltas->capacity){
        uint32_t first_len = deltas->capacity - entry->offset;
        memcpy(rewind->compression.data,deltas->data + entry->offset,first_len);

        uint32_t second_len = entry->length - first_len;
        memcpy(rewind->compression.data + first_len,deltas->data,second_len);
    }
    else{
        memcpy(rewind->compression.data,deltas->data + entry->offset,entry->length);
    }

    rewind->compression.length = entry->length;

    deltas->tail = entry->offset;
    deltas->length -= entry->length;
}

static void gb_rewind_push_delta(gb_rewind_t* rewind){

    if(!gb_rewind_deltas_reserve(rewind)) return;

    gb_rewind_deltas_t* deltas = &rewind->deltas;
    gb_rewind_buffer_t* compression = &rewind->compression;

    gb_rewind_entry_t* entry = rewind->entries.data + rewind->entries.tail;
    entry->offset = deltas->tail;
    entry->length = compression->length;

    if(deltas->tail + compression->length > deltas->capacity){
        
        uint32_t first_len = deltas->capacity - deltas->tail;
        memcpy(deltas->data + deltas->tail,compression->data,first_len);

        uint32_t second_len = compression->length - first_len;
        memcpy(deltas->data,compression->data + first_len,second_len);
    }
    else{
        memcpy(deltas->data + deltas->tail,compression->data,compression->length);
    }

    deltas->tail += compression->length;

    if(deltas->tail >= deltas->capacity){
        deltas->tail -= deltas->capacity;
    }

    deltas->length += compression->length;
}


void gb_rewind_push(gb_rewind_t* rewind){
    
    if(!rewind->enabled || !rewind->entries.logical_capacity) return;

    if(++rewind->recording_interval_count < rewind->recording_interval) return;

    rewind->recording_interval_count = 0;

    gb_rewind_entries_t* entries = &rewind->entries;

    if(entries->length == entries->logical_capacity){

        if(rewind->deltas.length > 0){

            gb_rewind_entry_t* head = entries->data + entries->head;

            rewind->deltas.head += head->length;
            
            if(rewind->deltas.head >= rewind->deltas.capacity){
                rewind->deltas.head -= rewind->deltas.capacity;
            }

            rewind->deltas.length -= head->length;
        }

        if(++entries->head >= entries->physical_capacity){
            entries->head = 0;
        }

        --entries->length;
    }

    gb_save_snapshot(rewind->gb,(gb_snapshot_t*)rewind->current_snapshot);

    gb_rewind_write_delta(rewind);

    gb_rewind_push_delta(rewind);

    uint8_t* temp = rewind->current_snapshot;
    rewind->current_snapshot = rewind->last_snapshot;
    rewind->last_snapshot = temp;

    if(++entries->tail >= entries->physical_capacity){
        entries->tail = 0;
    }

    ++entries->length;
}


void gb_rewind_execute(gb_rewind_t* rewind){
    
    if(!rewind->entries.length) return;

#ifdef _WIN32
    LARGE_INTEGER current = {0};

    if(!QueryPerformanceCounter(&current)){
        gb_printf_error("QueryPerformanceCounter failed");
    }

    float elapsed = (float)(current.QuadPart - rewind->last_time.QuadPart) / (float)rewind->freq_time.QuadPart;
#else
    struct timespec current = {0};
        
    if(clock_gettime(CLOCK_MONOTONIC,&current) < 0){
        gb_printf_errno(clock_gettime);
    }

    float elapsed = (current.tv_sec - rewind->last_time.tv_sec) + ((current.tv_nsec - rewind->last_time.tv_nsec) / 1e+9);

#endif

    elapsed += rewind->remaining_time;

    gb_rewind_entries_t* entries = &rewind->entries;

    while(elapsed >= rewind->frame_time && entries->length > 0){

        elapsed -= rewind->frame_time;

        gb_load_snapshot(rewind->gb,(gb_snapshot_t*)rewind->last_snapshot);

        entries->tail = ((entries->tail == 0) ? entries->physical_capacity : entries->tail) - 1;

        if(rewind->deltas.length > 0){
            gb_rewind_pop_delta(rewind);
            
            gb_rewind_read_delta(rewind);
        }

        --entries->length;
    }

    rewind->remaining_time = elapsed;
    rewind->last_time = current;
}


bool gb_rewind_start(gb_t* gb){
    if(!gb->cartridge_inserted) return false;

    gb_rewind_t* rewind = &gb->rewind;

    if(!rewind->enabled || rewind->rewinding || !rewind->entries.length) return false;
    
    rewind->rewinding = true;

    rewind->recording_interval_count = 0;

    rewind->remaining_time = 0.0f;

#ifdef _WIN32
    if(!QueryPerformanceCounter(&rewind->last_time)){
        gb_printf_error("QueryPerformanceCounter failed");
    }
#else
    if(clock_gettime(CLOCK_MONOTONIC,&rewind->last_time) < 0){
        gb_printf_errno(clock_gettime);
    }
#endif

    gb_breakpoint_manager_reset(&gb->breakpoint_manager);

    gb_event_manager_reset(&gb->event_manager);

    if(!gb->paused){
        gb_frame_timer_stop(&gb->frame_timer);
    }

    return true;
}

bool gb_rewind_end(gb_t* gb){
    if(!gb->cartridge_inserted) return false;

    gb_rewind_t* rewind = &gb->rewind;

    if(!rewind->rewinding) return false;

    rewind->rewinding = false;
    
    gb_update_mapping(gb);

    if(!gb->paused){
        gb_frame_timer_start(&gb->frame_timer);
    }

    return true;
}


void gb_rewind_set_enabled(gb_t* gb,bool enabled){
    gb_rewind_t* rewind = &gb->rewind;

    if(rewind->enabled == enabled) return;

    rewind->enabled = enabled;

    if(gb->cartridge_inserted){
        if(rewind->enabled){
            gb_rewind_load(rewind);
            gb_rewind_reset(rewind);
        }
        else{
            gb_rewind_unload(rewind);
        }
    }
}

bool gb_rewind_get_enabled(gb_t* gb){
    return gb->rewind.enabled;
};


void gb_rewind_set_capacity(gb_t* gb,uint32_t new_logical_capacity){

    gb_rewind_t* rewind = &gb->rewind;

    if(!gb->cartridge_inserted){
        rewind->entries.pending_logical_capacity = false;
        rewind->entries.logical_capacity = new_logical_capacity;
    }
    else{
        rewind->entries.pending_logical_capacity = true;
        rewind->entries.new_logical_capacity = new_logical_capacity;
    }
}

uint32_t gb_rewind_get_capacity(gb_t* gb){
    return gb->rewind.entries.logical_capacity;
}


void gb_rewind_set_recording_interval(gb_t* gb,uint32_t interval){
    if(interval){
        gb->rewind.recording_interval = interval;
    }
    else{
        gb->rewind.recording_interval = 1;
    }
}

uint32_t gb_rewind_get_recording_interval(gb_t* gb){
    return gb->rewind.recording_interval;
}


void gb_rewind_set_frame_time(gb_t* gb,float frame_time){
    gb->rewind.frame_time = frame_time;
}

float gb_rewind_get_frame_time(gb_t* gb){
    return gb->rewind.frame_time;
}


bool gb_rewind_load(gb_rewind_t* rewind){

    if(!rewind->enabled) return true;

    rewind->snapshot_length = sizeof(gb_snapshot_t) + rewind->gb->cartridge.ram_length + rewind->gb->cartridge.mapper.data_length;

    rewind->last_snapshot = (uint8_t*)malloc(rewind->snapshot_length);

    rewind->current_snapshot = (uint8_t*)malloc(rewind->snapshot_length);

    rewind->delta = (uint8_t*)malloc(rewind->snapshot_length);


    rewind->compression.capacity = ZSTD_compressBound(rewind->snapshot_length);
    rewind->compression.data = (uint8_t*)malloc(rewind->compression.capacity);


    if(rewind->entries.pending_logical_capacity){
        rewind->entries.pending_logical_capacity = false;
        rewind->entries.logical_capacity = rewind->entries.new_logical_capacity;
    }

    rewind->entries.data = (gb_rewind_entry_t*)malloc(sizeof(gb_rewind_entry_t) * rewind->entries.logical_capacity);
    
    rewind->entries.physical_capacity = rewind->entries.logical_capacity;


    if(!rewind->last_snapshot || !rewind->current_snapshot || !rewind->delta || !rewind->compression.data || !rewind->entries.data){
        gb_printf_errno(malloc);
        return false;
    }

    return true;
}

void gb_rewind_unload(gb_rewind_t* rewind){

    if(rewind->last_snapshot != NULL){
        free(rewind->last_snapshot);
        rewind->last_snapshot = NULL;
    }
    if(rewind->current_snapshot != NULL){
        free(rewind->current_snapshot);
        rewind->current_snapshot = NULL;
    }
    if(rewind->delta != NULL){
        free(rewind->delta);
        rewind->delta = NULL;
    }
    rewind->snapshot_length = 0;


    if(rewind->compression.data != NULL){
        free(rewind->compression.data);
        rewind->compression.data = NULL;
    }
    rewind->compression.capacity = 0;


    if(rewind->entries.data != NULL){
        free(rewind->entries.data);
        rewind->entries.data = NULL;
    }
    rewind->entries.physical_capacity = 0;


    if(rewind->deltas.data != NULL){
        free(rewind->deltas.data);
        rewind->deltas.data = NULL;
    }
    rewind->deltas.capacity = 0;
}


void gb_rewind_reset(gb_rewind_t* rewind){
    
    if(!rewind->enabled) return;

    rewind->entries.length = 0;
    rewind->entries.head = 0;
    rewind->entries.tail = 0;

    rewind->deltas.length = 0;
    rewind->deltas.head = 0;
    rewind->deltas.tail = 0;

    gb_save_snapshot(rewind->gb,(gb_snapshot_t*)rewind->last_snapshot);

    rewind->rewinding = false;

    if(rewind->entries.pending_logical_capacity){

        if(rewind->entries.new_logical_capacity > rewind->entries.physical_capacity){

            void* ptr = realloc(rewind->entries.data,sizeof(gb_rewind_entry_t) * rewind->entries.new_logical_capacity);

            if(!ptr){
                gb_printf_errno(realloc);
                return;
            }

            rewind->entries.data = (gb_rewind_entry_t*)ptr;
            rewind->entries.physical_capacity = rewind->entries.new_logical_capacity;
        }

        rewind->entries.pending_logical_capacity = false;
        rewind->entries.logical_capacity = rewind->entries.new_logical_capacity;
    }
}