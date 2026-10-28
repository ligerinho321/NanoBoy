#include "dma.h"
#include "gba.h"

void gba_dma_init(gba_dma_t* dma,gba_t* gba){
    dma->gba = gba;
}


static inline void gba_dma_write_control_lsb(gba_dma_channel_t* channel,uint8_t value){
    gba_dma_control_t* control = &channel->control;

    control->dst = (value >> 0x05) & 0x03;
    control->src = (control->src & 0x02) | ((value >> 0x07) & 0x01);
}

static inline void gba_dma_write_control_msb(gba_dma_channel_t* channel,uint8_t value){
    gba_dma_control_t* control = &channel->control;

    control->src = ((value & 0x01) << 0x01) | (control->src & 0x01);
    control->repeat = (value & 0x02) != 0x00;
    control->word = (value & 0x04) != 0x00;
    control->drq = (value & 0x08) != 0x00;
    control->start_timing = (value >> 0x04) & 0x03;
    control->irq_enabled = (value & 0x40) != 0x00;
    control->enabled = (value & 0x80) != 0x00;
}


static inline uint8_t gba_dma_read_control_lsb(gba_dma_channel_t* channel){
    gba_dma_control_t* control = &channel->control;
    return (
        ((control->src & 0x01) << 0x07) |
        (control->dst << 0x05)
    );
}

static inline uint8_t gba_dma_read_control_msb(gba_dma_channel_t* channel){
    gba_dma_control_t* control = &channel->control;
    return (
        (control->enabled << 0x07) |
        (control->irq_enabled << 0x06) |
        (control->start_timing << 0x04) |
        (control->drq << 0x03) |
        (control->word << 0x02) |
        (control->repeat << 0x01) |
        ((control->src & 0x02) >> 0x01)
    );
}


void gba_dma_write_register_raw(gba_dma_t* dma,uint8_t value,uint32_t address){

    switch(address){
        // DMA0 Src Address
        case 0x040000B0: dma->channel[0].src_address.b[0] = value; break;
        case 0x040000B1: dma->channel[0].src_address.b[1] = value; break;
        case 0x040000B2: dma->channel[0].src_address.b[2] = value; break;
        case 0x040000B3: dma->channel[0].src_address.b[3] = value & 0x07; break;
        // DMA0 Dst Address
        case 0x040000B4: dma->channel[0].dst_address.b[0] = value; break;
        case 0x040000B5: dma->channel[0].dst_address.b[1] = value; break;
        case 0x040000B6: dma->channel[0].dst_address.b[2] = value; break;
        case 0x040000B7: dma->channel[0].dst_address.b[3] = value & 0x07; break;
        // DMA0 Count
        case 0x040000B8: dma->channel[0].count.b[0] = value; break;
        case 0x040000B9: dma->channel[0].count.b[1] = value & 0x3F; break;
        // DMA0 Control
        case 0x040000BA: gba_dma_write_control_lsb(dma->channel + 0,value); break;
        case 0x040000BB: gba_dma_write_control_msb(dma->channel + 0,value & ~0x08); break;

        // DMA1 Src Address
        case 0x040000BC: dma->channel[1].src_address.b[0] = value; break;
        case 0x040000BD: dma->channel[1].src_address.b[1] = value; break;
        case 0x040000BE: dma->channel[1].src_address.b[2] = value; break;
        case 0x040000BF: dma->channel[1].src_address.b[3] = value & 0x0F; break;
        // DMA1 Dst Address
        case 0x040000C0: dma->channel[1].dst_address.b[0] = value; break;
        case 0x040000C1: dma->channel[1].dst_address.b[1] = value; break;
        case 0x040000C2: dma->channel[1].dst_address.b[2] = value; break;
        case 0x040000C3: dma->channel[1].dst_address.b[3] = value & 0x07; break;
        // DMA1 Count
        case 0x040000C4: dma->channel[1].count.b[0] = value; break;
        case 0x040000C5: dma->channel[1].count.b[1] = value & 0x3F; break;
        // DMA1 Control
        case 0x040000C6: gba_dma_write_control_lsb(dma->channel + 1,value); break;
        case 0x040000C7: gba_dma_write_control_msb(dma->channel + 1,value & ~0x08); break;

        // DMA2 Src Address
        case 0x040000C8: dma->channel[2].src_address.b[0] = value; break;
        case 0x040000C9: dma->channel[2].src_address.b[1] = value; break;
        case 0x040000CA: dma->channel[2].src_address.b[2] = value; break;
        case 0x040000CB: dma->channel[2].src_address.b[3] = value & 0x0F; break;
        // DMA2 Dst Address
        case 0x040000CC: dma->channel[2].dst_address.b[0] = value; break;
        case 0x040000CD: dma->channel[2].dst_address.b[1] = value; break;
        case 0x040000CE: dma->channel[2].dst_address.b[2] = value; break;
        case 0x040000CF: dma->channel[2].dst_address.b[3] = value & 0x07; break;
        // DMA2 Count
        case 0x040000D0: dma->channel[2].count.b[0] = value; break;
        case 0x040000D1: dma->channel[2].count.b[1] = value & 0x3F; break;
        // DMA2 Control
        case 0x040000D2: gba_dma_write_control_lsb(dma->channel + 2,value); break;
        case 0x040000D3: gba_dma_write_control_msb(dma->channel + 2,value & ~0x08); break;

        // DMA3 Src Address
        case 0x040000D4: dma->channel[3].src_address.b[0] = value; break;
        case 0x040000D5: dma->channel[3].src_address.b[1] = value; break;
        case 0x040000D6: dma->channel[3].src_address.b[2] = value; break;
        case 0x040000D7: dma->channel[3].src_address.b[3] = value & 0x0F; break;
        // DMA3 Dst Address
        case 0x040000D8: dma->channel[3].dst_address.b[0] = value; break;
        case 0x040000D9: dma->channel[3].dst_address.b[1] = value; break;
        case 0x040000DA: dma->channel[3].dst_address.b[2] = value; break;
        case 0x040000DB: dma->channel[3].dst_address.b[3] = value & 0x0F; break;
        // DMA3 Count
        case 0x040000DC: dma->channel[3].count.b[0] = value; break;
        case 0x040000DD: dma->channel[3].count.b[1] = value; break;
        // DMA3 Control
        case 0x040000DE: gba_dma_write_control_lsb(dma->channel + 3,value); break;
        case 0x040000DF: gba_dma_write_control_msb(dma->channel + 3,value); break;
    }
}

uint8_t gba_dma_read_register_raw(gba_dma_t* dma,uint32_t address){

    uint8_t value = dma->gba->memory.bus.b[address & 0x03];

    switch(address){
        // DMA0 Count
        case 0x040000B8: value = 0x00; break;
        case 0x040000B9: value = 0x00; break;
        // DMA0 Control
        case 0x040000BA: value = gba_dma_read_control_lsb(dma->channel + 0); break;
        case 0x040000BB: value = gba_dma_read_control_msb(dma->channel + 0); break;

        // DMA1 Count
        case 0x040000C4: value = 0x00; break;
        case 0x040000C5: value = 0x00; break;
        // DMA1 Control
        case 0x040000C6: value = gba_dma_read_control_lsb(dma->channel + 1); break;
        case 0x040000C7: value = gba_dma_read_control_msb(dma->channel + 1); break;

        // DMA2 Count
        case 0x040000D0: value = 0x00; break;
        case 0x040000D1: value = 0x00; break;
        // DMA2 Control
        case 0x040000D2: value = gba_dma_read_control_lsb(dma->channel + 2); break;
        case 0x040000D3: value = gba_dma_read_control_msb(dma->channel + 2); break;

        // DMA3 Count
        case 0x040000DC: value = 0x00; break;
        case 0x040000DD: value = 0x00; break;
        // DMA3 Control
        case 0x040000DE: value = gba_dma_read_control_lsb(dma->channel + 3); break;
        case 0x040000DF: value = gba_dma_read_control_msb(dma->channel + 3); break;
    }

    return value;
}


void gba_dma_reset(gba_dma_t* dma){
    memset(dma->channel,0x00,sizeof(dma->channel));
}