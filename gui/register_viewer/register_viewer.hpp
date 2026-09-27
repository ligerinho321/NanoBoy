#pragma once

#include <gui/utils/utils.hpp>

class register_viewer_t {
private:
    gb_t* gb = nullptr;

    bool open = false;

    void leaf(uint16_t address,uint8_t value);
    void leaf(const char* label,const char* fmt,...);

    bool tree(uint16_t address,uint8_t value);
    bool tree(const char* label);
    void tree_end();

    void joypad();
    void serial();
    void timer();
    void interrupt();
    void square1_channel();
    void square2_channel();
    void wave_channel();
    void noise_channel();
    void apu_control();
    void wave_ram();
    void ppu();
    void palette();
    void dma();
    void others();

    bool bit_active(uint8_t value,uint8_t bit) const noexcept {
        return (value & (1 << (bit & 0x07))) ? true : false;
    }

public:
    void init(gb_t* _gb);

    void render();

    void set_open(bool _open) noexcept {
        open = _open;
    }

    bool get_open() const noexcept {
        return open;
    }
};