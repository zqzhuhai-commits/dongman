#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
bin=$(mktemp)
trap 'rm -f "$bin"' EXIT
g++ -std=c++17 -Wall -Wextra -Werror -Isource source/site.cpp tests/site_test.cpp -o "$bin"
"$bin"
python3 -m json.tool packaging/application.json >/dev/null
python3 - <<'PY'
import ssl
context = ssl.create_default_context(cafile="romfs/cacert.pem")
assert len(context.get_ca_certs()) > 100
print("Bundled CA certificate store validated")
PY
bash -n scripts/package-nsp.sh
