#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
bin=$(mktemp)
trap 'rm -f "$bin"' EXIT
${CXX:-g++} -std=c++17 -Wall -Wextra -Werror -Isource \
  source/network.cpp tests/network_test.cpp \
  $(pkg-config --cflags --libs libcurl) -pthread -o "$bin"
python3 tests/network_server.py "$bin"
