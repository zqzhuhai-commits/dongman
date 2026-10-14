#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
bin=$(mktemp)
trap 'rm -f "$bin"' EXIT
g++ -std=c++17 -Wall -Wextra -Werror -Isource source/site.cpp tests/site_test.cpp -o "$bin"
"$bin"
python3 -m json.tool packaging/application.json >/dev/null
bash -n scripts/package-nsp.sh
