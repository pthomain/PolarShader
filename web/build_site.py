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
STAGE_ROOT = WEB_ROOT / ".stage"
DIST_ROOT = WEB_ROOT / "dist"
LANDING_PAGE = WEB_ROOT / "index.html"
REQUIREMENTS_PATH = WEB_ROOT / "requirements.txt"
HOME_ROOT = WEB_ROOT / ".home"
FASTLED_CACHE_ROOT = WEB_ROOT / ".fastled"
# TODO: revert to a tagged FastLED release once one ships the fl::s16x16 /
# fl::u16x16 / fl::s24x8 / fl::u24x8 fixed-point types.
#
# Why this is pinned to a master commit:
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
# When the next FastLED release exposes these types, switch back to a
# tagged-release URL (https://github.com/FastLED/FastLED/archive/refs/tags/
# <version>.zip) and rename FASTLED_REVISION back to FASTLED_VERSION.
FASTLED_REVISION = "cec6034926407cdc89e0c570aba3ad2bf8f0b907"  # master @ 2026-05-03
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
    Sketch("fabric", SKETCH_ROOT / "fabric" / "fabric.ino"),
    Sketch("round", SKETCH_ROOT / "round" / "round.ino"),
    Sketch("composer", SKETCH_ROOT / "composer" / "composer.ino"),
)

# Sketches that ship a custom UI panel mounted into FastLED's generated
# index.html. For each, we copy the listed assets verbatim into the dist
# dir alongside the FastLED-built page and inject <link>/<script> tags.
COMPOSER_PANEL_ASSETS = {
    "css":     ("composer.css",),
    "modules": ("schema.js", "codec.js", "signal-editor.js", "composer.js"),
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
    user_sketch = sketch.source_file.read_text(encoding="utf-8")
    (stage_dir / f"{sketch.name}.ino").write_text(
        "// Auto-generated by web/build_site.py: project sources amalgamated\n"
        "// below because the fastled CLI only compiles the sketch's .ino.\n"
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

    shutil.copy2(LANDING_PAGE, DIST_ROOT / "index.html")


def main() -> int:
    build_site()
    return 0


if __name__ == "__main__":
    sys.exit(main())
