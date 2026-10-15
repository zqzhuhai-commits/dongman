#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
: "${SWITCH_KEYS_FILE:?Set SWITCH_KEYS_FILE to your own local prod.keys file}"
: "${HACBREWPACK:=hacbrewpack}"
test -f "$SWITCH_KEYS_FILE"
# Do not mistake libnx's ExeFS PFS0 .nsp for an installable NCA-based NSP.
stage=$(mktemp -d)
trap 'rm -rf "$stage"' EXIT
mkdir -p "$stage/exefs" "$stage/control" dist
if [[ -n "${NSP_INPUT_DIR:-}" ]]; then
  # Use the exact native inputs built by CI, without a local SDK or key upload.
  test -f "$NSP_INPUT_DIR/exefs/main"
  test -f "$NSP_INPUT_DIR/exefs/main.npdm"
  test -f "$NSP_INPUT_DIR/control/control.nacp"
  cp -R "$NSP_INPUT_DIR/exefs/." "$stage/exefs/"
  cp -R "$NSP_INPUT_DIR/control/." "$stage/control/"
else
  : "${DEVKITPRO:?devkitPro environment is required}"
  test -f dongman.elf
  test -f dongman.nacp
  elf2nso dongman.elf "$stage/exefs/main"
  npdmtool packaging/application.json "$stage/exefs/main.npdm"
  cp dongman.nacp "$stage/control/control.nacp"
  cp "${DEVKITPRO}/libnx/default_icon.jpg" "$stage/control/icon_AmericanEnglish.dat"
fi
# Some tool versions print unrecognized key values. Keep tool output private
# and ephemeral instead of forwarding it to terminals or CI logs.
if ! "$HACBREWPACK" --keyset "$SWITCH_KEYS_FILE" --exefsdir "$stage/exefs" --controldir "$stage/control" --noromfs --nologo --tempdir "$stage/temp" --ncadir "$stage/nca" --nspdir "$stage/nsp" > "$stage/packaging.log" 2>&1; then
  echo "NSP packaging failed; verify the native inputs and required keys." >&2
  exit 1
fi
shopt -s nullglob
packages=("$stage/nsp/"*.nsp)
if (( ${#packages[@]} != 1 )); then
 echo "Expected one installable NSP, found ${#packages[@]}" >&2
 exit 1
fi
cp "${packages[0]}" dist/dongman.nsp
printf 'Created dist/dongman.nsp\n'
