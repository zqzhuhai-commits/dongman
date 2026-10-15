#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
output=${1:-build/dongman-host}
mkdir -p "$(dirname "$output")"
${CXX:-g++} -std=c++17 -Wall -Wextra -Werror -Isource source/*.cpp \
  $(pkg-config --cflags --libs sdl2 SDL2_image SDL2_ttf mpv libcurl) \
  -pthread -o "$output"
