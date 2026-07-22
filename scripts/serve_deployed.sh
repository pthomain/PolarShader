#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Reproduce a live GitHub Pages composer failure *locally, same-origin*.
#
# The deployed composer hides its body until JS reveals it and boots a WASM
# module, so a broken deploy shows a blank page whose real error is only in the
# browser console. You cannot inspect it by curling the assets (they all return
# 200). This script mirrors the exact deployed bundle into a temp dir and serves
# it over http://localhost so you can open it in a real browser (or point a
# headless runner at it) and read the console.
#
# Usage:
#   scripts/serve_deployed.sh                 # mirror the default deployment
#   scripts/serve_deployed.sh <base-url>      # mirror a specific composer URL
#   PORT=8123 scripts/serve_deployed.sh       # override the local port
#
# Then open the printed http://localhost:PORT/ and check the devtools console.

set -euo pipefail

BASE="${1:-https://pthomain.github.io/PolarShader/composer}"
BASE="${BASE%/}"
PORT="${PORT:-8110}"

DEST="$(mktemp -d "${TMPDIR:-/tmp}/composer_mirror.XXXXXX")"

# The core files the composer shell needs. Modules are discovered from the
# index.html imports below; this is the fixed set the page always loads.
FILES=(
  index.html index.css app.js index.js
  fastled.js fastled.wasm files.json
  composer.css composer.js schema.js codec.js pds-codec.js
  stepper.js signal-editor.js boot-diagnostics.js
)

echo "Mirroring $BASE -> $DEST"
for f in "${FILES[@]}"; do
  code="$(curl -fsS -o "$DEST/$f" -w '%{http_code}' "$BASE/$f" 2>/dev/null || echo 000)"
  printf '  %s  %s\n' "$code" "$f"
done

echo
echo "Serving $DEST at http://localhost:$PORT/index.html"
echo "Open it in a browser and read the console (Cmd+Opt+J / Ctrl+Shift+J)."
echo "Ctrl+C to stop."
cd "$DEST"
exec python3 -m http.server "$PORT"
