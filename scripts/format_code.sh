#!/usr/bin/env bash
set -euo pipefail

echo "🎨 Formatando código..."
find src include tests -type f \( -name "*.cpp" -o -name "*.hpp" \) \
    -exec clang-format -i {} +

echo "✅ Código formatado com sucesso!"
