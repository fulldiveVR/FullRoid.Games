#!/usr/bin/env bash
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "$0")" && pwd)"
OUT_NDS="$REPO_ROOT/../repository/nds"
OUT_3DS="$REPO_ROOT/../repository/3ds"

# Build the image if it does not exist
if ! docker image inspect lunarrunner-devkit >/dev/null 2>&1; then
    echo "==> lunarrunner-devkit image not found, building..."
    docker build -t lunarrunner-devkit "$REPO_ROOT"
fi

echo "==> Building Lunar Runner for NDS and 3DS"

"$REPO_ROOT/build-nds.sh"
"$REPO_ROOT/build-3ds.sh"

# Copy artifacts to repository/
mkdir -p "$OUT_NDS" "$OUT_3DS"
cp "$REPO_ROOT/lunarrunner-nds/lunarrunner.nds"  "$OUT_NDS/lunarrunner.nds"
cp "$REPO_ROOT/lunarrunner-3ds/lunarrunner.3dsx" "$OUT_3DS/lunarrunner.3dsx"

echo ""
echo "==> Results:"
ls -lh "$OUT_NDS/lunarrunner.nds"
ls -lh "$OUT_3DS/lunarrunner.3dsx"
