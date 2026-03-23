#!/usr/bin/env bash
set -euo pipefail

MINGW_CC=${MINGW_CC:-x86_64-w64-mingw32-gcc}
SRC=${1:-bank_reverse_task.c}
OUT=${2:-ybank_task.exe}
CFLAGS="-std=c11 -O2 -Wall -Wextra -pedantic"
LDFLAGS=""

if [[ "$SRC" == "ybank_win_gui.c" ]]; then
  LDFLAGS="-mwindows"
fi

if ! command -v "$MINGW_CC" >/dev/null 2>&1; then
  echo "[ERROR] $MINGW_CC not found."
  echo "Install MinGW-w64 and re-run this script to produce a real Windows .exe."
  exit 1
fi

"$MINGW_CC" $CFLAGS $LDFLAGS "$SRC" -o "$OUT"
echo "[OK] Built Windows executable: $OUT"
