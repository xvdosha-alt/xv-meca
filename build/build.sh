#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="${ROOT}/build/.cache"
OUT_DIR="${ROOT}/dist"
TOOLCHAIN="${ROOT}/build/cmake/mingw-w64-toolchain.cmake"

need() {
    if ! command -v "$1" >/dev/null 2>&1; then
        brew install "$2"
    fi
}

need cmake cmake
need ninja ninja
need x86_64-w64-mingw32-g++ mingw-w64
need python3 python

[[ -f "${TOOLCHAIN}" ]] || { echo "missing toolchain: ${TOOLCHAIN}"; exit 1; }

python3 "${ROOT}/scripts/patch_sdk_for_gcc.py"

cmake -S "${ROOT}/build" -B "${BUILD_DIR}" \
    -G Ninja \
    -DCMAKE_TOOLCHAIN_FILE="${TOOLCHAIN}" \
    -DCMAKE_BUILD_TYPE=Release

cmake --build "${BUILD_DIR}" --parallel "$(sysctl -n hw.ncpu 2>/dev/null || echo 4)"

EXE="${BUILD_DIR}/bin/xv_meca.exe"
[[ -f "${EXE}" ]] || exit 1

mkdir -p "${OUT_DIR}"
cp -f "${EXE}" "${OUT_DIR}/"

SIZE=$(du -h "${OUT_DIR}/xv_meca.exe" | cut -f1)
echo ""
echo "Done: ${OUT_DIR}/xv_meca.exe (${SIZE})"
