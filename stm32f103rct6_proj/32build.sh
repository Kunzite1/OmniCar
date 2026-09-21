#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
BUILD_TYPE="Debug"
CLEAN=0
JOBS=""

usage() {
    echo "Usage: $0 [Debug|Release] [--clean] [--jobs N]" >&2
}

if (($#)) && [[ "$1" == "Debug" || "$1" == "Release" ]]; then
    BUILD_TYPE="$1"
    shift
fi

while (($#)); do
    case "$1" in
        --clean) CLEAN=1 ;;
        --jobs)
            (($# >= 2)) || { usage; exit 2; }
            [[ "$2" =~ ^[1-9][0-9]*$ ]] || { echo "Invalid job count: $2" >&2; exit 2; }
            JOBS="$2"
            shift
            ;;
        -h|--help) usage; exit 0 ;;
        *) echo "Unknown argument: $1" >&2; usage; exit 2 ;;
    esac
    shift
done

for tool in cmake ninja arm-none-eabi-gcc arm-none-eabi-objcopy arm-none-eabi-size; do
    command -v "$tool" >/dev/null || { echo "Required tool not found: $tool" >&2; exit 1; }
done

BUILD_DIR="$SCRIPT_DIR/build/$BUILD_TYPE"
if ((CLEAN)) && [[ -f "$BUILD_DIR/build.ninja" ]]; then
    cmake --build "$BUILD_DIR" --target clean
fi

cmake -S "$SCRIPT_DIR" -B "$BUILD_DIR" -G Ninja -DCMAKE_TOOLCHAIN_FILE="$SCRIPT_DIR/cmake/gcc-arm-none-eabi.cmake" -DCMAKE_BUILD_TYPE="$BUILD_TYPE"
if [[ -n "$JOBS" ]]; then
    cmake --build "$BUILD_DIR" --parallel "$JOBS"
else
    cmake --build "$BUILD_DIR" --parallel
fi

ELF="$BUILD_DIR/stm32f103rct6_proj.elf"
[[ -f "$ELF" ]] || { echo "Build finished but ELF is missing: $ELF" >&2; exit 1; }
echo "Build artifact: $ELF"
