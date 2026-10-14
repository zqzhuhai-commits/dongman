cmake_minimum_required(VERSION 3.18)
project(dongman C)

# ============================================================
#  动漫共和国 - Switch Homebrew 动漫客户端
#  数据源: omofun.org.cn (MacCMS)
# ============================================================

set(CMAKE_TOOLCHAIN_FILE "$ENV{DEVKITPRO}/cmake/Switch.cmake")

# 可执行文件名
set(APP_TARGET dongman)

# 源文件
set(SOURCES
    source/main.c
    source/http.c
    source/parser.c
    source/ui.c
    source/player.c
)

add_executable(${APP_TARGET} ${SOURCES})

# 头文件路径
target_include_directories(${APP_TARGET} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# 链接库: SDL2, curl, m, z, nx
target_link_libraries(${APP_TARGET} PRIVATE
    SDL2
    curl
    m
    z
    nx
)

# ============================================================
#  NRO / NSP 打包配置
# ============================================================
set(NACP_NAME    "动漫共和国")
set(NACP_AUTHOR  "zqzhuhai")
set(NACP_VERSION "1.0.0")
set(NACP_ID      "0500d0d000d00000")   # 16位 hex, 自定义 appID

# 设置 romfs 目录
set_target_properties(${APP_TARGET} PROPERTIES
    SWITCH_ROMFS "${CMAKE_CURRENT_SOURCE_DIR}/romfs"
    SWITCH_NACP_NAME "${NACP_NAME}"
    SWITCH_NACP_AUTHOR "${NACP_AUTHOR}"
    SWITCH_NACP_VERSION "${NACP_VERSION}"
    SWITCH_NACP_ID "${NACP_ID}"
)

# 自动生成 NRO
switch_create_nro(${APP_TARGET})

# ============================================================
#  输出目录
# ============================================================
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/output)
