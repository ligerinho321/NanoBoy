cmake_minimum_required(VERSION 3.25)

project(NanoBoy)

find_package(SDL2 REQUIRED)

add_executable(NanoBoy)

target_compile_features(
    NanoBoy
    PRIVATE
    cxx_std_17
    c_std_17
)

target_sources(
    NanoBoy
    PRIVATE
    cJSON/cJSON.c
)

target_sources(
    NanoBoy
    PRIVATE
    core/blip_buf/blip_buf.c
    core/mappers/mbc1.c
    core/mappers/mbc2.c
    core/mappers/mbc3.c
    core/mappers/mbc5.c
    core/apu.c
    core/boot.c
    core/cartridge.c
    core/cpu.c
    core/dma.c
    core/gb.c
    core/interrupt.c
    core/joypad.c
    core/memory.c
    core/palette.c
    core/ppu.c
    core/serial.c
    core/timer.c
    core/utils.c
)

target_sources(
    NanoBoy
    PRIVATE
    imgui/imgui_demo.cpp
    imgui/imgui_draw.cpp
    imgui/imgui_impl_sdl2.cpp
    imgui/imgui_impl_sdlrenderer2.cpp
    imgui/imgui_tables.cpp
    imgui/imgui_widgets.cpp
    imgui/imgui.cpp
)

target_sources(
    NanoBoy
    PRIVATE
    gui/cheats.cpp
    gui/file_selector.cpp
    gui/object_viewer.cpp
    gui/palette_viewer.cpp
    gui/screen.cpp
    gui/tilemap_viewer.cpp
    gui/utils.cpp
    gui/wave_form.cpp
)

target_sources(
    NanoBoy
    PRIVATE
    nanoboy.cpp
    main.cpp
)

target_link_libraries(NanoBoy PRIVATE SDL2::SDL2)

if(WIN32)
    target_link_libraries(NanoBoy PRIVATE SDL2::SDL2main)

    add_custom_command(
        TARGET
            NanoBoy
        POST_BUILD COMMAND 
            ${CMAKE_COMMAND} -E copy_if_different 
            "$<TARGET_FILE:SDL2::SDL2>"
            "$<TARGET_FILE_DIR:NanoBoy>"
    )
endif()


if(MSVC)
    set(CMAKE_C_FLAGS_RELEASE "/O2")
    set(CMAKE_CXX_FLAGS_RELEASE "/O2")

    if(WIN32)
        target_link_options(NanoBoy PRIVATE $<$<CONFIG:Release>:/SUBSYSTEM:WINDOWS>)
    endif()
else()
    set(CMAKE_C_FLAGS_RELEASE "-O2")
    set(CMAKE_CXX_FLAGS_RELEASE "-O2")

    if(WIN32)
        target_link_options(NanoBoy PRIVATE $<$<CONFIG:Release>:-mwindows>)
    endif()
endif()
