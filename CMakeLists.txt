cmake_minimum_required(VERSION 3.18)
project(dongman C)

# ============================================================
#  动漫共和国 - Switch Homebrew 动漫客户端
#  数据源: omofun.org.cn (MacCMS)
# ============================================================

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
)

# 链接库: SDL2, curl, m, z, nx
target_link_libraries(${PROJECT_NAME} PRIVATE
    SDL2
    SDL2_image
    curl
    m
    z
    nx
)

# ============================================================
#  NRO 打包配置 (devkitPro Switch.cmake 自动处理)
# ============================================================
set_target_properties(${PROJECT_NAME} PROPERTIES
    OUTPUT_NAME "dongman"
)
