#include "rewind.h"
#include "gb.h"

void gb_rewind_init(gb_rewind_t* rewind,gb_t* gb){
    rewind->gb = gb;

#ifdef _WIN32
    if(!QueryPerformanceFrequency(&rewind->freq_time)){
        gb_printf_error("QueryPerformanceFrequency failed");
    }
#endif
}


void gb_rewind_push(gb_rewind_t* rewind){
    if(!rewind->enabled || !rewind->logical_capacity) return;

    gb_snapshot_t* snapshot = (gb_snapshot_t*)(rewind->data + rewind->tail * rewind->snapshot_length);

    gb_save_snapshot(rewind->gb,snapshot);

    if(++rewind->tail >= rewind->physical_capacity){
        rewind->tail = 0;
    }

    if(rewind->length < rewind->logical_capacity){
        ++rewind->length;
    }
    else{
        if(++rewind->head >= rewind->physical_capacity){
            rewind->head = 0;
        }
    }
}


void gb_rewind_execute(gb_rewind_t* rewind){
    
    if(!rewind->length) return;

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

    while(elapsed >= rewind->frame_time && rewind->length > 0){

        elapsed -= rewind->frame_time;

        rewind->tail = ((rewind->tail == 0) ? rewind->physical_capacity : rewind->tail) - 1;

        gb_snapshot_t* snapshot = (gb_snapshot_t*)(rewind->data + rewind->tail * rewind->snapshot_length);

        gb_load_snapshot(rewind->gb,snapshot);

        --rewind->length;
    }

    rewind->remaining_time = elapsed;
    rewind->last_time = current;
}


bool gb_rewind_start(gb_t* gb){
    if(!gb->cartridge_inserted) return false;

    gb_rewind_t* rewind = &gb->rewind;

    if(!rewind->enabled || rewind->rewinding || !rewind->length) return false;
    
    rewind->rewinding = true;

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

    if(!rewind->enabled){
        gb_rewind_reset(rewind);
    }
}

bool gb_rewind_get_enabled(gb_t* gb){
    return gb->rewind.enabled;
};


bool gb_rewind_set_capacity(gb_t* gb,size_t new_logical_capacity){

    gb_rewind_t* rewind = &gb->rewind;

    if(!rewind->gb->cartridge_inserted){
        rewind->logical_capacity = new_logical_capacity;
        return true;
    }

    if(new_logical_capacity <= rewind->physical_capacity){

        if(rewind->enabled && new_logical_capacity < rewind->length){

            size_t discarded = rewind->length - new_logical_capacity;

            rewind->head = (rewind->head + discarded) % rewind->physical_capacity;

            rewind->length = new_logical_capacity;
        }
    }
    else{
        uint8_t* ptr = (uint8_t*)realloc(rewind->data,new_logical_capacity * rewind->snapshot_length);

        if(!ptr){
            gb_printf_errno(realloc);
            return false;
        }

        rewind->data = ptr;

        if(rewind->enabled && ((rewind->tail < rewind->head) || (rewind->length == rewind->physical_capacity))){
            
            if(rewind->tail > 0){
                size_t delta = new_logical_capacity - rewind->physical_capacity;

                size_t n = gb_min(rewind->tail,delta);
                
                memcpy(rewind->data + rewind->physical_capacity * rewind->snapshot_length,rewind->data,n * rewind->snapshot_length);
                
                if(rewind->tail > delta){
                    size_t r = rewind->tail - delta;

                    memcpy(rewind->data,rewind->data + delta * rewind->snapshot_length,r * rewind->snapshot_length);
                }
            }

            rewind->tail = (rewind->tail + rewind->physical_capacity) % new_logical_capacity;
        }

        rewind->physical_capacity = new_logical_capacity;
    }

    rewind->logical_capacity = new_logical_capacity;

    return true;
}

size_t gb_rewind_get_capacity(gb_t* gb){
    return gb->rewind.logical_capacity;
}


void gb_rewind_set_frame_time(gb_t* gb,float frame_time){
    gb->rewind.frame_time = frame_time;
}

float gb_rewind_get_frame_time(gb_t* gb){
    return gb->rewind.frame_time;
}


bool gb_rewind_load(gb_rewind_t* rewind){

    size_t snapshot_length = sizeof(gb_snapshot_t) + rewind->gb->cartridge.ram_length + rewind->gb->cartridge.mapper.data_length;
    
    snapshot_length = gb_align_up(snapshot_length,alignof(gb_snapshot_t));

    rewind->data = (uint8_t*)malloc(rewind->logical_capacity * snapshot_length);

    if(!rewind->data){
        gb_printf_errno(malloc);
        return false;
    }

    rewind->snapshot_length = snapshot_length;

    rewind->physical_capacity = rewind->logical_capacity;

    return true;
}

void gb_rewind_unload(gb_rewind_t* rewind){
    rewind->snapshot_length = 0;
    gb_rewind_reset(rewind);
    gb_rewind_free(rewind);
}

void gb_rewind_reset(gb_rewind_t* rewind){
    rewind->length = 0;
    
    rewind->head = 0;
    rewind->tail = 0;

    rewind->rewinding = false;
}


void gb_rewind_free(gb_rewind_t* rewind){
    if(rewind->data != NULL){
        free(rewind->data);
        rewind->data = NULL;
    }
    rewind->physical_capacity = 0;
}