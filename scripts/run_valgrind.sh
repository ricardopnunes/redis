#!/usr/bin/env bash
set -euo pipefail

BUILD_DIR="build"
TEST_BIN="${BUILD_DIR}/tests/redis_tests"

if [[ ! -f "$TEST_BIN" ]]; then
    echo "❌ Binário de testes não encontrado! Rode: cmake -S . -B build && cmake --build build"
    exit 1
fi

echo "🔍 Executando Valgrind..."
valgrind \
    --leak-check=full \
    --show-leak-kinds=all \
    --track-origins=yes \
    --verbose \
    --error-exitcode=1 \
    "$TEST_BIN"

echo "✅ Nenhum vazamento detectado!"