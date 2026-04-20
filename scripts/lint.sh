#!/usr/bin/env bash
set -euo pipefail

if [[ ! -f build/compile_commands.json ]]; then
  cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
fi

mapfile -t format_files < <(find include src tests examples -type f \( -name "*.h" -o -name "*.hpp" -o -name "*.c" -o -name "*.cc" -o -name "*.cpp" \))
if [[ ${#format_files[@]} -gt 0 ]]; then
  clang-format --dry-run --Werror "${format_files[@]}"
fi

mapfile -t tidy_files < <(find src tests examples -type f \( -name "*.cc" -o -name "*.cpp" \))
if [[ ${#tidy_files[@]} -gt 0 ]]; then
  clang-tidy -p build "${tidy_files[@]}"
fi
