#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
stage=$(mktemp -d)
trap 'rm -rf "$stage"' EXIT
ffmpeg -hide_banner -loglevel error -f lavfi -i 'color=red:size=64x64:rate=24' \
  -f lavfi -i 'sine=frequency=440:sample_rate=48000' -t 3 \
  -c:v mpeg4 -c:a aac "$stage/clip.mp4"
${CXX:-g++} -std=c++17 -Wall -Wextra -Werror -Isource \
  tests/player_test.cpp source/network.cpp source/site.cpp \
  $(pkg-config --cflags --libs sdl2 SDL2_image SDL2_ttf mpv libcurl) \
  -pthread -o "$stage/test-player"
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy "$stage/test-player" "$stage/clip.mp4"
