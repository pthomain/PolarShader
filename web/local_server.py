#!/usr/bin/env python3

from __future__ import annotations

import http.server
import json
from pathlib import Path
import re
import socketserver
import sys
from urllib.parse import parse_qs, unquote, urlparse


MAX_PSC_BYTES = 1024 * 1024
SAFE_NAME_RE = re.compile(r"^[A-Za-z0-9][A-Za-z0-9._ -]*$")


def _json_bytes(payload: dict) -> bytes:
    return json.dumps(payload, separators=(",", ":")).encode("utf-8")


def _validate_psc(data: bytes) -> str | None:
    if len(data) < 6:
        return f".psc file is too short ({len(data)} bytes)"
    if data[:4] != b"PSC\0":
        return "bad .psc magic"
    if data[4] != 0:
        return f"unsupported .psc version {data[4]}"
    return None


def _playlist_name(raw: str, playlist_dir: Path) -> tuple[str, Path]:
    name = unquote(raw).strip()
    if not name:
        raise ValueError("missing playlist filename")

    candidate = Path(name)
    if candidate.is_absolute() or any(part in {"", ".", ".."} for part in candidate.parts):
        raise ValueError("playlist filename must be relative")
    if any(not SAFE_NAME_RE.fullmatch(part) for part in candidate.parts):
        raise ValueError("playlist filename contains unsupported characters")
    if candidate.suffix.lower() != ".psc":
        raise ValueError("playlist filename must end in .psc")

    resolved = (playlist_dir / candidate).resolve()
    try:
        resolved.relative_to(playlist_dir)
    except ValueError as exc:
        raise ValueError("playlist filename escapes build/psc") from exc

    return candidate.as_posix(), resolved


class LocalComposerServer(http.server.SimpleHTTPRequestHandler):
    repo_root: Path
    playlist_dir: Path

    def end_headers(self) -> None:
        self.send_header("Cross-Origin-Opener-Policy", "same-origin")
        self.send_header("Cross-Origin-Embedder-Policy", "require-corp")
        self.send_header("Cache-Control", "no-store")
        super().end_headers()

    def _send_json(self, status: int, payload: dict) -> None:
        data = _json_bytes(payload)
        self.send_response(status)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(data)))
        self.end_headers()
        self.wfile.write(data)

    def _send_error_json(self, status: int, message: str) -> None:
        self._send_json(status, {"ok": False, "error": message})

    def _query_name(self) -> tuple[str, Path]:
        parsed = urlparse(self.path)
        raw = parse_qs(parsed.query).get("name", [""])[0]
        return _playlist_name(raw, self.playlist_dir)

    def do_GET(self) -> None:
        parsed = urlparse(self.path)
        if parsed.path == "/api/capabilities":
            self._send_json(200, {
                "ok": True,
                "savePsc": True,
                "playlist": True,
                "playlistDir": "build/psc",
            })
            return

        if parsed.path == "/api/playlist":
            files = []
            if self.playlist_dir.is_dir():
                for path in sorted(self.playlist_dir.rglob("*.psc")):
                    rel = path.relative_to(self.playlist_dir).as_posix()
                    files.append({
                        "name": rel,
                        "size": path.stat().st_size,
                        "mtime": int(path.stat().st_mtime),
                    })
            self._send_json(200, {"ok": True, "files": files})
            return

        if parsed.path == "/api/playlist/file":
            try:
                name, path = self._query_name()
                data = path.read_bytes()
            except OSError:
                self._send_error_json(404, "playlist file not found")
                return
            except ValueError as exc:
                self._send_error_json(400, str(exc))
                return

            self.send_response(200)
            self.send_header("Content-Type", "application/octet-stream")
            self.send_header("Content-Disposition", f'attachment; filename="{Path(name).name}"')
            self.send_header("Content-Length", str(len(data)))
            self.end_headers()
            self.wfile.write(data)
            return

        super().do_GET()

    def do_POST(self) -> None:
        parsed = urlparse(self.path)
        if parsed.path != "/api/playlist/save":
            self._send_error_json(404, "unknown API endpoint")
            return

        try:
            name, path = self._query_name()
        except ValueError as exc:
            self._send_error_json(400, str(exc))
            return

        length_raw = self.headers.get("Content-Length")
        try:
            length = int(length_raw or "-1")
        except ValueError:
            self._send_error_json(411, "invalid Content-Length")
            return

        if length < 0:
            self._send_error_json(411, "missing Content-Length")
            return
        if length > MAX_PSC_BYTES:
            self._send_error_json(413, f".psc file is too large ({length} bytes)")
            return

        data = self.rfile.read(length)
        error = _validate_psc(data)
        if error:
            self._send_error_json(400, error)
            return

        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(data)
        self._send_json(200, {
            "ok": True,
            "file": {
                "name": name,
                "size": len(data),
                "mtime": int(path.stat().st_mtime),
            },
        })


def main() -> None:
    if len(sys.argv) != 4:
        raise SystemExit("usage: local_server.py PORT DIST_DIR REPO_ROOT")

    port = int(sys.argv[1])
    directory = Path(sys.argv[2]).resolve()
    repo_root = Path(sys.argv[3]).resolve()

    LocalComposerServer.repo_root = repo_root
    LocalComposerServer.playlist_dir = (repo_root / "build" / "psc").resolve()
    socketserver.TCPServer.allow_reuse_address = True
    with socketserver.TCPServer(
        ("127.0.0.1", port),
        lambda *a: LocalComposerServer(*a, directory=directory),
    ) as server:
        server.serve_forever()


if __name__ == "__main__":
    main()
