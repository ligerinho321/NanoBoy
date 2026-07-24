#include "mbc3.h"
#include "../gb.h"

bool gb_mbc3_init(gb_cartridge_t* cartridge,uint8_t flags){

    printf("Mapper: MBC3\n");

    gb_mbc3_t* mbc3 = (gb_mbc3_t*)malloc(sizeof(gb_mbc3_t));

    if(!mbc3){
        gb_printf_errno(malloc);
        return false;
    }

    memset(mbc3,0x00,sizeof(gb_mbc3_t));

    mbc3->has_rtc = flags & gb_cartridge_rtc;

    cartridge->mapper.data = mbc3;

    if(mbc3->has_rtc){        
        cartridge->mapper.rtc_update_timer = gb_mbc3_rtc_update_timer;
        cartridge->mapper.rtc_save = gb_mbc3_rtc_save;
        cartridge->mapper.rtc_load = gb_mbc3_rtc_load;
    }

    cartridge->mapper.reset = gb_mbc3_reset;

    gb_cartridge_set_rom0_bank(cartridge,0x00);

    cartridge->rom0_handler.write = gb_mbc3_write_register_0;
    cartridge->rom1_handler.write = gb_mbc3_write_register_1;
    
    if(flags & gb_cartridge_ram){
        if(!gb_cartridge_init_ram(cartridge,flags & gb_cartridge_battery)){
            return false;
        }
    }

    return true;
}


void gb_mbc3_update_ram_and_rtc_mapping(gb_cartridge_t* cartridge){

    gb_mbc3_t* mbc3 = (gb_mbc3_t*)cartridge->mapper.data;

    if(!mbc3->ram_and_rtc_enabled) goto unmap;

    if(mbc3->ram_and_rtc_bank <= 0x07){

        if(cartridge->ram_size > 0x00){
            
            gb_cartridge_set_ram_bank(cartridge,mbc3->ram_and_rtc_bank);

            cartridge->ram_handler.write = gb_cartridge_write_ram;
            cartridge->ram_handler.read = gb_cartridge_read_ram;
        }
        else{
            goto unmap;
        }
    }
    else if(mbc3->ram_and_rtc_bank <= 0x0C){

        if(mbc3->has_rtc){
            
            cartridge->ram_handler.write = gb_mbc3_rtc_write_register;
            cartridge->ram_handler.read = gb_mbc3_rtc_read_register;
        }
        else{
            goto unmap;
        }
    }
    else{
        goto unmap;
    }

    return;

    unmap:
    cartridge->ram_handler.write = gb_memory_write_empty;
    cartridge->ram_handler.read = gb_memory_read_empty;
}


void gb_mbc3_write_register_0(void* data,uint8_t value,uint16_t address){
    gb_cartridge_t* cartridge = (gb_cartridge_t*)data;
    gb_mbc3_t* mbc3 = (gb_mbc3_t*)cartridge->mapper.data;
    //0x0000-0x1FFF
    if(address < 0x2000){
        if(!cartridge->ram_size && !mbc3->has_rtc) return;

        mbc3->ram_and_rtc_enabled = (value & 0x0F) == 0x0A;

        gb_mbc3_update_ram_and_rtc_mapping(cartridge);
    }
    //0x2000-0x3FFF
    else{
        mbc3->rom_bank = gb_max(0x01,value);
        
        gb_cartridge_set_rom1_bank(cartridge,mbc3->rom_bank);
    }
}

void gb_mbc3_write_register_1(void* data,uint8_t value,uint16_t address){    
    gb_cartridge_t* cartridge = (gb_cartridge_t*)data;
    gb_mbc3_t* mbc3 = (gb_mbc3_t*)cartridge->mapper.data;
    //0x4000-0x5FFF
    if(address < 0x6000){
        if(!cartridge->ram_size && !mbc3->has_rtc) return;

        mbc3->ram_and_rtc_bank = value & 0x0F;

        gb_mbc3_update_ram_and_rtc_mapping(cartridge);
    }
    //0x6000-0x7FFF
    else{
        if(!mbc3->has_rtc) return;

        bool new_latch = value & 0x01;

        if(!mbc3->rtc.latch && new_latch){
            
            gb_mbc3_rtc_update_timer(cartridge);

            memcpy(mbc3->rtc.latched_reg,mbc3->rtc.reg,sizeof(mbc3->rtc.latched_reg));
        }

        mbc3->rtc.latch = new_latch;
    }
}


void gb_mbc3_rtc_write_register(void* data,uint8_t value,uint16_t address){
    
    gb_cartridge_t* cartridge = (gb_cartridge_t*)data;
    gb_mbc3_t* mbc3 = (gb_mbc3_t*)cartridge->mapper.data;
    gb_mbc3_rtc_t* rtc = &mbc3->rtc;

    gb_mbc3_rtc_update_timer(cartridge);

    switch(mbc3->ram_and_rtc_bank){
        case 0x08:
            rtc->reg[0x00] = value & 0x3F;
            rtc->cycles = 0;
            rtc->last_update_cycle = cartridge->gb->cycle;
            break;
        case 0x09:
            rtc->reg[0x01] = value & 0x3F;
            break;
        case 0x0A:
            rtc->reg[0x02] = value & 0x1F;
            break;
        case 0x0B:
            rtc->reg[0x03] = value;
            break;
        case 0x0C:
            if((rtc->reg[0x04] & 0x40) && !(value & 0x40)){
                rtc->last_update_cycle = cartridge->gb->cycle;
            }
            rtc->reg[0x04] = value & 0xC1;
            break;
    }
}

uint8_t gb_mbc3_rtc_read_register(void* data,uint16_t address){

    gb_cartridge_t* cartridge = (gb_cartridge_t*)data;
    gb_mbc3_t* mbc3 = (gb_mbc3_t*)cartridge->mapper.data;
    gb_mbc3_rtc_t* rtc = &mbc3->rtc;

    uint8_t value = 0xFF;

    switch(mbc3->ram_and_rtc_bank){
        case 0x08: value = rtc->latched_reg[0x00]; break;
        case 0x09: value = rtc->latched_reg[0x01]; break;
        case 0x0A: value = rtc->latched_reg[0x02]; break;
        case 0x0B: value = rtc->latched_reg[0x03]; break;
        case 0x0C: value = rtc->latched_reg[0x04]; break;
    }

    return value;
}


static inline void gb_mbc3_rtc_clock(gb_mbc3_rtc_t* rtc){

    rtc->reg[0x00] = (rtc->reg[0x00] + 0x01) & 0x3F;

    //Seconds
    if(rtc->reg[0x00] != 0x3C) return;

    rtc->reg[0x00] = 0x00;
    rtc->reg[0x01] = (rtc->reg[0x01] + 0x01) & 0x3F;

    //Minutes
    if(rtc->reg[0x01] != 0x3C) return;

    rtc->reg[0x01] = 0x00;
    rtc->reg[0x02] = (rtc->reg[0x02] + 0x01) & 0x1F;

    //Hours
    if(rtc->reg[0x02] != 0x18) return;

    rtc->reg[0x02] = 0x00;
    rtc->reg[0x03] = (rtc->reg[0x03] + 0x01) & 0xFF;

    //Low Days
    if(rtc->reg[0x03] > 0x00) return;

    //Hi Days
    if(rtc->reg[0x04] & 0x01){
        rtc->reg[0x04] &= ~0x01;
        rtc->reg[0x04] |= 0x80;
    }
    else{
        rtc->reg[0x04] |= 0x01;
    }
}


void gb_mbc3_rtc_update_timer(gb_cartridge_t* cartridge){
    gb_mbc3_t* mbc3 = (gb_mbc3_t*)cartridge->mapper.data;
    gb_mbc3_rtc_t* rtc = &mbc3->rtc;

    if(rtc->reg[0x04] & 0x40) return;

    uint64_t elapsed_cycles = cartridge->gb->cycle - rtc->last_update_cycle;

    uint64_t frequency = gb_get_clock_rate(cartridge->gb) / gb_mbc3_rtc_clock_rate;

    rtc->cycles += elapsed_cycles / frequency;

    while(rtc->cycles >= gb_mbc3_rtc_clock_rate){

        rtc->cycles -= gb_mbc3_rtc_clock_rate;

        gb_mbc3_rtc_clock(rtc);
    }

    rtc->last_update_cycle = cartridge->gb->cycle - (elapsed_cycles % frequency);
}


void gb_mbc3_rtc_save(gb_cartridge_t* cartridge,const char* path){
    gb_mbc3_t* mbc3 = (gb_mbc3_t*)cartridge->mapper.data;
    gb_mbc3_rtc_t* rtc = &mbc3->rtc;
    
    uint8_t data[sizeof(rtc->reg) + sizeof(time_t)];
    
    memcpy(data,rtc->reg,sizeof(rtc->reg));

    time_t current_time = time(NULL);

    memcpy(data + sizeof(rtc->reg),&current_time,sizeof(time_t));

    gb_save_file(path,data,sizeof(data));
}

void gb_mbc3_rtc_load(gb_cartridge_t* cartridge,const char* path){
    gb_mbc3_t* mbc3 = (gb_mbc3_t*)cartridge->mapper.data;
    gb_mbc3_rtc_t* rtc = &mbc3->rtc;

    uint8_t* data = NULL;
    size_t len = 0;

    if(!gb_load_file(path,(void**)&data,&len)) return;

    if(len == (sizeof(rtc->reg) + sizeof(time_t))){

        memcpy(rtc->reg,data,sizeof(rtc->reg));

        time_t last_time = 0;
        memcpy(&last_time,data + sizeof(rtc->reg),sizeof(time_t));

        time_t current_time = time(NULL);

        time_t seconds = current_time - last_time;

        while(seconds--){
            gb_mbc3_rtc_clock(rtc);
        }
    }

    free(data);
}


void gb_mbc3_reset(gb_cartridge_t* cartridge){

    gb_mbc3_t* mbc3 = (gb_mbc3_t*)cartridge->mapper.data;

    mbc3->rom_bank = 0x01;
    gb_cartridge_set_rom1_bank(cartridge,mbc3->rom_bank);

    if((cartridge->ram_size > 0x00) || mbc3->has_rtc){
        mbc3->ram_and_rtc_enabled = false;
        mbc3->ram_and_rtc_bank = 0x00;
        gb_mbc3_update_ram_and_rtc_mapping(cartridge);
    }

    if(mbc3->has_rtc){
        memset(mbc3->rtc.latched_reg,0,sizeof(mbc3->rtc.latched_reg));
        mbc3->rtc.latch = false;
        mbc3->rtc.last_update_cycle = 0;
    }
}