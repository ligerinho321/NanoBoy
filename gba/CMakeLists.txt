cmake_minimum_required(VERSION 3.25)

project(GBA LANGUAGES C CXX)

add_executable(
    GBA
    apu.c
    arm.c
    thumb.c
    cpu.c
    interrupt.c
    ppu.c
    timer.c
    memory.c
    dma.c
    keypad.c
    serial.c
    cartridge.c
    bios.c
    gba.c
    utils.c
    main.c

    ../third_party/cJSON/cJSON.c
)