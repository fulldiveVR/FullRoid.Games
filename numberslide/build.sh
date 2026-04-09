#!/usr/bin/env bash
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "$0")" && pwd)"
OUT_NDS="$REPO_ROOT/../repository/nds"
OUT_3DS="$REPO_ROOT/../repository/3ds"

# Build the image if it does not exist
if ! docker image inspect numberslide-devkit >/dev/null 2>&1; then
    echo "==> numberslide-devkit image not found, building..."
    docker build -t numberslide-devkit "$REPO_ROOT"
fi

echo "==> Building NumberSlide for NDS and 3DS"

"$REPO_ROOT/build-nds.sh"
"$REPO_ROOT/build-3ds.sh"

# Copy artifacts to repository/
mkdir -p "$OUT_NDS" "$OUT_3DS"
cp "$REPO_ROOT/numberslide-nds/numberslide.nds"  "$OUT_NDS/numberslide.nds"
cp "$REPO_ROOT/numberslide-3ds/numberslide.3dsx" "$OUT_3DS/numberslide.3dsx"

echo ""
echo "==> Results:"
ls -lh "$OUT_NDS/numberslide.nds"
ls -lh "$OUT_3DS/numberslide.3dsx"
