#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
BUILD_TYPE="${1:-Debug}"
[[ "$BUILD_TYPE" == Debug || "$BUILD_TYPE" == Release ]] || { echo "Usage: $0 [Debug|Release] [--clean] [--jobs N]" >&2; exit 2; }
shift || true
CLEAN=0
JOBS=""
while (($#)); do
    case "$1" in
        --clean) CLEAN=1 ;;
        --jobs) (($# >= 2)) || exit 2; JOBS="$2"; shift ;;
        *) echo "Unknown argument: $1" >&2; exit 2 ;;
    esac
    shift
done
for tool in cmake ninja arm-none-eabi-gcc arm-none-eabi-objcopy arm-none-eabi-size; do
    command -v "$tool" >/dev/null || { echo "Required tool not found: $tool" >&2; exit 1; }
done
BUILD_DIR="$SCRIPT_DIR/build/$BUILD_TYPE"
if ((CLEAN)); then cmake --build "$BUILD_DIR" --target clean 2>/dev/null || true; fi
cmake -S "$SCRIPT_DIR" -B "$BUILD_DIR" -G Ninja -DCMAKE_TOOLCHAIN_FILE="$SCRIPT_DIR/cmake/gcc-arm-none-eabi.cmake" -DCMAKE_BUILD_TYPE="$BUILD_TYPE"
if [[ -n "$JOBS" ]]; then cmake --build "$BUILD_DIR" --parallel "$JOBS"; else cmake --build "$BUILD_DIR" --parallel; fi
