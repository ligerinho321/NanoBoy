#include "breakpoint_manager.h"
#include "../gb.h"

void gb_breakpoint_manager_init(gb_breakpoint_manager_t* breakpoint_manager,gb_t* gb){
    breakpoint_manager->gb = gb;
}


bool gb_breakpoint_manager_check(gb_breakpoint_manager_t* breakpoint_manager,uint16_t address){

    if(breakpoint_manager->last_check_address == address){
        return false;
    }

    breakpoint_manager->last_check_address = address;

    if(!breakpoint_manager->enabled || !breakpoint_manager->breakpoints) return false;

    gb_breakpoint_t* breakpoint = breakpoint_manager->breakpoints;
    
    do{
        if(breakpoint->enabled){
            if(breakpoint->memory_type == gb_memory_cpu_type){

                if(address == breakpoint->address){

                    gb_pause(breakpoint_manager->gb,true);

                    return true;
                }
            }
            else{
                uint8_t memory_type = gb_memory_type(breakpoint_manager->gb,address);
                
                if(breakpoint->memory_type == memory_type){
                    
                    size_t absolute_address = gb_memory_type_absolute_address(breakpoint_manager->gb,memory_type,address);

                    if(absolute_address == breakpoint->address){
                        
                        gb_pause(breakpoint_manager->gb,true);

                        return true;
                    }
                }
            }
        }
        
        breakpoint = breakpoint->next;

    }while(breakpoint != NULL);

    return false;
}


void gb_breakpoint_manager_enable(gb_t* gb,bool enabled){
    gb->breakpoint_manager.enabled = enabled;
}


void gb_breakpoint_manager_add(gb_t* gb,gb_breakpoint_t* breakpoint){
    gb_list_add_element(gb->breakpoint_manager.breakpoints,breakpoint,gb_breakpoint_t);
}

void gb_breakpoint_manager_remove(gb_t* gb,gb_breakpoint_t* breakpoint){
    gb_list_remove_element(gb->breakpoint_manager.breakpoints,breakpoint,gb_breakpoint_t);
}

void gb_breakpoint_manager_clear(gb_t* gb){
    gb->breakpoint_manager.breakpoints = NULL;
}
