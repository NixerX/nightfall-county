#!/usr/bin/env bash
# Builds Nightfall County on Debian/Ubuntu. Run: ./build.sh  then  ./build/nightfall
set -euo pipefail
cd "$(dirname "$0")"

if command -v apt-get >/dev/null; then
  need=()
  for p in build-essential cmake git libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev libgl1-mesa-dev; do
    dpkg -s "$p" >/dev/null 2>&1 || need+=("$p")
  done
  if [ ${#need[@]} -gt 0 ]; then
    echo "Installing: ${need[*]}"
    sudo apt-get install -y "${need[@]}"
  fi
fi

cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j"$(nproc)"
echo
echo "Done. Run: ./build/nightfall"
