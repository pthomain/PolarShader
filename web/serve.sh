#!/usr/bin/env bash
#
# Rebuild the web demo into web/dist, then serve it locally with the
# cross-origin isolation headers (COOP/COEP) that the pthreads WASM build needs
# for SharedArrayBuffer.
#
# Usage:
#   web/serve.sh [PORT]            # rebuild, then serve on PORT (default 8000)
#   web/serve.sh --no-build [PORT] # serve the current web/dist without rebuilding
#
# Environment:
#   PYTHON=/path/to/python                 # optional; defaults to .venv/bin/python or python3
#   POLARSHADER_SKETCHES_OVERRIDE=composer # optional; defaults to composer

set -euo pipefail

# Resolve paths relative to this script so it works from any CWD.
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
DIST_DIR="$SCRIPT_DIR/dist"
BUILD=1

if [[ "${1:-}" == "--no-build" ]]; then
    BUILD=0
    shift
fi

PORT="${1:-8000}"

if [[ "$BUILD" -eq 1 ]]; then
    if [[ -n "${PYTHON:-}" ]]; then
        PYTHON_BIN="$PYTHON"
    elif [[ -x "$REPO_ROOT/.venv/bin/python" ]]; then
        PYTHON_BIN="$REPO_ROOT/.venv/bin/python"
    else
        PYTHON_BIN="python3"
    fi

    export POLARSHADER_SKETCHES_OVERRIDE="${POLARSHADER_SKETCHES_OVERRIDE:-composer}"

    echo "Rebuilding $DIST_DIR with $PYTHON_BIN ..."
    "$PYTHON_BIN" "$SCRIPT_DIR/build_site.py"
    touch "$DIST_DIR/.nojekyll"
fi

if [[ ! -f "$DIST_DIR/index.html" ]]; then
    echo "error: $DIST_DIR/index.html not found; run without --no-build first:" >&2
    echo "  $SCRIPT_DIR/serve.sh $PORT" >&2
    exit 1
fi

echo "Serving $DIST_DIR at http://localhost:$PORT/index.html (COOP/COEP enabled, Ctrl+C to stop)"

exec python3 -c "
import sys, http.server, socketserver
port, directory = int(sys.argv[1]), sys.argv[2]
class H(http.server.SimpleHTTPRequestHandler):
    def end_headers(self):
        self.send_header('Cross-Origin-Opener-Policy', 'same-origin')
        self.send_header('Cross-Origin-Embedder-Policy', 'require-corp')
        self.send_header('Cache-Control', 'no-store')
        super().end_headers()
socketserver.TCPServer.allow_reuse_address = True
socketserver.TCPServer(('', port), lambda *a: H(*a, directory=directory)).serve_forever()
" "$PORT" "$DIST_DIR"
