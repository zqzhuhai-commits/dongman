#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
: "${DEVKITPRO:?devkitPro environment is required}"
: "${SWITCH_KEYS_FILE:?Set SWITCH_KEYS_FILE to your own local prod.keys file}"
: "${HACBREWPACK:=hacbrewpack}"
test -f "$SWITCH_KEYS_FILE"
test -f dongman.elf
test -f dongman.nacp
# Do not mistake libnx's ExeFS PFS0 .nsp for an installable NCA-based NSP.
stage=$(mktemp -d)
trap 'rm -rf "$stage"' EXIT
mkdir -p "$stage/exefs" "$stage/control" dist
elf2nso dongman.elf "$stage/exefs/main"
npdmtool packaging/application.json "$stage/exefs/main.npdm"
cp dongman.nacp "$stage/control/control.nacp"
cp "${DEVKITPRO}/libnx/default_icon.jpg" "$stage/control/icon_AmericanEnglish.dat"
"$HACBREWPACK" --keyset "$SWITCH_KEYS_FILE" --exefsdir "$stage/exefs" --controldir "$stage/control" --noromfs --nologo --tempdir "$stage/temp" --ncadir "$stage/nca" --nspdir "$stage/nsp"
shopt -s nullglob
packages=("$stage/nsp/"*.nsp)
if (( ${#packages[@]} != 1 )); then
 echo "Expected one installable NSP, found ${#packages[@]}" >&2
 exit 1
fi
cp "${packages[0]}" dist/dongman.nsp
printf 'Created dist/dongman.nsp\n'
