#!/usr/bin/env bash
#
# push-roms.sh — Push ROM files to Documents/Roms on a connected Android device.
#
# Usage:
#   ./push-roms.sh [-s suffix] <file_or_dir> [file_or_dir ...]
#
# Options:
#   -s suffix   Append suffix to filename before extension
#
# Examples:
#   ./push-roms.sh game.zip                    → game.zip
#   ./push-roms.sh -s _v2 game.nds             → game_v2.nds
#   ./push-roms.sh -s _debug ~/Downloads/*.nes
#   ./push-roms.sh roms/

set -euo pipefail

DEVICE_DEST="/sdcard/Documents/Roms"

die() { echo "error: $*" >&2; exit 1; }

command -v adb &>/dev/null || die "adb not found — add Android SDK platform-tools to PATH"

SUFFIX=""
if [[ "${1:-}" == "-s" ]]; then
    [[ $# -ge 2 ]] || die "-s requires a suffix argument"
    SUFFIX="$2"
    shift 2
fi

[[ $# -gt 0 ]] || { echo "Usage: $(basename "$0") [-s suffix] <file_or_dir> [...]" >&2; exit 1; }

# Check device is connected
DEVICES=$(adb devices | awk 'NR>1 && /\tdevice$/ {print $1}')
[[ -n "$DEVICES" ]] || die "no device connected (run: adb devices)"

DEVICE_COUNT=$(echo "$DEVICES" | wc -l | tr -d ' ')
if [[ "$DEVICE_COUNT" -gt 1 ]]; then
    echo "Multiple devices connected:"
    echo "$DEVICES"
    echo ""
    read -rp "Enter device serial to use: " SERIAL
    ADB="adb -s $SERIAL"
else
    SERIAL="$DEVICES"
    ADB="adb -s $SERIAL"
    echo "Device: $SERIAL"
fi

# Ensure destination directory exists
$ADB shell mkdir -p "$DEVICE_DEST"

PUSHED=0
FAILED=0

push_file() {
    local src="$1"
    local filename
    filename="$(basename "$src")"
    if [[ -n "$SUFFIX" ]]; then
        local name="${filename%.*}"
        local ext="${filename##*.}"
        if [[ "$name" == "$ext" ]]; then
            # No extension
            filename="${name}${SUFFIX}"
        else
            filename="${name}${SUFFIX}.${ext}"
        fi
    fi
    echo "  pushing: $filename"
    if $ADB push "$src" "$DEVICE_DEST/$filename"; then
        (( PUSHED++ )) || true
    else
        echo "  FAILED:  $filename" >&2
        (( FAILED++ )) || true
    fi
}

echo "Destination: $DEVICE_DEST"
echo ""

for ARG in "$@"; do
    if [[ -d "$ARG" ]]; then
        # Directory: push all files inside (non-recursive, one level)
        shopt -s nullglob
        FILES=("$ARG"/*)
        shopt -u nullglob
        [[ ${#FILES[@]} -gt 0 ]] || { echo "  (empty directory: $ARG)"; continue; }
        for F in "${FILES[@]}"; do
            [[ -f "$F" ]] && push_file "$F"
        done
    elif [[ -f "$ARG" ]]; then
        push_file "$ARG"
    else
        echo "  skipped (not found): $ARG" >&2
        (( FAILED++ )) || true
    fi
done

echo ""
echo "Done — $PUSHED pushed, $FAILED failed."
