#!/usr/bin/env bash
set -euo pipefail

BUILD_DIR="build"
BIN="${BUILD_DIR}/tests/redis_tests"

if [[ ! -f "$BIN" ]]; then
    echo "❌ Binário não encontrado! Rode: cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug && cmake --build build"
    exit 1
fi

echo "🔬 Executando testes com sanitizers ativos..."
ASAN_OPTIONS="detect_leaks=1:strict_string_checks=1" \
UBSAN_OPTIONS="print_stacktrace=1" \
"$BIN"

echo "✅ Nenhum erro detectado pelos sanitizers!"
