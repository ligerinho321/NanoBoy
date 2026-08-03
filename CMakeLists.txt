cmake_minimum_required(VERSION 3.25)

project(NanoBoy LANGUAGES C CXX)

find_package(SDL2 REQUIRED)
find_package(zstd REQUIRED)

message("CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")
message("CMAKE_C_COMPILER: ${CMAKE_C_COMPILER}")
message("CMAKE_CXX_COMPILER: ${CMAKE_CXX_COMPILER}")

add_executable(NanoBoy)

#target_compile_options(NanoBoy PRIVATE -Wall -Wextra)

set_target_properties(
    NanoBoy
    PROPERTIES

    C_STANDARD 11
    C_STANDARD_REQUIRED ON
    C_EXTENSIONS ON

    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS ON
)

get_property(c_standard TARGET NanoBoy PROPERTY C_STANDARD)
message("C_STANDARD: ${c_standard}")

get_property(c_standard_required TARGET NanoBoy PROPERTY C_STANDARD_REQUIRED)
message("C_STANDARD_REQUIRED: ${c_standard_required}")

get_property(c_extensions TARGET NanoBoy PROPERTY C_EXTENSIONS)
message("C_EXTENSIONS: ${c_extensions}")

get_property(cxx_standard TARGET NanoBoy PROPERTY CXX_STANDARD)
message("CXX_STANDARD: ${cxx_standard}")

get_property(cxx_standard_required TARGET NanoBoy PROPERTY CXX_STANDARD_REQUIRED)
message("CXX_STANDARD_REQUIRED: ${cxx_standard_required}")

get_property(cxx_extensions TARGET NanoBoy PROPERTY CXX_EXTENSIONS)
message("CXX_EXTENSIONS: ${cxx_extensions}")

target_sources(
    NanoBoy
    PRIVATE
    
    core/mappers/mbc1.c
    core/mappers/mbc2.c
    core/mappers/mbc3.c
    core/mappers/mbc5.c
    core/mappers/mbc6.c
    core/mappers/mbc7.c
    core/mappers/mmm01.c
    core/mappers/huc1.c

    core/utils/blip_buf.c
    core/utils/ring_buffer.c
    core/utils/utils.c

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
    core/printer.c
    core/savestate.c
    core/serial.c
    core/timer.c
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
    gui/cJSON/cJSON.c
    gui/cheats/cheats.cpp
    gui/file_dialog/file_base_dialog.cpp
    gui/file_dialog/file_save_dialog.cpp
    gui/file_dialog/file_selector_dialog.cpp
    gui/object_viewer/object_viewer.cpp
    gui/palette_viewer/palette_viewer.cpp
    gui/printer/printer.cpp
    gui/savestate/savestate.cpp
    gui/screen/screen.cpp
    gui/input/input.cpp
    gui/tilemap_viewer/tilemap_viewer.cpp
    gui/utils/utils.cpp
    gui/wave_form/wave_form.cpp
)

target_sources(
    NanoBoy
    PRIVATE
    nanoboy.cpp
    main.cpp
)

target_include_directories(
    NanoBoy
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)


if(WIN32)
    target_link_libraries(NanoBoy PRIVATE SDL2::SDL2main)
    target_link_options(NanoBoy PRIVATE $<$<CONFIG:Release>:-mwindows>)
endif()

target_link_libraries(NanoBoy PRIVATE SDL2::SDL2)

target_link_libraries(NanoBoy PRIVATE zstd::libzstd_shared)


set(CMAKE_C_FLAGS_RELEASE "-O2 -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")