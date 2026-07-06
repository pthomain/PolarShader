#!/usr/bin/env python3

from __future__ import annotations

import http.server
import codecs
import json
import os
from pathlib import Path
import re
import shutil
import signal
import socketserver
import subprocess
import sys
import threading
import time
from urllib.parse import parse_qs, unquote, urlparse
import uuid


MAX_PSC_BYTES = 1024 * 1024
MAX_JSON_BYTES = 64 * 1024
DEPLOY_TIMEOUT_SECONDS = 10 * 60
DEVICE_LIST_TIMEOUT_SECONDS = 8
PROCESS_TERM_GRACE_SECONDS = 5
JOB_HISTORY_LIMIT = 10
SAFE_NAME_RE = re.compile(r"^[A-Za-z0-9][A-Za-z0-9._ -]*$")
DEFAULT_DISPLAY_BRIGHTNESS = 255

DEPLOY_TARGETS = [
    {
        "id": "seeed_xiao",
        "label": "Seeeduino XIAO (SAMD21)",
        "matches": ["seeed_xiao", "samd21", "atsamd21"],
    },
    {
        "id": "seeed_xiao_rp2040_fabric",
        "label": "Seeed XIAO RP2040 Fabric",
        "matches": ["rp2040", "xiao rp2040", "seeed xiao rp2040"],
    },
    {
        "id": "seeed_xiao_rp2040_round",
        "label": "Seeed XIAO RP2040 Round",
        "matches": ["rp2040", "xiao rp2040", "seeed xiao rp2040"],
    },
    {
        "id": "teensy41_matrix",
        "label": "Teensy 4.1 Matrix",
        "matches": ["teensy", "teensyduino", "imxrt1062"],
    },
]
DEPLOY_TARGET_IDS = {target["id"] for target in DEPLOY_TARGETS}


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


def _coerce_display_brightness(value) -> int:
    if isinstance(value, bool) or not isinstance(value, int):
        raise ValueError("brightness must be an integer")
    if value < 0 or value > 255:
        raise ValueError("brightness must be between 0 and 255")
    return value


def _read_display_config(path: Path) -> tuple[int, bool, str | None]:
    if not path.is_file():
        return DEFAULT_DISPLAY_BRIGHTNESS, False, None
    try:
        body = json.loads(path.read_text(encoding="utf-8"))
    except json.JSONDecodeError:
        return DEFAULT_DISPLAY_BRIGHTNESS, False, "ignoring display config: invalid JSON"
    except OSError as exc:
        return DEFAULT_DISPLAY_BRIGHTNESS, False, f"ignoring display config: {exc}"
    if not isinstance(body, dict):
        return DEFAULT_DISPLAY_BRIGHTNESS, False, "ignoring display config: display config must be a JSON object"
    try:
        return _coerce_display_brightness(body.get("brightness")), True, None
    except ValueError as exc:
        return DEFAULT_DISPLAY_BRIGHTNESS, False, f"ignoring display config: {exc}"


def _write_display_config(path: Path, brightness: int) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(
        json.dumps({"brightness": brightness}, separators=(",", ":")) + "\n",
        encoding="utf-8",
    )


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


def _deduplicated_playlist_path(name: str, path: Path, playlist_dir: Path) -> tuple[str, Path]:
    if not path.exists():
        return name, path

    requested = Path(name)
    parent = requested.parent
    stem = requested.stem
    suffix = requested.suffix
    for index in range(1, 10000):
        candidate = parent / f"{stem}-{index}{suffix}"
        candidate_name, candidate_path = _playlist_name(candidate.as_posix(), playlist_dir)
        if not candidate_path.exists():
            return candidate_name, candidate_path
    raise ValueError("could not allocate a unique playlist filename")


def _duration_label(seconds: int | float) -> str:
    if seconds % 60 == 0:
        return f"{int(seconds // 60)}m"
    return f"{seconds:g}s"


class ApiError(Exception):
    def __init__(self, status: int, message: str):
        super().__init__(message)
        self.status = status
        self.message = message


class DeployJob:
    def __init__(self, job_id: str, env: str):
        now = time.time()
        self.id = job_id
        self.env = env
        self.status = "starting"
        self.logs = ""
        self.exit_code: int | None = None
        self.error: str | None = None
        self.created_at = now
        self.started_at: float | None = None
        self.ended_at: float | None = None
        self.lock = threading.Lock()

    def append_log(self, text: str) -> None:
        if not text:
            return
        with self.lock:
            self.logs += text

    def update(self, **fields) -> None:
        with self.lock:
            for key, value in fields.items():
                setattr(self, key, value)

    def snapshot(self) -> dict:
        with self.lock:
            return {
                "id": self.id,
                "env": self.env,
                "status": self.status,
                "logs": self.logs,
                "exitCode": self.exit_code,
                "error": self.error,
                "createdAt": self.created_at,
                "startedAt": self.started_at,
                "endedAt": self.ended_at,
            }


class DeployManager:
    def __init__(
        self,
        repo_root: Path,
        platformio_python: str,
        *,
        deploy_timeout: int = DEPLOY_TIMEOUT_SECONDS,
        platformio_base_override: list[str] | None = None,
    ):
        self.repo_root = repo_root
        self.platformio_python = platformio_python
        self.deploy_timeout = deploy_timeout
        self.platformio_base_override = platformio_base_override
        self.deploy_lock = threading.Lock()
        self.deploy_lock_state_lock = threading.Lock()
        self.deploy_lock_holder: str | None = None
        self.jobs_lock = threading.Lock()
        self.jobs: dict[str, DeployJob] = {}
        self.job_order: list[str] = []
        self.active_proc_lock = threading.Lock()
        self.active_proc: subprocess.Popen | None = None

    def is_deploy_active(self) -> bool:
        return self.deploy_lock.locked()

    def _busy_message(self) -> str:
        if self.deploy_lock_holder == "device_detection":
            return "device detection in progress"
        if self.deploy_lock_holder == "build_input":
            return "build input update in progress"
        return "deploy already in progress"

    def _acquire_operation_lock(self, holder: str) -> None:
        with self.deploy_lock_state_lock:
            if not self.deploy_lock.acquire(blocking=False):
                raise ApiError(409, self._busy_message())
            self.deploy_lock_holder = holder

    def _release_operation_lock(self) -> None:
        with self.deploy_lock_state_lock:
            self.deploy_lock_holder = None
            self.deploy_lock.release()

    def targets(self) -> list[dict]:
        return [dict(target) for target in DEPLOY_TARGETS]

    def _remember_job(self, job: DeployJob) -> None:
        with self.jobs_lock:
            self.jobs[job.id] = job
            self.job_order.append(job.id)
            while len(self.job_order) > JOB_HISTORY_LIMIT:
                old_id = self.job_order.pop(0)
                self.jobs.pop(old_id, None)

    def job_snapshot(self, job_id: str) -> dict:
        with self.jobs_lock:
            job = self.jobs.get(job_id)
        if not job:
            raise ApiError(404, "deploy job not found")
        return job.snapshot()

    def _probe_platformio(self, base: list[str]) -> tuple[bool, str]:
        try:
            proc = subprocess.run(
                [*base, "--version"],
                cwd=self.repo_root,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
                timeout=15,
                check=False,
            )
        except (OSError, subprocess.TimeoutExpired) as exc:
            return False, str(exc)
        if proc.returncode == 0:
            return True, proc.stdout.strip()
        return False, proc.stdout.strip() or f"exit code {proc.returncode}"

    def _platformio_base_cmd(self) -> list[str]:
        if self.platformio_base_override is not None:
            return list(self.platformio_base_override)

        candidates: list[list[str]] = []
        if self.platformio_python:
            candidates.append([self.platformio_python, "-m", "platformio"])
        for name in ("pio", "platformio"):
            resolved = shutil.which(name)
            if resolved:
                candidates.append([resolved])

        errors = []
        for candidate in candidates:
            ok, detail = self._probe_platformio(candidate)
            if ok:
                return candidate
            errors.append(f"{' '.join(candidate)}: {detail}")

        detail = "; ".join(errors) if errors else "no pio/platformio executable found"
        raise ApiError(500, f"PlatformIO is not available ({detail})")

    def _platformio_argv(self, args: list[str]) -> list[str]:
        return [*self._platformio_base_cmd(), *args]

    def list_devices(self) -> dict:
        self._acquire_operation_lock("device_detection")
        try:
            argv = self._platformio_argv(["device", "list", "--json-output"])
            try:
                proc = subprocess.run(
                    argv,
                    cwd=self.repo_root,
                    stdout=subprocess.PIPE,
                    stderr=subprocess.STDOUT,
                    text=True,
                    timeout=DEVICE_LIST_TIMEOUT_SECONDS,
                    check=False,
                )
            except subprocess.TimeoutExpired:
                return {"devices": [], "error": "device detection timed out"}
            except OSError as exc:
                return {"devices": [], "error": str(exc)}

            if proc.returncode != 0:
                return {
                    "devices": [],
                    "error": proc.stdout.strip() or f"device detection failed with exit code {proc.returncode}",
                }

            try:
                devices = json.loads(proc.stdout or "[]")
            except json.JSONDecodeError as exc:
                return {"devices": [], "error": f"device detection returned invalid JSON: {exc}"}
            if not isinstance(devices, list):
                return {"devices": [], "error": "device detection returned unexpected JSON"}
            return {"devices": devices}
        finally:
            self._release_operation_lock()

    def acquire_build_input_lock(self) -> None:
        self._acquire_operation_lock("build_input")

    def release_build_input_lock(self) -> None:
        self._release_operation_lock()

    def start_deploy(self, env: str) -> dict:
        if env not in DEPLOY_TARGET_IDS:
            raise ApiError(400, "unknown deploy target")
        self._acquire_operation_lock("deploy")

        job = DeployJob(uuid.uuid4().hex, env)
        try:
            self._remember_job(job)
            thread = threading.Thread(target=self._run_deploy, args=(job, True), daemon=True)
            thread.start()
        except Exception as exc:
            job.update(status="failed", error=str(exc), ended_at=time.time())
            self._release_operation_lock()
            raise ApiError(500, f"failed to start deploy: {exc}") from exc

        return job.snapshot()

    def _read_process_output(self, job: DeployJob, proc: subprocess.Popen) -> None:
        stream = proc.stdout
        if stream is None:
            return
        decoder = codecs.getincrementaldecoder("utf-8")("replace")
        try:
            while True:
                chunk = os.read(stream.fileno(), 4096)
                if not chunk:
                    break
                job.append_log(decoder.decode(chunk))
            tail = decoder.decode(b"", final=True)
            if tail:
                job.append_log(tail)
        except Exception as exc:
            job.append_log(f"\n[deploy log reader failed: {exc}]\n")
        finally:
            stream.close()

    def _kill_process_group(self, proc: subprocess.Popen) -> None:
        if proc.poll() is not None:
            return
        try:
            pgid = os.getpgid(proc.pid)
        except ProcessLookupError:
            return

        try:
            os.killpg(pgid, signal.SIGTERM)
        except ProcessLookupError:
            return
        try:
            proc.wait(timeout=PROCESS_TERM_GRACE_SECONDS)
            return
        except subprocess.TimeoutExpired:
            pass

        try:
            os.killpg(pgid, signal.SIGKILL)
        except ProcessLookupError:
            return
        try:
            proc.wait(timeout=PROCESS_TERM_GRACE_SECONDS)
        except subprocess.TimeoutExpired:
            return

    def _run_deploy(self, job: DeployJob, release_deploy_lock: bool) -> None:
        proc: subprocess.Popen | None = None
        try:
            argv = self._platformio_argv(["run", "-e", job.env, "-t", "upload"])
            job.update(status="running", started_at=time.time())
            job.append_log(f"$ {' '.join(argv)}\n")
            proc = subprocess.Popen(
                argv,
                cwd=self.repo_root,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                bufsize=0,
                shell=False,
                start_new_session=True,
            )
            with self.active_proc_lock:
                self.active_proc = proc

            reader = threading.Thread(target=self._read_process_output, args=(job, proc), daemon=True)
            reader.start()

            try:
                exit_code = proc.wait(timeout=self.deploy_timeout)
            except subprocess.TimeoutExpired:
                message = f"timed out after {_duration_label(self.deploy_timeout)}"
                job.update(status="timed_out", error=message)
                job.append_log(f"\n[deploy {message}; terminating process group]\n")
                self._kill_process_group(proc)
                exit_code = proc.poll()
                if exit_code is None:
                    exit_code = -signal.SIGKILL

            reader.join(timeout=2)
            if job.snapshot()["status"] == "timed_out":
                job.update(exit_code=exit_code, ended_at=time.time())
            elif exit_code == 0:
                job.update(status="succeeded", exit_code=0, ended_at=time.time())
            else:
                job.update(
                    status="failed",
                    exit_code=exit_code,
                    error=f"PlatformIO upload failed with exit code {exit_code}",
                    ended_at=time.time(),
                )
        except ApiError as exc:
            job.update(status="failed", error=exc.message, ended_at=time.time())
            job.append_log(f"\n[{exc.message}]\n")
        except Exception as exc:
            job.update(status="failed", error=str(exc), ended_at=time.time())
            job.append_log(f"\n[deploy failed: {exc}]\n")
        finally:
            with self.active_proc_lock:
                if self.active_proc is proc:
                    self.active_proc = None
            if release_deploy_lock:
                self._release_operation_lock()

    def shutdown(self) -> None:
        with self.active_proc_lock:
            proc = self.active_proc
        if proc and proc.poll() is None:
            self._kill_process_group(proc)


class ThreadedLocalServer(socketserver.ThreadingMixIn, socketserver.TCPServer):
    allow_reuse_address = True
    daemon_threads = True


class LocalComposerServer(http.server.SimpleHTTPRequestHandler):
    repo_root: Path
    playlist_dir: Path
    display_config_path: Path
    deploy_manager: DeployManager

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

    def _api_allowed_hosts(self) -> set[str]:
        port = self.server.server_address[1]
        return {f"127.0.0.1:{port}", f"localhost:{port}"}

    def _drain_request_body(self) -> None:
        length_raw = self.headers.get("Content-Length")
        try:
            remaining = int(length_raw or "0")
        except ValueError:
            return
        while remaining > 0:
            chunk = self.rfile.read(min(remaining, 64 * 1024))
            if not chunk:
                return
            remaining -= len(chunk)

    def _reject_api_request(self, status: int, message: str, *, drain: bool) -> bool:
        if drain:
            self._drain_request_body()
        self._send_error_json(status, message)
        return False

    def _guard_api_request(self, *, drain_on_reject: bool = False) -> bool:
        allowed_hosts = self._api_allowed_hosts()
        host = (self.headers.get("Host") or "").lower()
        if host not in allowed_hosts:
            return self._reject_api_request(403, "forbidden API host", drain=drain_on_reject)

        origin = self.headers.get("Origin")
        if origin:
            parsed = urlparse(origin)
            if parsed.scheme != "http" or parsed.netloc.lower() not in allowed_hosts:
                return self._reject_api_request(403, "forbidden API origin", drain=drain_on_reject)

        return True

    def _query_name(self) -> tuple[str, Path]:
        parsed = urlparse(self.path)
        raw = parse_qs(parsed.query).get("name", [""])[0]
        return _playlist_name(raw, self.playlist_dir)

    def _read_json_body(self) -> dict:
        length_raw = self.headers.get("Content-Length")
        try:
            length = int(length_raw or "-1")
        except ValueError as exc:
            self._drain_request_body()
            raise ApiError(411, "invalid Content-Length") from exc
        if length < 0:
            raise ApiError(411, "missing Content-Length")
        if length > MAX_JSON_BYTES:
            self._drain_request_body()
            raise ApiError(413, "request body is too large")

        data = self.rfile.read(length)
        try:
            body = json.loads(data.decode("utf-8") or "{}")
        except (UnicodeDecodeError, json.JSONDecodeError) as exc:
            raise ApiError(400, "invalid JSON body") from exc
        if not isinstance(body, dict):
            raise ApiError(400, "JSON body must be an object")
        return body

    def do_GET(self) -> None:
        parsed = urlparse(self.path)
        if parsed.path.startswith("/api/") and not self._guard_api_request():
            return

        if parsed.path == "/api/capabilities":
            self._send_json(200, {
                "ok": True,
                "savePsc": True,
                "playlist": True,
                "deploy": True,
                "displayConfig": True,
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

        if parsed.path == "/api/display/config":
            brightness, saved, warning = _read_display_config(self.display_config_path)
            payload = {
                "ok": True,
                "brightness": brightness,
                "saved": saved,
                "min": 0,
                "max": 255,
            }
            if warning:
                payload["warning"] = warning
            self._send_json(200, payload)
            return

        if parsed.path == "/api/deploy/targets":
            self._send_json(200, {"ok": True, "targets": self.deploy_manager.targets()})
            return

        if parsed.path == "/api/deploy/status":
            job_id = parse_qs(parsed.query).get("id", [""])[0]
            if not job_id:
                self._send_error_json(400, "missing deploy job id")
                return
            try:
                job = self.deploy_manager.job_snapshot(job_id)
            except ApiError as exc:
                self._send_error_json(exc.status, exc.message)
                return
            self._send_json(200, {"ok": True, "job": job})
            return

        if parsed.path.startswith("/api/"):
            self._send_error_json(404, "unknown API endpoint")
            return

        super().do_GET()

    def do_POST(self) -> None:
        parsed = urlparse(self.path)
        if parsed.path.startswith("/api/") and not self._guard_api_request(drain_on_reject=True):
            return

        if parsed.path == "/api/deploy/devices":
            self._drain_request_body()
            try:
                result = self.deploy_manager.list_devices()
            except ApiError as exc:
                self._send_error_json(exc.status, exc.message)
                return
            self._send_json(200, {"ok": True, **result})
            return

        if parsed.path == "/api/deploy":
            try:
                body = self._read_json_body()
                env = body.get("env")
                if not isinstance(env, str):
                    raise ApiError(400, "missing deploy target")
                job = self.deploy_manager.start_deploy(env)
            except ApiError as exc:
                self._send_error_json(exc.status, exc.message)
                return
            self._send_json(202, {"ok": True, "job": job})
            return

        if parsed.path == "/api/display/config":
            try:
                body = self._read_json_body()
                brightness = _coerce_display_brightness(body.get("brightness"))
                self.deploy_manager.acquire_build_input_lock()
                try:
                    _write_display_config(self.display_config_path, brightness)
                finally:
                    self.deploy_manager.release_build_input_lock()
            except ValueError as exc:
                self._send_error_json(400, str(exc))
                return
            except OSError as exc:
                self._send_error_json(500, f"failed to save display config: {exc}")
                return
            except ApiError as exc:
                self._send_error_json(exc.status, exc.message)
                return
            self._send_json(200, {"ok": True, "brightness": brightness, "saved": True})
            return

        if parsed.path == "/api/playlist/delete":
            self._drain_request_body()
            try:
                name, path = self._query_name()
            except ValueError as exc:
                self._send_error_json(400, str(exc))
                return

            try:
                self.deploy_manager.acquire_build_input_lock()
                try:
                    path.unlink()
                finally:
                    self.deploy_manager.release_build_input_lock()
            except FileNotFoundError:
                self._send_error_json(404, "playlist file not found")
                return
            except IsADirectoryError:
                self._send_error_json(400, "playlist path is not a file")
                return
            except OSError as exc:
                self._send_error_json(500, f"failed to delete playlist file: {exc}")
                return
            except ApiError as exc:
                self._send_error_json(exc.status, exc.message)
                return

            self._send_json(200, {"ok": True, "file": {"name": name}})
            return

        if parsed.path != "/api/playlist/save":
            self._reject_api_request(404, "unknown API endpoint", drain=True)
            return

        try:
            name, path = self._query_name()
        except ValueError as exc:
            self._reject_api_request(400, str(exc), drain=True)
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
            self._reject_api_request(413, f".psc file is too large ({length} bytes)", drain=True)
            return

        data = self.rfile.read(length)
        error = _validate_psc(data)
        if error:
            self._send_error_json(400, error)
            return

        try:
            self.deploy_manager.acquire_build_input_lock()
            try:
                name, path = _deduplicated_playlist_path(name, path, self.playlist_dir)
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_bytes(data)
                saved_size = len(data)
                saved_mtime = int(path.stat().st_mtime)
            finally:
                self.deploy_manager.release_build_input_lock()
        except ValueError as exc:
            self._send_error_json(409, str(exc))
            return
        except OSError as exc:
            self._send_error_json(500, f"failed to save playlist file: {exc}")
            return
        except ApiError as exc:
            self._send_error_json(exc.status, exc.message)
            return

        self._send_json(200, {
            "ok": True,
            "file": {
                "name": name,
                "size": saved_size,
                "mtime": saved_mtime,
            },
        })

    def do_OPTIONS(self) -> None:
        parsed = urlparse(self.path)
        if parsed.path.startswith("/api/"):
            if not self._guard_api_request(drain_on_reject=True):
                return
            self._send_error_json(405, "method not allowed")
            return
        self.send_error(501, "Unsupported method")


def main() -> None:
    if len(sys.argv) != 5:
        raise SystemExit("usage: local_server.py PORT DIST_DIR REPO_ROOT PYTHON")

    port = int(sys.argv[1])
    directory = Path(sys.argv[2]).resolve()
    repo_root = Path(sys.argv[3]).resolve()
    platformio_python = sys.argv[4]

    LocalComposerServer.repo_root = repo_root
    LocalComposerServer.playlist_dir = (repo_root / "build" / "psc").resolve()
    LocalComposerServer.display_config_path = (repo_root / "build" / "display_config.json").resolve()
    LocalComposerServer.deploy_manager = DeployManager(repo_root, platformio_python)
    with ThreadedLocalServer(
        ("127.0.0.1", port),
        lambda *a: LocalComposerServer(*a, directory=directory),
    ) as server:
        try:
            server.serve_forever()
        finally:
            LocalComposerServer.deploy_manager.shutdown()


if __name__ == "__main__":
    main()
