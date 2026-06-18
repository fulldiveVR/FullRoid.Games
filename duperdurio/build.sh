#!/usr/bin/env bash
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "$0")" && pwd)"
OUT_3DS="$REPO_ROOT/../repository/3ds"

# Build the image if it does not exist
if ! docker image inspect duperdurio-devkit >/dev/null 2>&1; then
    echo "==> duperdurio-devkit image not found, building..."
    docker build -t duperdurio-devkit "$REPO_ROOT"
fi

echo "==> Building DuperDurio for 3DS"

SRC_3DSX="$REPO_ROOT/duperdurio-3ds/duperdurio.3dsx"
mtime_before=$(stat -f %m "$SRC_3DSX" 2>/dev/null || echo 0)

"$REPO_ROOT/build-3ds.sh"

# Guard: never copy/deploy a stale binary if the build didn't actually rebuild it.
mtime_after=$(stat -f %m "$SRC_3DSX" 2>/dev/null || echo 0)
if [ "$mtime_after" = "$mtime_before" ]; then
    echo "==> ERROR: .3dsx not rebuilt (build failed?) — skipping copy & phone deploy." >&2
    exit 1
fi

mkdir -p "$OUT_3DS"
cp "$REPO_ROOT/duperdurio-3ds/duperdurio.3dsx" "$OUT_3DS/duperdurio.3dsx"

echo ""
echo "==> Result:"
ls -lh "$OUT_3DS/duperdurio.3dsx"

# Deploy to a connected Android phone's 3DS emulator roms folder, if present.
PHONE_DIR="/sdcard/Documents/Roms/3DS"
INDEX_FILE="$REPO_ROOT/.deploy_index"
if command -v adb >/dev/null 2>&1 \
   && [ -n "$(adb devices | awk 'NR>1 && $2=="device" {print $1; exit}')" ]; then
    # Persistent, ever-incrementing deploy index so each push lands under a new
    # filename — the emulator can't serve a stale build cached by name.
    idx=$(cat "$INDEX_FILE" 2>/dev/null || echo 0)
    case "$idx" in ''|*[!0-9]*) idx=0 ;; esac
    idx=$((idx + 1))
    echo "$idx" > "$INDEX_FILE"
    name="duperdurio_${idx}.3dsx"
    echo ""
    echo "==> Phone detected — deploying as $name to $PHONE_DIR"
    adb shell mkdir -p "$PHONE_DIR"
    # Remove previous indexed builds so only the latest remains.
    adb shell "rm -f $PHONE_DIR/duperdurio_*.3dsx"
    adb push "$REPO_ROOT/duperdurio-3ds/duperdurio.3dsx" "$PHONE_DIR/$name"
else
    echo "==> No adb device connected; skipping phone deploy."
fi
