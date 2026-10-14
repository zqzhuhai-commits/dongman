cmake_minimum_required(VERSION 3.18)
project(dongman C)

set(PORTLIBS "$ENV{DEVKITPRO}/portlibs/switch")

set(SOURCES
    source/main.c
    source/http.c
    source/parser.c
    source/ui.c
    source/player.c
)

add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${PORTLIBS}/include
)

target_link_directories(${PROJECT_NAME} PRIVATE ${PORTLIBS}/lib)

target_link_libraries(${PROJECT_NAME} PRIVATE
    SDL2
    SDL2_image
    EGL
    GLESv2
    glad
    curl
    m
    z
    nx
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    OUTPUT_NAME "dongman"
)
