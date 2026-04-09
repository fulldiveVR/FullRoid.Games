#!/usr/bin/env bash
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "$0")" && pwd)"
NDS_ROM="$REPO_ROOT/snake-nds/snake.nds"

if [ ! -f "$NDS_ROM" ]; then
  echo "ERROR: $NDS_ROM not found. Run build-nds.sh first."
  exit 1
fi

echo "==> Checking snake.nds with ndstool..."

docker run --rm \
  -v "$REPO_ROOT:/project" \
  -w /project/snake-nds \
  snake-devkit \
  ndstool -i snake.nds

echo "==> Check complete."
