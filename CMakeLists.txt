cmake_minimum_required(VERSION 3.25)

project(NanoBoy LANGUAGES C CXX)

find_package(SDL2 REQUIRED)

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
