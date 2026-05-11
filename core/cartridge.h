#pragma once

#include "./utils.h"

typedef enum _gb_cartridge_component_t {
    gb_cartridge_ram = 0x01,
    gb_cartridge_battery = 0x02,
    gb_cartridge_timer = 0x04,
    gb_cartridge_rumble = 0x08,
    gb_cartridge_sensor = 0x10
} gb_cartridge_component_t;