#!/usr/bin/env python3

from __future__ import annotations

from dataclasses import dataclass
import struct
from pathlib import Path
import os
import re
import shutil
import subprocess
import sys
import tempfile
import urllib.request
import zipfile


REPO_ROOT = Path(__file__).resolve().parents[1]
WEB_ROOT = REPO_ROOT / "web"
SOURCE_ROOT = REPO_ROOT / "src"
SKETCH_ROOT = WEB_ROOT / "sketches"
LANDING_PAGE = WEB_ROOT / "index.html"
REQUIREMENTS_PATH = WEB_ROOT / "requirements.txt"
# POLARSHADER_WORK_ROOT_OVERRIDE lets an external orchestrator reparent the
# four write roots (.stage/, .home/, .fastled/, dist/) under a directory of
# its choosing — useful when build_site.py is invoked from outside the
# PolarShader checkout and the caller wants to keep the submodule tree clean.
# Defaults to WEB_ROOT, preserving the original layout.
WORK_ROOT = Path(os.environ["POLARSHADER_WORK_ROOT_OVERRIDE"]).resolve() \
    if "POLARSHADER_WORK_ROOT_OVERRIDE" in os.environ else WEB_ROOT
STAGE_ROOT = WORK_ROOT / ".stage"
DIST_ROOT = WORK_ROOT / "dist"
HOME_ROOT = WORK_ROOT / ".home"
FASTLED_CACHE_ROOT = WORK_ROOT / ".fastled"

# Why FASTLED_REVISION is pinned to a master commit:
#   - PolarShader's pipeline (src/renderer/pipeline/maths/units/Units.h,
#     FixedPointMaths.h, etc.) uses fl::s16x16 / fl::u16x16 / fl::s24x8 /
#     fl::u24x8 unconditionally as part of its public type system.
#   - These types live under src/fl/math/fixed_point/ on FastLED master and
#     are NOT present in the latest tagged release (3.10.3, 2025-09-20), so
#     a release-tagged WASM build fails with "no type named 's16x16' in
#     namespace 'fl'".
#   - The embedded targets already track FastLED master via platformio.ini
#     (lib_deps: https://github.com/FastLED/FastLED.git#master), so this
#     keeps the WASM build aligned with the same source of truth.
#
# TODO: revert to a tagged FastLED release once one ships the fl::s16x16 /
# fl::u16x16 / fl::s24x8 / fl::u24x8 fixed-point types. When the next FastLED
# release exposes these types, switch back to a tagged-release URL
# (https://github.com/FastLED/FastLED/archive/refs/tags/<version>.zip) and
# rename FASTLED_REVISION back to FASTLED_VERSION.
#
# POLARSHADER_FASTLED_REVISION_OVERRIDE lets an external orchestrator pin a
# different FastLED commit (e.g. to share a single revision across multiple
# projects that consume PolarShader). MUST be read here, before
# FASTLED_ARCHIVE_URL and FASTLED_LIBRARY_ROOT are derived from it.
FASTLED_REVISION = os.environ.get(
    "POLARSHADER_FASTLED_REVISION_OVERRIDE",
    "cec6034926407cdc89e0c570aba3ad2bf8f0b907",  # master @ 2026-05-03
)
FASTLED_ARCHIVE_URL = f"https://github.com/FastLED/FastLED/archive/{FASTLED_REVISION}.zip"
FASTLED_LIBRARY_ROOT = FASTLED_CACHE_ROOT / f"FastLED-{FASTLED_REVISION}"
PLACEHOLDER_FRONTEND_MARKER = 'module._extern_setup();'
MINIMAL_FRONTEND_MARKERS = (
    "const canvas = document.getElementById('canvas');",
    "FastLED WASM",
)

EXCLUDED_SOURCE_FILES = {
    "FastLED.h",
    "main_samd.cpp",
    "main_teensy.cpp",
    "main_rp2040_fabric.cpp",
    "main_rp2040_round.cpp",
}

EXCLUDED_SOURCE_DIRS = {
    "native",
}

EXCLUDED_AMALGAMATED_SOURCE_PATHS = {
    "display/src/SmartMatrixDisplay.cpp",
    # Native-only PGM/PPM dumper: ships its own main() and includes
    # native/Arduino.h (a dir excluded from staging). platformio.ini keeps it
    # out of every env except native_pf_snapshot; exclude it here too so the
    # WASM unity build never links a second main.
    "tools/pf_snapshot.cpp",
}


def pinned_fastled_cli_version() -> str:
    prefix = "fastled=="
    for requirement in REQUIREMENTS_PATH.read_text(encoding="utf-8").splitlines():
        normalized_requirement = requirement.split("#", 1)[0].strip()
        if normalized_requirement.startswith(prefix):
            return normalized_requirement[len(prefix):].strip()

    raise SystemExit(f"Missing pinned fastled CLI version in {REQUIREMENTS_PATH}")


FASTLED_CLI_VERSION = pinned_fastled_cli_version()


@dataclass(frozen=True)
class Sketch:
    name: str
    source_file: Path


SKETCHES = (
    Sketch("composer", SKETCH_ROOT / "composer" / "composer.ino"),
)

# POLARSHADER_SKETCHES_OVERRIDE filters SKETCHES to a comma-separated list of
# names. The composer is the only shipped sketch; it renders every display and
# selects fabric/round at startup via _composer_set_initial_display. The
# override hook is kept for future sketches. Unknown names are rejected loudly.
if "POLARSHADER_SKETCHES_OVERRIDE" in os.environ:
    _requested_sketch_names = {
        name.strip()
        for name in os.environ["POLARSHADER_SKETCHES_OVERRIDE"].split(",")
        if name.strip()
    }
    _known_sketch_names = {sketch.name for sketch in SKETCHES}
    _unknown_sketch_names = _requested_sketch_names - _known_sketch_names
    if _unknown_sketch_names:
        raise SystemExit(
            "POLARSHADER_SKETCHES_OVERRIDE references unknown sketch(es): "
            f"{sorted(_unknown_sketch_names)}. Known: {sorted(_known_sketch_names)}."
        )
    SKETCHES = tuple(
        sketch for sketch in SKETCHES if sketch.name in _requested_sketch_names
    )
    if not SKETCHES:
        raise SystemExit(
            "POLARSHADER_SKETCHES_OVERRIDE filtered out every sketch."
        )

# Sketches that ship a custom UI panel mounted into FastLED's generated
# index.html. For each, we copy the listed assets verbatim into the dist
# dir alongside the FastLED-built page and inject <link>/<script> tags.
COMPOSER_PANEL_ASSETS = {
    "css":     ("composer.css",),
    "modules": ("schema.js", "codec.js", "stepper.js", "signal-editor.js", "composer.js"),
}
SKETCHES_WITH_PANEL = {"composer": COMPOSER_PANEL_ASSETS}


def reset_directory(path: Path) -> None:
    if path.exists():
        shutil.rmtree(path)
    path.mkdir(parents=True, exist_ok=True)


def copy_tree_without_platform_entrypoints(stage_dir: Path) -> None:
    for child in SOURCE_ROOT.iterdir():
        if child.name in EXCLUDED_SOURCE_DIRS:
            continue

        destination = stage_dir / child.name
        if child.is_dir():
            shutil.copytree(
                child,
                destination,
                ignore=shutil.ignore_patterns("SmartMatrixDisplay.cpp"),
            )
            continue

        if child.name in EXCLUDED_SOURCE_FILES:
            continue

        shutil.copy2(child, destination)


def list_amalgamated_source_files(stage_dir: Path) -> list[str]:
    source_files: list[str] = []
    for source_path in sorted(stage_dir.rglob("*.cpp")):
        relative_path = source_path.relative_to(stage_dir).as_posix()
        if relative_path in EXCLUDED_AMALGAMATED_SOURCE_PATHS:
            continue
        source_files.append(relative_path)
    return source_files


def stage_sketch(sketch: Sketch) -> Path:
    stage_dir = STAGE_ROOT / sketch.name
    reset_directory(stage_dir)
    copy_tree_without_platform_entrypoints(stage_dir)

    # The fastled CLI generates a single sketch.cpp by concatenating only
    # the .ino files in the staged dir (see fastled-wasm v2.0.6
    # toolchain/emscripten.py: _create_sketch_wrapper). Other .cpp files
    # are detected but never passed to em++, so anything we do not pull
    # into the .ino is silently dropped from the link. We therefore
    # amalgamate every project .cpp into the staged .ino as a unity build.
    sketch_includes = "\n".join(
        f'#include "{rel_path}"' for rel_path in list_amalgamated_source_files(stage_dir)
    )
    sketch_prelude = os.environ.get("POLARSHADER_SKETCH_PRELUDE_OVERRIDE", "")
    if sketch_prelude and not sketch_prelude.endswith("\n"):
        sketch_prelude += "\n"
    user_sketch = sketch.source_file.read_text(encoding="utf-8")
    (stage_dir / f"{sketch.name}.ino").write_text(
        "// Auto-generated by web/build_site.py: project sources amalgamated\n"
        "// below because the fastled CLI only compiles the sketch's .ino.\n"
        f"{sketch_prelude}"
        f"{sketch_includes}\n\n{user_sketch}",
        encoding="utf-8",
    )
    return stage_dir


def disable_fastled_wasm_build_system(fastled_dir: Path) -> None:
    wasm_build_script = fastled_dir / "ci" / "wasm_build.py"
    disabled_script = fastled_dir / "ci" / "wasm_build.py.disabled"

    if not wasm_build_script.exists():
        return
    if disabled_script.exists():
        disabled_script.unlink()
    wasm_build_script.rename(disabled_script)


def remove_placeholder_frontend_dist(fastled_dir: Path) -> None:
    dist_dir = fastled_dir / "src" / "platforms" / "wasm" / "compiler" / "dist"
    index_path = dist_dir / "index.html"
    if not index_path.exists():
        return

    index_text = index_path.read_text(encoding="utf-8")
    if PLACEHOLDER_FRONTEND_MARKER in index_text:
        shutil.rmtree(dist_dir)


def normalize_fastled_library(fastled_dir: Path) -> Path:
    disable_fastled_wasm_build_system(fastled_dir)
    remove_placeholder_frontend_dist(fastled_dir)
    return fastled_dir


def download_fastled_source(destination_dir: Path) -> None:
    FASTLED_CACHE_ROOT.mkdir(parents=True, exist_ok=True)
    if destination_dir.exists():
        shutil.rmtree(destination_dir)

    with tempfile.TemporaryDirectory(dir=FASTLED_CACHE_ROOT) as temp_dir_name:
        archive_path = Path(temp_dir_name) / f"FastLED-{FASTLED_REVISION}.zip"
        with urllib.request.urlopen(FASTLED_ARCHIVE_URL) as response, archive_path.open("wb") as archive_file:
            shutil.copyfileobj(response, archive_file)

        with zipfile.ZipFile(archive_path) as archive:
            archive.extractall(FASTLED_CACHE_ROOT)

    extracted_dir = FASTLED_CACHE_ROOT / f"FastLED-{FASTLED_REVISION}"
    if extracted_dir != destination_dir:
        extracted_dir.rename(destination_dir)


def ensure_fastled_library() -> Path:
    # FASTLED_REVISION is encoded in FASTLED_LIBRARY_ROOT, so directory
    # presence is a sufficient cache check (a different revision lands in
    # a different directory).
    if (FASTLED_LIBRARY_ROOT / "library.json").exists():
        return normalize_fastled_library(FASTLED_LIBRARY_ROOT)

    download_fastled_source(FASTLED_LIBRARY_ROOT)
    return normalize_fastled_library(FASTLED_LIBRARY_ROOT)


def fastled_cli_version(executable: Path) -> str | None:
    try:
        completed = subprocess.run(
            [str(executable), "--version"],
            check=True,
            capture_output=True,
            text=True,
        )
    except (OSError, subprocess.CalledProcessError):
        return None

    version_output = "\n".join(
        output.strip()
        for output in (completed.stdout, completed.stderr)
        if output.strip()
    )
    if not version_output:
        return None

    version_match = re.search(r"\b\d+\.\d+\.\d+(?:[A-Za-z0-9._+-]*)\b", version_output)
    return version_match.group(0) if version_match else version_output.splitlines()[0]


def resolve_fastled_executable() -> str:
    candidate_names = ("fastled.exe", "fastled") if os.name == "nt" else ("fastled",)
    candidate_paths: list[Path] = []

    for candidate_name in candidate_names:
        candidate_paths.append(Path(sys.executable).parent / candidate_name)

    path_candidate = shutil.which("fastled")
    if path_candidate is not None:
        candidate_paths.append(Path(path_candidate))

    checked_paths: set[str] = set()
    mismatched_versions: list[tuple[Path, str]] = []
    for candidate_path in candidate_paths:
        candidate_key = str(candidate_path)
        if candidate_key in checked_paths:
            continue
        checked_paths.add(candidate_key)

        version = fastled_cli_version(candidate_path)
        if version is None:
            continue
        if version == FASTLED_CLI_VERSION:
            return str(candidate_path)
        mismatched_versions.append((candidate_path, version))

    mismatch_hint = ""
    if mismatched_versions:
        mismatched_path, mismatched_version = mismatched_versions[0]
        mismatch_hint = f" Found {mismatched_path} reporting version {mismatched_version}."

    raise SystemExit(
        f"fastled CLI {FASTLED_CLI_VERSION} not found for {sys.executable}. "
        f"Install it with `{sys.executable} -m pip install -r {REQUIREMENTS_PATH}`."
        f"{mismatch_hint}"
    )


def detect_clang_toolchain_platform_arch() -> tuple[str, str]:
    if sys.platform == "win32":
        platform_name = "win"
    elif sys.platform == "darwin":
        platform_name = "darwin"
    else:
        platform_name = "linux"

    machine = getattr(os, "uname", lambda: None)()
    machine_name = machine.machine.lower() if machine is not None else ""
    if not machine_name:
        machine_name = "amd64" if struct.calcsize("P") * 8 == 64 else "x86"

    if machine_name in ("x86_64", "amd64"):
        arch = "x86_64"
    elif machine_name in ("arm64", "aarch64"):
        arch = "arm64"
    else:
        arch = machine_name

    return platform_name, arch


def ensure_emscripten_installation() -> Path:
    from clang_tool_chain.installers.emscripten import ensure_emscripten_available
    from clang_tool_chain.path_utils import get_emscripten_install_dir

    platform_name, arch = detect_clang_toolchain_platform_arch()
    ensure_emscripten_available(platform_name, arch)
    install_dir = get_emscripten_install_dir(platform_name, arch)
    config_path = install_dir / ".emscripten"
    if not config_path.exists():
        raise FileNotFoundError(f"Expected Emscripten config at {config_path}")
    return install_dir


def run_fastled_compile(stage_dir: Path) -> None:
    HOME_ROOT.mkdir(parents=True, exist_ok=True)
    fastled_library = ensure_fastled_library()
    fastled_executable = resolve_fastled_executable()

    command = [
        fastled_executable,
        str(stage_dir),
        "--just-compile",
        "--release",
        "--fastled-path",
        str(fastled_library),
    ]

    original_home = os.environ.get("HOME")
    os.environ["HOME"] = str(HOME_ROOT)
    try:
        emscripten_install_dir = ensure_emscripten_installation()
    finally:
        if original_home is None:
            os.environ.pop("HOME", None)
        else:
            os.environ["HOME"] = original_home

    env = dict(os.environ)
    env["HOME"] = str(HOME_ROOT)
    env["EMSCRIPTEN"] = str(emscripten_install_dir / "emscripten")
    env["EMSCRIPTEN_ROOT"] = env["EMSCRIPTEN"]
    env["EM_CONFIG"] = str(emscripten_install_dir / ".emscripten")
    env["EMSDK_PYTHON"] = sys.executable
    env["EMCC_SKIP_SANITY_CHECK"] = "1"
    env["PATH"] = f"{emscripten_install_dir / 'bin'}{os.pathsep}{env.get('PATH', '')}"

    subprocess.run(
        command,
        cwd=stage_dir,
        check=True,
        env=env,
    )


def find_build_output(stage_dir: Path) -> Path:
    preferred_dirs = (
        stage_dir / "fastled_js",
        stage_dir / "build",
        stage_dir / "out",
    )

    for candidate in preferred_dirs:
        if (candidate / "index.html").exists():
            return candidate

    built_pages = [path.parent for path in stage_dir.rglob("index.html")]
    if not built_pages:
        raise FileNotFoundError(f"Could not find FastLED build output in {stage_dir}")

    return max(built_pages, key=lambda path: path.stat().st_mtime)


def validate_frontend_output(output_dir: Path) -> None:
    index_path = output_dir / "index.html"
    if not index_path.exists():
        raise FileNotFoundError(f"Missing frontend index at {index_path}")

    index_text = index_path.read_text(encoding="utf-8")
    if PLACEHOLDER_FRONTEND_MARKER in index_text and any(
        marker in index_text for marker in MINIMAL_FRONTEND_MARKERS
    ):
        raise RuntimeError(
            "FastLED frontend assets were not built; refusing to publish the minimal fallback page."
        )


def inject_panel_assets(sketch: "Sketch", dist_sketch_dir: Path, assets: dict) -> None:
    """For sketches with a custom UI panel: copy CSS + JS modules into the
    dist sketch dir and patch the FastLED-generated index.html to load them
    via <link rel=stylesheet> and <script type=module>.

    The panel itself is mounted from JS at runtime (composer.js builds the
    DOM after the WASM module is ready), so we don't ship an HTML fragment.

    A short content hash is appended to each asset URL as a query string so
    the browser doesn't keep serving stale modules from cache after a
    redeploy. ES module imports inside composer.js are *also* rewritten to
    carry the same hash — otherwise Chrome/Firefox keep their own
    HTTP-cache copies of the imported modules across reloads.
    """
    import hashlib

    sketch_src_dir = sketch.source_file.parent
    asset_names = list(assets["css"]) + list(assets["modules"])

    # Compute a single hash over all panel sources so the same redeploy
    # invalidates every URL together. (Per-file hashes would also work but
    # add no benefit here — the panel is shipped as a unit.)
    hasher = hashlib.sha1()
    for fname in asset_names:
        src = sketch_src_dir / fname
        if not src.exists():
            raise FileNotFoundError(f"Panel asset missing: {src}")
        hasher.update(src.read_bytes())
    cache_buster = hasher.hexdigest()[:8]

    # Copy assets verbatim, then patch ES-module import statements in each
    # JS module to carry the same cache_buster on every relative import.
    module_set = set(assets["modules"])
    for fname in asset_names:
        src = sketch_src_dir / fname
        dst = dist_sketch_dir / fname
        if fname in module_set:
            text = src.read_text(encoding="utf-8")
            # Naive but sufficient: rewrite `from './foo.js'` →
            # `from './foo.js?v=HASH'`. Only matches the panel's own modules.
            for sibling in module_set:
                if sibling == fname:
                    continue
                text = text.replace(
                    f"from './{sibling}'",
                    f"from './{sibling}?v={cache_buster}'",
                )
                text = text.replace(
                    f"import('./{sibling}')",
                    f"import('./{sibling}?v={cache_buster}')",
                )
            dst.write_text(text, encoding="utf-8")
        else:
            shutil.copy2(src, dst)

    # Build the injection block. CSS first, then ES modules (composer.js
    # imports the others). FastLED's generated index.html ends with a
    # closing </body>; insert just before it.
    css_links = "\n".join(
        f'    <link rel="stylesheet" href="./{name}?v={cache_buster}">'
        for name in assets["css"]
    )
    # composer.js is the entry module; the others are imported transitively.
    entry_module = assets["modules"][-1]
    inject = (
        css_links
        + f'\n    <script type="module" src="./{entry_module}?v={cache_buster}"></script>\n'
    )

    index_path = dist_sketch_dir / "index.html"
    text = index_path.read_text(encoding="utf-8")
    if "</body>" not in text:
        raise RuntimeError(f"Cannot inject panel assets — no </body> in {index_path}")
    text = text.replace("</body>", inject + "</body>", 1)
    index_path.write_text(text, encoding="utf-8")


# The FastLED CLI generates a background render worker (fastled_background_worker.js)
# that runs its own wasm module and owns the OffscreenCanvas — the main-thread
# module's frames are never shown. Its onmessage switch has no case for the
# composer's live-scene push, so composer_apply_scene never reaches the visible
# module. We inject a case + handler here (the file is regenerated every build,
# so this must run post-generation rather than being a checked-in edit).
_WORKER_CASE_ANCHOR = (
    "      default:\n"
    "        throw new Error(`Unknown message type: ${type}`);"
)
_WORKER_CASE_INJECT = (
    '      case "composer_apply_scene":\n'
    "        response = handleComposerApplyScene(payload);\n"
    "        break;\n"
    '      case "composer_set_display":\n'
    "        response = handleComposerSetDisplay(payload);\n"
    "        break;\n"
    '      case "composer_set_bloom":\n'
    "        response = handleComposerSetBloom(payload);\n"
    "        break;\n"
    + _WORKER_CASE_ANCHOR
)
_WORKER_START_ANCHOR = (
    "    workerState.externFunctions.externSetup();\n"
    '    workerLog("LOG", "BACKGROUND_WORKER", "FastLED setup completed");'
)
# The graphics manager is created lazily; a bloom toggle sent before then is
# stashed on workerState.composerBloomEnabled. Replay it onto the freshly built
# manager so an early toggle is not lost.
_WORKER_GM_INIT_ANCHOR = (
    '    workerLog("LOG", "BACKGROUND_WORKER", "Graphics manager initialized", {'
)
_WORKER_GM_INIT_INJECT = (
    "    if (workerState.graphicsManager"
    " && workerState.composerBloomEnabled !== undefined"
    ' && typeof workerState.graphicsManager.setBloomEnabled === "function") {\n'
    "      workerState.graphicsManager.setBloomEnabled(workerState.composerBloomEnabled);\n"
    "    }\n"
    + _WORKER_GM_INIT_ANCHOR
)
_WORKER_START_INJECT = (
    "    applyComposerInitialDisplayFromUrl(workerState.fastledModule);\n"
    + _WORKER_START_ANCHOR
)
_WORKER_HANDLER = """

// Injected by PolarShader build_site.py (patch_render_worker): apply a composer
// .psc scene to this worker's wasm module, which owns the OffscreenCanvas. The
// worker uses `workerState.fastledModule` as its Module; `_composer_apply_scene`
// is an EMSCRIPTEN_KEEPALIVE export of the composer sketch. Returns { status }
// where status is 0 on success or a non-zero composer::DecodeStatus.
function composerCommandSeq(payload) {
  const raw = payload && typeof payload.seq === 'number' ? payload.seq : 0;
  if (!Number.isFinite(raw) || raw < 0) return 0;
  return raw >>> 0;
}

function composerPhase(seq, phase, data) {
  workerLog("LOG", "COMPOSER", "seq " + seq + " " + phase, {
    seq,
    phase,
    ...(data || {})
  });
}

function enterComposerCommand(seq, command) {
  const latest = workerState.composerLatestSeq || 0;
  if (seq !== 0 && latest !== 0 && seq < latest) {
    composerPhase(seq, command + "-stale", { latest });
    return {
      ok: false,
      stale: true,
      seq,
      reason: "stale " + command + " seq " + seq + "; latest is " + latest
    };
  }
  if (seq !== 0) workerState.composerLatestSeq = seq;
  composerPhase(seq, command + "-accepted", { latest: workerState.composerLatestSeq || latest });
  return { ok: true, seq };
}

function composerDisplayFromUrlParams(params) {
  const raw = params && (params.display || params.ps_display);
  if (raw === "1" || raw === "round" || raw === "polar") return 1;
  return 0;
}

function applyComposerInitialDisplayFromUrl(Module) {
  if (!Module || typeof Module._composer_set_initial_display !== "function") return;
  const which = composerDisplayFromUrlParams(workerState.urlParams || {});
  Module._composer_set_initial_display(which);
  composerPhase(0, "initial-display", { which });
}

function handleComposerApplyScene(payload) {
  const seq = composerCommandSeq(payload);
  const gate = enterComposerCommand(seq, "apply");
  if (!gate.ok) return { status: -1, stale: true, seq, reason: gate.reason };

  const Module = workerState.fastledModule;
  composerPhase(seq, "worker-apply-enter");
  // Granular guards: a bare {status:-1} can't be told apart from a dropped
  // response on the composer side, so each failure carries a `reason`.
  if (!Module) return { status: -1, seq, reason: 'worker module not ready' };
  if (!Module._composer_apply_scene) return { status: -1, seq, reason: 'missing _composer_apply_scene export' };
  if (!Module._malloc || !Module._free || !Module.HEAPU8) return { status: -1, seq, reason: 'missing _malloc/_free/HEAPU8' };
  const bytes = new Uint8Array(payload && payload.bytes ? payload.bytes : []);
  let ptr = 0;
  let enteredApply = false;
  let applyReturned = false;
  let result;
  try {
    composerPhase(seq, "worker-apply-malloc-start", { byteLength: bytes.length });
    ptr = Module._malloc(bytes.length);
    if (bytes.length > 0 && !ptr) return { status: -1, seq, reason: 'malloc returned null' };
    composerPhase(seq, "worker-apply-copy-start", { byteLength: bytes.length });
    Module.HEAPU8.set(bytes, ptr);
    // status is 0 on success or a non-zero composer::DecodeStatus.
    composerPhase(seq, "wasm-apply-start", { byteLength: bytes.length });
    enteredApply = true;
    const status = typeof Module._composer_apply_scene_seq === "function"
      ? Module._composer_apply_scene_seq(ptr, bytes.length, seq)
      : Module._composer_apply_scene(ptr, bytes.length);
    applyReturned = true;
    composerPhase(seq, "wasm-apply-end", { status });
    result = { status, seq };
  } catch (e) {
    // A wasm trap here would otherwise reject the message and surface as an
    // opaque failure; report it as a -1 with the trap message instead.
    result = { status: -1, seq, reason: 'apply threw in worker: ' + (e && e.message ? e.message : e) };
  }

  if (ptr && (!enteredApply || applyReturned)) {
    try {
      Module._free(ptr);
    } catch (e) {
      const reason = 'free after worker apply failed: ' + (e && e.message ? e.message : e);
      if (result.status === 0) return { status: -1, seq, reason };
      result.reason = result.reason ? result.reason + '; ' + reason : reason;
    }
  }
  return result;
}

function ensureWorkerWasmFunctions(seq) {
  composerPhase(seq, "screenmap-bind-start");
  const Module = workerState.fastledModule;
  if (!Module) return { ok: false, reason: 'worker module not ready' };
  if (!Module.cwrap) return { ok: false, reason: 'missing cwrap' };
  if (!Module._malloc || !Module._free || !Module.getValue || !Module.UTF8ToString) {
    return { ok: false, reason: 'missing screenmap memory helpers' };
  }

  try {
    const fns = workerState.wasmFunctions || {};
    if (!fns.getFrameData) fns.getFrameData = Module.cwrap("getFrameData", "number", ["number"]);
    if (!fns.getScreenMapData) fns.getScreenMapData = Module.cwrap("getScreenMapData", "number", ["number"]);
    if (!fns.getStripPixelData) fns.getStripPixelData = Module.cwrap("getStripPixelData", "number", ["number", "number"]);
    if (!fns.freeFrameData) fns.freeFrameData = Module.cwrap("freeFrameData", null, ["number"]);
    workerState.wasmFunctions = fns;
    composerPhase(seq, "screenmap-bind-ok");
    return { ok: true, Module, fns };
  } catch (e) {
    return { ok: false, reason: 'wasm function bind failed: ' + (e && e.message ? e.message : e) };
  }
}

// Injected by PolarShader build_site.py (patch_render_worker): re-fetch the
// active screenmap from the wasm module and push it to the graphics manager.
// The worker only fetches the screenmap once (in handleStart), so after a
// runtime display switch the renderer would keep the previous geometry's
// layout. GraphicsManagerThreeJS.updateScreenMap() flags a scene rebuild, so
// pushing the new map here re-lays the pixels (grid ↔ round).
function refreshWorkerScreenMap(seq) {
  const binding = ensureWorkerWasmFunctions(seq);
  if (!binding.ok) return binding;
  const { Module, fns } = binding;
  composerPhase(seq, "screenmap-size-malloc-start");
  const sizePtr = Module._malloc(4);
  let dataPtr = 0;
  try {
    composerPhase(seq, "screenmap-fetch-start");
    dataPtr = fns.getScreenMapData(sizePtr);
    if (dataPtr === 0) {
      return { ok: false, reason: 'no screenmap data available after display switch' };
    }

    composerPhase(seq, "screenmap-parse-start");
    const size = Module.getValue(sizePtr, "i32");
    const screenMapData = JSON.parse(Module.UTF8ToString(dataPtr, size));
    workerState.screenMaps = screenMapData;
    if (workerState.graphicsManager && workerState.graphicsManager.updateScreenMap) {
      composerPhase(seq, "screenmap-update-start");
      workerState.graphicsManager.updateScreenMap(screenMapData);
      if (typeof workerState.graphicsManager.clearTexture === "function") {
        composerPhase(seq, "screenmap-clear-start");
        workerState.graphicsManager.clearTexture();
      }
      if (typeof workerState.graphicsManager.render === "function") {
        composerPhase(seq, "screenmap-render-start");
        workerState.graphicsManager.render();
      }
    }
    composerPhase(seq, "screenmap-ok", { screenMapCount: Object.keys(screenMapData || {}).length });
    return { ok: true, screenMapCount: Object.keys(screenMapData || {}).length };
  } catch (e) {
    workerLog("ERROR", "BACKGROUND_WORKER", "Failed to refresh screenmap after display switch", e);
    return { ok: false, reason: 'screenmap refresh failed: ' + (e && e.message ? e.message : e) };
  } finally {
    if (dataPtr !== 0 && fns.freeFrameData) fns.freeFrameData(dataPtr);
    Module._free(sizePtr);
  }
}

// Injected by PolarShader build_site.py (patch_render_worker): switch the active
// display geometry on this worker's module (0 = fabric, 1 = round). The C++ side
// rebuilds the display and replays the last-applied scene, so the pipeline is
// preserved. The new geometry's screenmap must then be pushed to the renderer so
// the canvas re-lays out (round for polar displays). Returns { ok } so the
// composer can report success/failure.
function handleComposerSetDisplay(payload) {
  const seq = composerCommandSeq(payload);
  const gate = enterComposerCommand(seq, "display");
  if (!gate.ok) return { ok: false, stale: true, seq, reason: gate.reason };

  const Module = workerState.fastledModule;
  composerPhase(seq, "worker-display-enter");
  if (!Module) return { ok: false, seq, reason: 'worker module not ready' };
  if (!Module._composer_set_display) return { ok: false, seq, reason: 'missing _composer_set_display export' };
  const which = payload && typeof payload.which === 'number' ? payload.which : 0;
  let displayApplied = false;
  try {
    composerPhase(seq, "wasm-display-start", { which });
    if (typeof Module._composer_set_display_seq === "function") {
      Module._composer_set_display_seq(which, seq);
    } else {
      Module._composer_set_display(which);
    }
    displayApplied = true;
    composerPhase(seq, "wasm-display-end", { which });
    const refresh = refreshWorkerScreenMap(seq);
    if (!refresh.ok) {
      return {
        ok: false,
        seq,
        displayApplied: true,
        screenMapOk: false,
        reason: 'display switched, but screenmap refresh failed: ' + refresh.reason
      };
    }
    composerPhase(seq, "worker-display-ok", { screenMapCount: refresh.screenMapCount });
    return {
      ok: true,
      seq,
      displayApplied: true,
      screenMapOk: true,
      screenMapCount: refresh.screenMapCount
    };
  } catch (e) {
    return {
      ok: false,
      seq,
      displayApplied,
      screenMapOk: false,
      reason: 'set_display threw in worker: ' + (e && e.message ? e.message : e)
    };
  }
}

// Injected by PolarShader build_site.py (patch_render_worker): toggle the ThreeJS
// bloom post-processing pass on the graphics manager that owns the OffscreenCanvas
// in this worker. The graphics manager is patched (patch_threejs_bloom_toggle) to
// expose setBloomEnabled + honour bloom_enabled across scene rebuilds. Store the
// desired state so it survives a graphics manager that is not ready yet.
function handleComposerSetBloom(payload) {
  const enabled = !(payload && payload.enabled === false);
  workerState.composerBloomEnabled = enabled;
  const gm = workerState.graphicsManager;
  if (!gm) return { ok: true, enabled, deferred: true };
  if (typeof gm.setBloomEnabled === "function") {
    gm.setBloomEnabled(enabled);
  } else {
    gm.bloom_enabled = enabled;
  }
  return { ok: true, enabled };
}
"""


def patch_render_worker(dist_sketch_dir: Path) -> None:
    """Inject a composer_apply_scene message handler into the FastLED-generated
    background render worker so live scene pushes reach the module that owns the
    canvas. Raises if the expected anchor is gone (FastLED changed the worker),
    so a silent no-op can't mask a broken composer."""
    worker_path = dist_sketch_dir / "fastled_background_worker.js"
    if not worker_path.exists():
        raise FileNotFoundError(
            f"Render worker not found for panel sketch: {worker_path}. "
            "FastLED no longer emits a background worker — the composer's "
            "worker-routed scene apply needs revisiting."
        )
    text = worker_path.read_text(encoding="utf-8")
    if "handleComposerApplyScene" not in text:
        if _WORKER_CASE_ANCHOR not in text:
            raise RuntimeError(
                f"Cannot patch {worker_path}: onmessage switch anchor not found. "
                "The FastLED worker's message dispatch changed; update "
                "patch_render_worker."
            )
        text = text.replace(_WORKER_CASE_ANCHOR, _WORKER_CASE_INJECT, 1)
        text = text + _WORKER_HANDLER
    if "applyComposerInitialDisplayFromUrl(workerState.fastledModule);" not in text:
        if _WORKER_START_ANCHOR not in text:
            raise RuntimeError(
                f"Cannot patch {worker_path}: worker start anchor not found. "
                "FastLED's worker setup changed; update patch_render_worker."
            )
        text = text.replace(_WORKER_START_ANCHOR, _WORKER_START_INJECT, 1)
    if "workerState.composerBloomEnabled" not in text.split(_WORKER_GM_INIT_ANCHOR, 1)[0]:
        if _WORKER_GM_INIT_ANCHOR not in text:
            raise RuntimeError(
                f"Cannot patch {worker_path}: graphics-manager-init anchor not found. "
                "FastLED's worker graphics init changed; update patch_render_worker."
            )
        text = text.replace(_WORKER_GM_INIT_ANCHOR, _WORKER_GM_INIT_INJECT, 1)
    worker_path.write_text(text, encoding="utf-8")


_MAIN_WORKER_READY_ANCHOR = (
    '    console.log("FastLED running in Web Worker mode (background thread)");'
)
_MAIN_WORKER_READY_INJECT = (
    _MAIN_WORKER_READY_ANCHOR
    + """
    if (typeof window !== "undefined") {
      window.__polarComposerWorkerManager = fastLEDWorkerManager;
      window.__polarComposerWorkerReady = true;
      window.dispatchEvent(new CustomEvent("polar:worker-ready", {
        detail: { mode: "background_worker" }
      }));
      (async () => {
        try {
          const raw = window.sessionStorage && window.sessionStorage.getItem("polarComposerBootScene:v1");
          const saved = raw ? JSON.parse(raw) : null;
          if (saved?.verified !== true || !Array.isArray(saved.bytes) || saved.bytes.length === 0) return;
          const bytes = new Uint8Array(saved.bytes);
          const buffer = bytes.buffer.slice(bytes.byteOffset, bytes.byteOffset + bytes.byteLength);
          await fastLEDWorkerManager.sendMessageWithResponse({
            type: "composer_apply_scene",
            payload: { bytes: buffer, seq: 0 }
          }, [buffer], 2000);
        } catch (error) {
          console.warn("Polar composer boot scene replay failed:", error);
        }
      })();
    }"""
)

_MAIN_SETUP_ANCHOR = (
    "      fastLEDController.setup();\n"
    '      console.log("🔧 fastLEDController.setup() completed successfully");'
)
_MAIN_SETUP_INJECT = (
    """      if (typeof moduleInstance._composer_set_initial_display === "function") {
        const rawDisplay = urlParams.get("display") || urlParams.get("ps_display");
        const composerDisplay = (rawDisplay === "1" || rawDisplay === "round" || rawDisplay === "polar") ? 1 : 0;
        moduleInstance._composer_set_initial_display(composerDisplay);
      }
"""
    + _MAIN_SETUP_ANCHOR
)


def patch_main_worker_ready_hook(dist_sketch_dir: Path) -> None:
    """Expose a reliable worker-ready marker for the composer panel.

    FastLED emits its own worker event while the generated app is booting; on a
    full gfx-mode reload the composer module can miss that one-shot event and
    then incorrectly fall back to the main module. Publish a tiny stable hook
    after worker start so composer can detect/replay into the worker-owned
    canvas.
    """
    index_path = dist_sketch_dir / "index.js"
    if not index_path.exists():
        raise FileNotFoundError(f"Generated index.js not found: {index_path}")
    text = index_path.read_text(encoding="utf-8")
    if "__polarComposerWorkerReady" in text:
        return
    if _MAIN_WORKER_READY_ANCHOR not in text:
        raise RuntimeError(
            f"Cannot patch {index_path}: worker-ready anchor not found. "
            "FastLED's startup logging changed; update patch_main_worker_ready_hook."
        )
    text = text.replace(_MAIN_WORKER_READY_ANCHOR, _MAIN_WORKER_READY_INJECT, 1)
    index_path.write_text(text, encoding="utf-8")


def patch_main_initial_display_hook(dist_sketch_dir: Path) -> None:
    """Select the composer display before main-thread FastLED setup.

    On GitHub Pages or worker-disabled browsers, the visible renderer is the
    main module. Its screenmap is captured during fastLEDController.setup(), so
    the startup display must be selected before that call.
    """
    index_path = dist_sketch_dir / "index.js"
    if not index_path.exists():
        raise FileNotFoundError(f"Generated index.js not found: {index_path}")
    text = index_path.read_text(encoding="utf-8")
    if "_composer_set_initial_display(composerDisplay)" in text:
        return
    if _MAIN_SETUP_ANCHOR not in text:
        raise RuntimeError(
            f"Cannot patch {index_path}: main setup anchor not found. "
            "FastLED's setup flow changed; update patch_main_initial_display_hook."
        )
    text = text.replace(_MAIN_SETUP_ANCHOR, _MAIN_SETUP_INJECT, 1)
    index_path.write_text(text, encoding="utf-8")


_THREEJS_BLOOM_REPLACEMENTS = (
    ("    this.bloom_stength = 1;\n", "    this.bloom_stength = 0.125;\n"),
    ("    this.bloom_radius = 16;\n", "    this.bloom_radius = 2;\n"),
    ("    this.base_bloom_strength = 16;\n", "    this.base_bloom_strength = 2;\n"),
    ("    this.max_bloom_strength = 20;\n", "    this.max_bloom_strength = 2.5;\n"),
    ("    this.min_bloom_strength = 0.5;\n", "    this.min_bloom_strength = 0.0625;\n"),
    ("      this.bloom_stength = 16;\n", "      this.bloom_stength = 2;\n"),
    ("      this.bloom_radius = 1;\n", "      this.bloom_radius = 0.125;\n"),
)


def patch_threejs_bloom(dist_sketch_dir: Path) -> None:
    """Reduce FastLED's generated ThreeJS bloom settings to 12.5% of default."""
    asset_dir = dist_sketch_dir / "assets"
    paths = sorted(asset_dir.glob("graphics_manager_threejs-*.js"))
    if not paths:
        raise FileNotFoundError(
            f"ThreeJS graphics manager not found under {asset_dir}. "
            "The bloom patch needs updating for FastLED's generated output."
        )

    for path in paths:
        text = path.read_text(encoding="utf-8")
        for old, new in _THREEJS_BLOOM_REPLACEMENTS:
            if old not in text:
                raise RuntimeError(
                    f"Cannot patch bloom in {path}: expected snippet not found: {old.strip()}"
                )
            text = text.replace(old, new, 1)
        path.write_text(text, encoding="utf-8")


# Add a runtime bloom on/off switch to FastLED's generated ThreeJS graphics
# manager (driven from the composer panel via the worker's composer_set_bloom
# message). The pass itself is still created; we flip its `.enabled` flag, which
# EffectComposer honours per-frame, and stash the desired state in
# `this.bloom_enabled` so it survives scene rebuilds (_setupRenderPasses reapplies
# it to the freshly created pass).
_THREEJS_BLOOM_TOGGLE_REPLACEMENTS = (
    (
        "    this.auto_bloom_enabled = true;\n",
        "    this.auto_bloom_enabled = true;\n    this.bloom_enabled = true;\n",
    ),
    (
        "      this.composer.addPass(bloomPass);\n",
        "      bloomPass.enabled = this.bloom_enabled;\n"
        "      this.composer.addPass(bloomPass);\n",
    ),
    (
        "  /**\n   * Updates the bloom pass strength dynamically\n",
        "  /**\n"
        "   * Enables or disables the bloom pass at runtime (PolarShader)\n"
        "   * @param {boolean} enabled - Whether bloom should render\n"
        "   */\n"
        "  setBloomEnabled(enabled) {\n"
        "    this.bloom_enabled = !!enabled;\n"
        "    if (!this.composer || !this.composer.passes) {\n"
        "      return;\n"
        "    }\n"
        "    for (const pass of this.composer.passes) {\n"
        '      if (pass.constructor.name === "UnrealBloomPass") {\n'
        "        pass.enabled = this.bloom_enabled;\n"
        "      }\n"
        "    }\n"
        "  }\n"
        "  /**\n   * Updates the bloom pass strength dynamically\n",
    ),
)


def patch_threejs_bloom_toggle(dist_sketch_dir: Path) -> None:
    """Add a runtime setBloomEnabled() switch to the ThreeJS graphics manager."""
    asset_dir = dist_sketch_dir / "assets"
    paths = sorted(asset_dir.glob("graphics_manager_threejs-*.js"))
    if not paths:
        raise FileNotFoundError(
            f"ThreeJS graphics manager not found under {asset_dir}. "
            "The bloom toggle patch needs updating for FastLED's generated output."
        )

    for path in paths:
        text = path.read_text(encoding="utf-8")
        if "setBloomEnabled(enabled)" in text:
            continue
        for old, new in _THREEJS_BLOOM_TOGGLE_REPLACEMENTS:
            if old not in text:
                raise RuntimeError(
                    f"Cannot patch bloom toggle in {path}: expected snippet not found: {old.strip()}"
                )
            text = text.replace(old, new, 1)
        path.write_text(text, encoding="utf-8")


def build_site() -> None:
    reset_directory(STAGE_ROOT)
    reset_directory(DIST_ROOT)

    for sketch in SKETCHES:
        stage_dir = stage_sketch(sketch)
        run_fastled_compile(stage_dir)
        output_dir = find_build_output(stage_dir)
        validate_frontend_output(output_dir)
        dist_sketch_dir = DIST_ROOT / sketch.name
        shutil.copytree(output_dir, dist_sketch_dir)
        if sketch.name in SKETCHES_WITH_PANEL:
            inject_panel_assets(sketch, dist_sketch_dir, SKETCHES_WITH_PANEL[sketch.name])
            patch_render_worker(dist_sketch_dir)
            patch_main_initial_display_hook(dist_sketch_dir)
            patch_main_worker_ready_hook(dist_sketch_dir)
            patch_threejs_bloom(dist_sketch_dir)
            patch_threejs_bloom_toggle(dist_sketch_dir)

    shutil.copy2(LANDING_PAGE, DIST_ROOT / "index.html")


def main() -> int:
    build_site()
    return 0


if __name__ == "__main__":
    sys.exit(main())
