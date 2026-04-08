#!/usr/bin/env bash
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "$0")" && pwd)"
OUT_NDS="$REPO_ROOT/../repository/nds"
OUT_3DS="$REPO_ROOT/../repository/3ds"

# Собрать образ если его нет
if ! docker image inspect snake-devkit >/dev/null 2>&1; then
    echo "==> Образ snake-devkit не найден, собираем..."
    docker build -t snake-devkit "$REPO_ROOT"
fi

echo "==> Сборка Snake для NDS и 3DS"

"$REPO_ROOT/build-nds.sh"
"$REPO_ROOT/build-3ds.sh"

# Копируем артефакты в repository/
mkdir -p "$OUT_NDS" "$OUT_3DS"
cp "$REPO_ROOT/snake-nds/snake.nds"  "$OUT_NDS/snake.nds"
cp "$REPO_ROOT/snake-3ds/snake.3dsx" "$OUT_3DS/snake.3dsx"

echo ""
echo "==> Результаты:"
ls -lh "$OUT_NDS/snake.nds"
ls -lh "$OUT_3DS/snake.3dsx"
