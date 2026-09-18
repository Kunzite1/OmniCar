#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
BUILD_TYPE="${1:-Debug}"
[[ "$BUILD_TYPE" == Debug || "$BUILD_TYPE" == Release ]] || { echo "Usage: $0 [Debug|Release] [--no-build] [--dry-run]" >&2; exit 2; }
shift || true
NO_BUILD=0
DRY_RUN=0
while (($#)); do
    case "$1" in
        --no-build) NO_BUILD=1 ;;
        --dry-run) DRY_RUN=1 ;;
        *) echo "Unknown argument: $1" >&2; exit 2 ;;
    esac
    shift
done
command -v openocd >/dev/null || { echo "Required tool not found: openocd" >&2; exit 1; }
ELF="$SCRIPT_DIR/build/$BUILD_TYPE/stm32f103rct6_proj.elf"
if (( ! NO_BUILD )) && [[ ! -f "$ELF" ]]; then "$SCRIPT_DIR/32build.sh" "$BUILD_TYPE"; fi
[[ -f "$ELF" ]] || { echo "ELF not found: $ELF" >&2; exit 1; }
OPENOCD_ELF="$ELF"
case "$(uname -s)" in MINGW*|MSYS*|CYGWIN*) OPENOCD_ELF="$(cygpath -m "$ELF")" ;; esac
CMD=(openocd -f interface/stlink.cfg -f target/stm32f1x.cfg -c "program $OPENOCD_ELF verify reset exit")
printf 'OpenOCD command:'; printf ' %q' "${CMD[@]}"; printf '\n'
if (( ! DRY_RUN )); then "${CMD[@]}"; fi
