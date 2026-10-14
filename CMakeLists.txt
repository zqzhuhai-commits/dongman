cmake_minimum_required(VERSION 3.18)
project(dongman C)

# ============================================================
#  动漫共和国 - Switch Homebrew 动漫客户端
#  数据源: omofun.org.cn (MacCMS)
# ============================================================

# devkitPro portlibs 路径
set(PORTLIBS "$ENV{DEVKITPRO}/portlibs/switch")

# 源文件
set(SOURCES
    source/main.c
    source/http.c
    source/parser.c
    source/ui.c
    source/player.c
)

add_executable(${PROJECT_NAME} ${SOURCES})

# 头文件路径
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${PORTLIBS}/include
)

# 链接库
target_link_directories(${PROJECT_NAME} PRIVATE ${PORTLIBS}/lib)
target_link_libraries(${PROJECT_NAME} PRIVATE
    SDL2
    SDL2_image
    curl
    m
    z
    nx
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    OUTPUT_NAME "dongman"
)
