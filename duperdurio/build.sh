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

"$REPO_ROOT/build-3ds.sh"

mkdir -p "$OUT_3DS"
cp "$REPO_ROOT/duperdurio-3ds/duperdurio.3dsx" "$OUT_3DS/duperdurio.3dsx"

echo ""
echo "==> Result:"
ls -lh "$OUT_3DS/duperdurio.3dsx"
