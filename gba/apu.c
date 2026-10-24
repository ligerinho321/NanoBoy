#include "apu.h"
#include "gba.h"

const bool gba_square_duty_table[4][8] = {
    {0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,1},
    {1,0,0,0,0,1,1,1},
    {0,1,1,1,1,1,1,0}
};

const uint8_t gba_wave_volume_shift[4] = {
    4,0,1,2
};

const uint8_t gba_noise_divisor[8] = {
    8,16,32,48,64,80,96,112
};


void gba_apu_init(gba_apu_t* apu,gba_t* gba){
    apu->gba = gba;
}


uint16_t gba_sweep_get_new_frequency(gba_sweep_t* sweep){
    uint16_t delta = sweep->shadow_frequency >> sweep->shift;
    return sweep->negate ? sweep->shadow_frequency - delta : sweep->shadow_frequency + delta;
}

void gba_sweep_clock(gba_square_t* square){
    gba_sweep_t* sweep = &square->sweep;

    if(!sweep->period) return;

    if(sweep->timer && --sweep->timer > 0x00) return;

    uint16_t new_frequency = gba_sweep_get_new_frequency(sweep);

    sweep->calc_negate = sweep->negate;

    if(new_frequency > 0x7FF){
        square->enabled = false;
        return;
    }

    if(sweep->shift > 0x00){
        sweep->shadow_frequency = new_frequency;
        square->frequency.v = new_frequency;

        new_frequency = gba_sweep_get_new_frequency(sweep);

        if(new_frequency > 0x7FF){
            square->enabled = false;
        }
    }
}


void gba_envelope_clock(gba_envelope_t* envelope){
    if(!envelope->period) return;

    if(envelope->timer && --envelope->timer > 0x00) return;

    envelope->timer = envelope->period;

    if(!envelope->automatic_change) return;

    if(envelope->add_mode){
        if(envelope->current_volume < 0x0F){
            ++envelope->current_volume;
        }
        else{
            envelope->automatic_change = false;
        }
    }
    else{
        if(envelope->current_volume > 0x00){
            --envelope->current_volume;
        }
        else{
            envelope->automatic_change = false;
        }
    }
}


void gba_square_clock(gba_square_t* square,int cycles){
    square->timer -= cycles;

    if(square->timer > 0x00) return;

    square->timer = (0x800 - square->frequency.v) << 0x02;

    square->duty_pos = (square->duty_pos + 0x01) & 0x07;

    square->duty = square->new_duty;

    // TODO: Update Output
}

void gba_square_reset(gba_square_t* square){
    square->enabled = false;

    square->panning.right = false;
    square->panning.left = false;

    memset(&square->sweep,0x00,sizeof(square->sweep));
    memset(&square->length_counter,0x00,sizeof(square->length_counter));
    memset(&square->envelope,0x00,sizeof(square->envelope));

    square->new_duty = 0x00;
    square->duty = 0x00;
    square->duty_pos = 0x00;
    square->frequency.v = 0x00;
    square->timer = 0x00;
}


void gba_wave_clock(gba_wave_t* wave,int cycles){
    wave->timer -= cycles;

    if(wave->timer > 0x00) return;

    wave->timer = (0x800 - wave->frequency.v) << 0x01;

    uint8_t* ptr = wave->ram;

    if(!wave->double_banks && wave->bank){
        ptr += 0x10;
    }

    uint8_t n = wave->double_banks ? 0x20 : 0x10;

    uint8_t output = ptr[0] >> 0x04;
    ptr[0] <<= 0x04;

    for(uint8_t i = 0x01; i < n; ++i){
        ptr[i - 1] |= ptr[i] >> 0x04;
        ptr[i] <<= 0x04;
    }

    ptr[n - 0x01] |= output;

    // TODO: Update ouput
}

void gba_wave_reset(gba_wave_t* wave){
    wave->enabled = false;

    wave->panning.right = false;
    wave->panning.left = false;

    wave->double_banks = false;
    wave->bank = 0x00;
    wave->dac_enabled = false;

    memset(&wave->length_counter,0x00,sizeof(wave->length_counter));

    wave->volume_code = 0x00;
    wave->force_volume = false;

    wave->frequency.v = 0x00;
    wave->timer = 0x00;

    memset(&wave->ram,0x00,sizeof(wave->ram));
}


void gba_noise_clock(gba_noise_t* noise,int cycles){
    noise->timer -= cycles;

    if(noise->timer > 0x00) return;

    noise->timer = gba_noise_divisor[noise->divisor_code] << noise->clock_shift;

    //Using a noise channel clock shift of 14 or 15 results in the LFSR receiving no clocks.
    if(noise->clock_shift > 0x0D) return;

    bool bit = ((noise->lfsr & 0x02) ? 0x01 : 0x00) ^ ((noise->lfsr & 0x01) ? 0x01 : 0x00);

    noise->lfsr &= ~0x8000;
    noise->lfsr |= bit ? 0x8000 : 0x0000;

    if(noise->width_mode){
        noise->lfsr &= ~0x0080;
        noise->lfsr |= bit ? 0x0080 : 0x0000;
    }

    noise->lfsr >>= 0x01;

    // TODO: Update Output
}

void gba_noise_reset(gba_noise_t* noise){
    noise->enabled = false;

    noise->panning.right = false;
    noise->panning.left = false;

    memset(&noise->length_counter,0x00,sizeof(noise->length_counter));
    memset(&noise->envelope,0x00,sizeof(noise->envelope));

    noise->divisor_code = 0x00;
    noise->width_mode = false;
    noise->clock_shift = 0x00;
    noise->lfsr = 0x00;
    noise->timer = 0x00;
}


void gba_apu_write_register_raw(gba_apu_t* apu,uint8_t value,uint32_t address){
    switch(address){
        // NR10
        case 0x04000060: gba_square_write_register0(&apu->square1,value); break;
        case 0x04000061: break;
        // NR11
        case 0x04000062: gba_square_write_register1(&apu->square1,value); break;
        // NR12
        case 0x04000063: gba_square_write_register2(&apu->square1,value); break;

        // NR13
        case 0x04000064: gba_square_write_register3(&apu->square1,value); break;
        // NR14:
        case 0x04000065: gba_square_write_register4(&apu->square1,value); break;
        case 0x04000066: break;
        case 0x04000067: break;

        // NR21
        case 0x04000068: gba_square_write_register1(&apu->square2,value); break;
        // NR22
        case 0x04000069: gba_square_write_register2(&apu->square2,value); break;
        case 0x0400006A: break;
        case 0x0400006B: break;

        // NR23
        case 0x0400006C: gba_square_write_register3(&apu->square2,value); break;
        // NR24:
        case 0x0400006D: gba_square_write_register4(&apu->square2,value); break;
        case 0x0400006E: break;
        case 0x0400006F: break;

        // NR30
        case 0x04000070: gba_wave_write_register0(&apu->wave,value); break;
        case 0x04000071: break;
        // NR31
        case 0x04000072: gba_wave_write_register1(&apu->wave,value); break;
        // NR32
        case 0x04000073: gba_wave_write_register2(&apu->wave,value); break;

        // NR33
        case 0x04000074: gba_wave_write_register3(&apu->wave,value); break;
        // NR34
        case 0x04000075: gba_wave_write_register4(&apu->wave,value); break;
        case 0x04000076: break;
        case 0x04000077: break;

        // NR41
        case 0x04000078: gba_noise_write_register1(&apu->noise,value); break;
        // NR42
        case 0x04000079: gba_noise_write_register2(&apu->noise,value); break;
        case 0x0400007A: break;
        case 0x0400007B: break;

        // NR43
        case 0x0400007C: gba_noise_write_register3(&apu->noise,value); break;
        // NR44
        case 0x0400007D: gba_noise_write_register4(&apu->noise,value); break;
        case 0x0400007E: break;
        case 0x0400007F: break;

        // NR50
        case 0x04000080:{
            apu->master_volume.right = value & 0x07;
            apu->master_volume.left = (value >> 0x04) & 0x07;
            break;
        }
        // NR51
        case 0x04000081:{
            apu->square1.panning.right = (value & 0x01) != 0x00;
            apu->square1.panning.left = (value & 0x10) != 0x00;

            apu->square2.panning.right = (value & 0x02) != 0x00;
            apu->square2.panning.left = (value & 0x20) != 0x00;

            apu->wave.panning.right = (value & 0x04) != 0x00;
            apu->wave.panning.left = (value & 0x40) != 0x00;

            apu->noise.panning.right = (value & 0x08) != 0x00;
            apu->noise.panning.left = (value & 0x80) != 0x00;
            break;
        }
        // Control Mixing
        case 0x04000082:{
            apu->psg_volume = value & 0x03;
            apu->pcm1_volume = (value & 0x04) != 0x00;
            apu->pcm2_volume = (value & 0x08) != 0x00;
            break;
        }
        // PCM Control
        case 0x04000083:{
            apu->pcm1.panning.right = (value & 0x01) != 0x00;
            apu->pcm1.panning.left = (value & 0x02) != 0x00;
            apu->pcm1.timer_select = (value & 0x04) != 0x00;
            // TODO: Reset PCM1 FIFO

            apu->pcm2.panning.right = (value & 0x10) != 0x00;
            apu->pcm2.panning.left = (value & 0x20) != 0x00;
            apu->pcm2.timer_select = (value & 0x40) != 0x00;
            // TOOD: Reset PCM2 FIFO
            break;
        }

        // NR52
        case 0x04000084:{
            bool new_enabled = (value & 0x80) != 0x00;

            if(!apu->enabled && new_enabled){
                apu->enabled = true;
            }
            else if(apu->enabled && !new_enabled){
                gba_apu_reset(apu);
            }
            break;
        }
        case 0x04000085: break;
        case 0x04000086: break;
        case 0x04000087: break;

        // PWM Control
        case 0x04000088:{
            apu->bias.b[0] = value & 0xFE;
            break;
        }
        case 0x04000089:{
            apu->bias.b[1] = value & 0x03;

            apu->sample_rate = (value >> 0x06) & 0x03;
            break;
        }
        case 0x0400008A: break;
        case 0x0400008B: break;

        //Wave RAM
        case 0x04000090:
        case 0x04000091:
        case 0x04000092:
        case 0x04000093:
        case 0x04000094:
        case 0x04000095:
        case 0x04000096:
        case 0x04000097:
        case 0x04000098:
        case 0x04000099:
        case 0x0400009A:
        case 0x0400009B:
        case 0x0400009C:
        case 0x0400009D:
        case 0x0400009E:
        case 0x0400009F:{
            apu->wave.ram[(apu->wave.bank << 0x04) | (address & 0x0F)] = value;
            break;
        }

        // PCM1 FIFO
        case 0x040000A0: break;
        case 0x040000A1: break;
        case 0x040000A2: break;
        case 0x040000A3: break;

        // PCM2 FIFO
        case 0x040000A4: break;
        case 0x040000A5: break;
        case 0x040000A6: break;
        case 0x040000A7: break;
    }
}

uint8_t gba_apu_read_register_raw(gba_apu_t* apu,uint32_t address){
    uint8_t value = apu->gba->memory.bus.b[address & 0x03];

    switch(address){
        // NR10
        case 0x04000060: value = gba_square_read_register0(&apu->square1); break;
        case 0x04000061: value = 0x00; break;
        // NR11
        case 0x04000062: value = gba_square_read_register1(&apu->square1); break;
        // NR12
        case 0x04000063: value = gba_square_read_register2(&apu->square1); break;

        // NR13
        case 0x04000064: value = 0x00; break;
        // NR14:
        case 0x04000065: value = gba_square_read_register4(&apu->square1); break;
        case 0x04000066: value = 0x00; break;
        case 0x04000067: value = 0x00; break;

        // NR21
        case 0x04000068: value = gba_square_read_register1(&apu->square2); break;
        // NR22
        case 0x04000069: value = gba_square_read_register2(&apu->square2); break;
        case 0x0400006A: value = 0x00; break;
        case 0x0400006B: value = 0x00; break;

        // NR23
        case 0x0400006C: value = 0x00; break;
        // NR24:
        case 0x0400006D: value = gba_square_read_register4(&apu->square2); break;
        case 0x0400006E: value = 0x00; break;
        case 0x0400006F: value = 0x00; break;

        // NR30
        case 0x04000070: value = gba_wave_read_register0(&apu->wave); break;
        case 0x04000071: value = 0x00; break;
        // NR31
        case 0x04000072: value = 0x00; break;
        // NR32
        case 0x04000073: value = gba_wave_read_register2(&apu->wave); break;

        // NR33
        case 0x04000074: value = 0x00; break;
        // NR34
        case 0x04000075: value = gba_wave_read_register4(&apu->wave); break;
        case 0x04000076: value = 0x00; break;
        case 0x04000077: value = 0x00; break;

        // NR41
        case 0x04000078: value = 0x00; break;
        // NR42
        case 0x04000079: value = gba_noise_read_register2(&apu->noise); break;
        case 0x0400007A: value = 0x00; break;
        case 0x0400007B: value = 0x00; break;

        // NR43
        case 0x0400007C: value = gba_noise_read_register3(&apu->noise); break;
        // NR44
        case 0x0400007D: value = gba_noise_read_register4(&apu->noise); break;
        case 0x0400007E: value = 0x00; break;
        case 0x0400007F: value = 0x00; break;

        // NR50
        case 0x04000080:{
            value = (
                (apu->master_volume.right & 0x07) |
                ((apu->master_volume.left & 0x07) << 0x04)
            );
            break;
        }
        // NR51
        case 0x04000081:{
            value = (
                (apu->square1.panning.right ? 0x01 : 0x00) |
                (apu->square2.panning.right ? 0x02 : 0x00) |
                (apu->wave.panning.right ? 0x04 : 0x00) |
                (apu->noise.panning.right ? 0x08 : 0x00) |
                (apu->square1.panning.left ? 0x10 : 0x00) |
                (apu->square2.panning.left ? 0x20 : 0x00) |
                (apu->wave.panning.left ? 0x40 : 0x00) |
                (apu->noise.panning.left ? 0x80 : 0x00)
            );
            break;
        }
        // Control Mixing
        case 0x04000082:{
            value = (
                (apu->psg_volume & 0x03) |
                (apu->pcm1_volume ? 0x04 : 0x00) |
                (apu->pcm2_volume ? 0x08 : 0x00)
            );
            break;
        }
        // PCM Control
        case 0x04000083:{
            value = (
                (apu->pcm1.panning.right ? 0x01 : 0x00) |
                (apu->pcm1.panning.left ? 0x02 : 0x00) |
                (apu->pcm1.timer_select ? 0x04 : 0x00) |
                (apu->pcm2.panning.right ? 0x10 : 0x00) |
                (apu->pcm2.panning.left ? 0x20 : 0x00) |
                (apu->pcm2.timer_select ? 0x40 : 0x00)
            );
            break;
        }

        // NR52
        case 0x04000084:{
            value = (
                (apu->square1.enabled ? 0x01 : 0x00) |
                (apu->square2.enabled ? 0x02 : 0x00) |
                (apu->wave.enabled ? 0x04 : 0x00) |
                (apu->noise.enabled ? 0x08 : 0x00) |
                (apu->enabled ? 0x80 : 0x00)
            );
            break;
        }
        case 0x04000085: value = 0x00; break;
        case 0x04000086: value = 0x00; break;
        case 0x04000087: value = 0x00; break;


        // PWM Control
        case 0x04000088:{
            value = apu->bias.b[0] & 0xFE;
            break;
        }
        case 0x04000089:{
            value = (
                (apu->bias.b[1] & 0x03) |
                ((apu->sample_rate & 0x03) << 0x06)
            );
            break;
        }
        case 0x0400008A: value = 0x00; break;
        case 0x0400008B: value = 0x00; break;

        //Wave RAM
        case 0x04000090:
        case 0x04000091:
        case 0x04000092:
        case 0x04000093:
        case 0x04000094:
        case 0x04000095:
        case 0x04000096:
        case 0x04000097:
        case 0x04000098:
        case 0x04000099:
        case 0x0400009A:
        case 0x0400009B:
        case 0x0400009C:
        case 0x0400009D:
        case 0x0400009E:
        case 0x0400009F:{
            value = apu->wave.ram[(apu->wave.bank << 0x04) | (address & 0x0F)];
            break;
        }
    }

    return value;
}


void gba_envelope_write(gba_envelope_t* envelope,uint8_t value,bool *channel_enabled){
    envelope->period = value & 0x07;
    envelope->add_mode = (value & 0x08) != 0x00;
    envelope->initial_volume = (value >> 0x04) & 0x0F;

    if(!envelope->initial_volume && !envelope->add_mode){
        *channel_enabled = false;
    }
}

uint8_t gba_envelope_read(gba_envelope_t* envelope){
    return (
        (envelope->period & 0x07) |
        (envelope->add_mode ? 0x08 : 0x00) |
        ((envelope->initial_volume & 0x0F) << 0x04)
    );
}

void gba_envelope_trigger(gba_envelope_t* envelope){
    envelope->timer = envelope->period;
    envelope->current_volume = envelope->initial_volume;
    envelope->automatic_change = true;
}


void gba_square_write_register0(gba_square_t* square,uint8_t value){
    gba_sweep_t* sweep = &square->sweep;

    sweep->shift = value & 0x07;
    sweep->period = (value >> 0x04) & 0x07;

    bool new_negate = (value & 0x08) != 0x00;

    if(sweep->negate && !new_negate && sweep->calc_negate){
        square->enabled = false;
    }

    sweep->negate = new_negate;
}

uint8_t gba_square_read_register0(gba_square_t* square){
    return (
        (square->sweep.shift & 0x07) |
        (square->sweep.negate ? 0x08 : 0x00) |
        ((square->sweep.period & 0x07) << 0x04)
    );
}


void gba_square_write_register1(gba_square_t* square,uint8_t value){
    square->length_counter.counter = 0x40 - (value & 0x3F);

    //A alteracao do duty só entra em vigor apos a amostra atual ser concluida
    //Isso é necessario para passar no teste channel_2_duty_delay.gb
    square->new_duty = (value >> 0x06) & 0x03;
}

uint8_t gba_square_read_register1(gba_square_t* square){
    return (square->new_duty & 0x03) << 0x06;
}


void gba_square_write_register2(gba_square_t* square,uint8_t value){
    gba_envelope_write(&square->envelope,value,&square->enabled);
}

uint8_t gba_square_read_register2(gba_square_t* square){
    return gba_envelope_read(&square->envelope);
}


void gba_square_write_register3(gba_square_t* square,uint8_t value){
    square->frequency.b[0] = value;
}


void gba_square_write_register4(gba_square_t* square,uint8_t value){
    square->frequency.b[1] = value & 0x07;

    if(value & 0x80){

        square->enabled = square->envelope.initial_volume || square->envelope.add_mode;

        square->timer = (0x800 - square->frequency.v) << 0x02;

        if(square->length_counter.counter == 0x00){
            square->length_counter.counter = 0x40;
        }

        square->duty = square->new_duty;

        gba_envelope_trigger(&square->envelope);

        if(square->has_sweep){
            square->sweep.shadow_frequency = square->frequency.v;
            square->sweep.timer = square->sweep.period;
            square->sweep.calc_negate = false;

            if(square->sweep.shift){
                uint16_t new_frequency = gba_sweep_get_new_frequency(&square->sweep);
                square->sweep.calc_negate = square->sweep.negate;
                if(new_frequency > 0x7FF){
                    square->enabled = false;
                }
            }
        }
    }

    square->length_counter.enabled = (value & 0x40) != 0x00;
}

uint8_t gba_square_read_register4(gba_square_t* square){
    return (square->length_counter.enabled ? 0x40 : 0x00);
}



void gba_wave_write_register0(gba_wave_t* wave,uint8_t value){
    wave->double_banks = (value & 0x20) != 0x00;
    wave->bank = (value & 0x40) != 0x00;
    wave->dac_enabled = (value & 0x80) != 0x00;

    if(!wave->dac_enabled && wave->enabled){
        wave->enabled = false;
    }
}

uint8_t gba_wave_read_register0(gba_wave_t* wave){
    return (
        (wave->double_banks ? 0x20 : 0x00) |
        (wave->bank ? 0x40 : 0x00) |
        (wave->dac_enabled ? 0x80 : 0x00)
    );
}


void gba_wave_write_register1(gba_wave_t* wave,uint8_t value){
    wave->length_counter.counter = 0x100 - value;
}


void gba_wave_write_register2(gba_wave_t* wave,uint8_t value){
    wave->volume_code = (value >> 0x05) & 0x03;
    wave->force_volume = (value & 0x80) != 0x00;
}

uint8_t gba_wave_read_register2(gba_wave_t* wave){
    return (
        ((wave->volume_code & 0x03) << 0x05) |
        (wave->force_volume ? 0x80 : 0x00)
    );
}


void gba_wave_write_register3(gba_wave_t* wave,uint8_t value){
    wave->frequency.b[0] = value;
}


void gba_wave_write_register4(gba_wave_t* wave,uint8_t value){
    wave->frequency.b[1] = value & 0x07;

    if(value & 0x80){
        wave->enabled = wave->dac_enabled;

        wave->timer = (0x800 - wave->frequency.v) << 0x01;

        if(wave->length_counter.counter == 0x00){
            wave->length_counter.counter = 0x100;
        }
    }

    wave->length_counter.enabled = (value & 0x40) != 0x00;
}

uint8_t gba_wave_read_register4(gba_wave_t* wave){
    return (wave->length_counter.enabled ? 0x40 : 0x00);
}


void gba_noise_write_register1(gba_noise_t* noise,uint8_t value){
    noise->length_counter.counter = 0x40 - (value & 0x3F);
}


void gba_noise_write_register2(gba_noise_t* noise,uint8_t value){
    gba_envelope_write(&noise->envelope,value,&noise->enabled);
}

uint8_t gba_noise_read_register2(gba_noise_t* noise){
    return gba_envelope_read(&noise->envelope);
}


void gba_noise_write_register3(gba_noise_t* noise,uint8_t value){
    noise->divisor_code = value & 0x07;
    noise->width_mode = (value & 0x08) != 0x00;
    noise->clock_shift = (value >> 0x04) & 0x0F;
}

uint8_t gba_noise_read_register3(gba_noise_t* noise){
    return (
        (noise->divisor_code & 0x07) |
        (noise->width_mode ? 0x08 : 0x00) |
        ((noise->clock_shift & 0x0F) << 0x04)
    );
}


void gba_noise_write_register4(gba_noise_t* noise,uint8_t value){

    if(value & 0x80){
        noise->enabled = noise->envelope.initial_volume || noise->envelope.add_mode;

        noise->timer = gba_noise_divisor[noise->divisor_code] << noise->clock_shift;

        if(noise->length_counter.counter == 0x00){
            noise->length_counter.counter = 0x40;
        }

        gba_envelope_trigger(&noise->envelope);

        noise->lfsr = 0x7FFF;
    }

    noise->length_counter.enabled = (value & 0x40) != 0x00;
}

uint8_t gba_noise_read_register4(gba_noise_t* noise){
    return (noise->length_counter.enabled ? 0x40 : 0x00);
}


void gba_apu_reset(gba_apu_t* apu){
    apu->enabled = false;

    apu->master_volume.right = 0;
    apu->master_volume.left = 0;

    apu->psg_volume = 0x00;
    apu->pcm1_volume = 0x00;
    apu->pcm2_volume = 0x00;

    apu->bias.v = 0x00;
    apu->sample_rate = 0x00;
    
    gba_square_reset(&apu->square1);
    gba_square_reset(&apu->square2);
    gba_wave_reset(&apu->wave);
    gba_noise_reset(&apu->noise);
}