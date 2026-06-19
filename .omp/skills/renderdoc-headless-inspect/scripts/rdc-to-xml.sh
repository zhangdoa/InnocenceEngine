#!/usr/bin/env bash
# rdc-to-xml.sh — convert a RenderDoc capture to greppable structured XML.
#
# Usage: rdc-to-xml.sh <capture.rdc> <out.xml> [format]
#   format: xml (default; API + descriptors, no bulk bytes) | zip.xml (plus buffer/texture bytes)
# Exit: renderdoccmd's exit code; 2 on usage error.

set -u

[[ $# -ge 2 ]] || { echo "usage: $0 <capture.rdc> <out.xml> [xml|zip.xml]" >&2; exit 2; }
rdc="$1"; out="$2"; fmt="${3:-xml}"
[[ -f "$rdc" ]] || { echo "no such capture: $rdc" >&2; exit 2; }
command -v renderdoccmd >/dev/null 2>&1 || { echo "renderdoccmd not on PATH" >&2; exit 2; }

renderdoccmd convert -f "$rdc" -o "$out" -c "$fmt"
