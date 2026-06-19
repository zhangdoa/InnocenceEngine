#!/usr/bin/env bash
# lldb-batch.sh — run lldb in batch on the native build.
# Discovers lldb and the python311.dll directory (lldb links it; missing → exit 53 / 0xC06D007E).
#
# Usage: lldb-batch.sh <commands-file> <exe> [-- run-args...]
# Exit: lldb's exit code; 2 on setup error.

set -u

[[ $# -ge 2 ]] || { echo "usage: $0 <commands-file> <exe> [-- run-args...]" >&2; exit 2; }
cmds="$1"; exe="$2"; shift 2
[[ -f "$cmds" ]] || { echo "no commands file: $cmds" >&2; exit 2; }

# discover lldb: PATH first, then common LLVM installs
lldb="$(command -v lldb 2>/dev/null || command -v lldb.exe 2>/dev/null || true)"
if [[ -z "$lldb" ]]; then
  for c in "/c/Program Files/LLVM/bin/lldb.exe" "/c/Program Files (x86)/LLVM/bin/lldb.exe"; do
    [[ -x "$c" ]] && { lldb="$c"; break; }
  done
fi
[[ -n "$lldb" ]] || { echo "lldb not found (PATH or LLVM install)" >&2; exit 2; }

# discover the directory holding python311.dll
pydir=""
for c in /c/Python311 "${LOCALAPPDATA:-}/Programs/Python/Python311" "/c/Program Files/Python311"; do
  [[ -n "$c" && -f "$c/python311.dll" ]] && { pydir="$c"; break; }
done
[[ -z "$pydir" ]] && command -v python >/dev/null 2>&1 && {
  p="$(python -c 'import sys,os;print(os.path.dirname(sys.executable))' 2>/dev/null)"
  [[ -n "$p" && -f "$p/python311.dll" ]] && pydir="$p"
}
[[ -n "$pydir" ]] && export PATH="$pydir:$PATH"

exec "$lldb" --batch -s "$cmds" "$exe" "$@"
