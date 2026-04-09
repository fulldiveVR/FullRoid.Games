#!/usr/bin/env bash
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "$0")" && pwd)"

JOBS=$(sysctl -n hw.logicalcpu 2>/dev/null || nproc 2>/dev/null || echo 4)

echo "==> Building NumberSlide for Nintendo DS..."

docker run --rm \
  -v "$REPO_ROOT:/project" \
  -w /project/numberslide-nds \
  numberslide-devkit \
  bash -c "make clean 2>/dev/null || true; make -j$JOBS"

echo "==> Done: numberslide-nds/numberslide.nds"
