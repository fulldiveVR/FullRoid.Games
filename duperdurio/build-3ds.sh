#!/usr/bin/env bash
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "$0")" && pwd)"

JOBS=$(sysctl -n hw.logicalcpu 2>/dev/null || nproc 2>/dev/null || echo 4)

echo "==> Building DuperDurio for 3DS..."

docker run --rm \
  -v "$REPO_ROOT:/project" \
  -w /project/duperdurio-3ds \
  duperdurio-devkit \
  bash -c '
    set -e
    make clean 2>/dev/null || true
    # Build the AI sprite atlas (romfs/gfx/sprites.t3x) from the tex3ds script.
    if [ -f gfx/sprites.t3s ]; then
      mkdir -p romfs/gfx
      echo "==> Packing sprite atlas with tex3ds..."
      tex3ds -i gfx/sprites.t3s -o romfs/gfx/sprites.t3x
    fi
    # 3dsxtool requires an SMDH when embedding RomFS — build one from icon.png.
    echo "==> Building SMDH metadata..."
    smdhtool --create "DuperDurio" "Classic Super Durio for 3DS" "DuperDurio Port" icon.png duperdurio.smdh
    make -j'"$JOBS"'
  '

echo "==> Done: duperdurio-3ds/duperdurio.3dsx"
