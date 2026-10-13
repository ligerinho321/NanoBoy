cmake_minimum_required(VERSION 3.25)

project(GBA LANGUAGES C CXX)

add_executable(
    GBA
    arm.c
    thumb.c
    cpu.c
    gba.c
    main.c
)