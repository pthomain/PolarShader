#!/usr/bin/env python3

from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
import os
import shutil
import subprocess
import sys


REPO_ROOT = Path(__file__).resolve().parents[1]
WEB_ROOT = REPO_ROOT / "web"
SOURCE_ROOT = REPO_ROOT / "src"
SKETCH_ROOT = WEB_ROOT / "sketches"
STAGE_ROOT = WEB_ROOT / ".stage"
DIST_ROOT = WEB_ROOT / "dist"
LANDING_PAGE = WEB_ROOT / "index.html"
HOME_ROOT = WEB_ROOT / ".home"
CLANG_TOOLCHAIN_ROOT = WEB_ROOT / ".clang-tool-chain"
TOOLCHAIN_SHIM_ROOT = WEB_ROOT / ".toolchain-shims"

EXCLUDED_SOURCE_FILES = {
    "main_samd.cpp",
    "main_teensy.cpp",
    "main_rp2040_fabric.cpp",
    "main_rp2040_round.cpp",
}

EXCLUDED_SOURCE_DIRS = {
    "native",
}


@dataclass(frozen=True)
class Sketch:
    name: str
    source_file: Path


SKETCHES = (
    Sketch("fabric", SKETCH_ROOT / "fabric" / "fabric.ino"),
    Sketch("round", SKETCH_ROOT / "round" / "round.ino"),
)

FASTLED_LIBRARY_CANDIDATES = (
    REPO_ROOT / ".pio" / "libdeps" / "seeed_xiao_rp2040_fabric" / "FastLED",
    REPO_ROOT / ".pio" / "libdeps" / "seeed_xiao_rp2040_round" / "FastLED",
    REPO_ROOT / ".pio" / "libdeps" / "seeed_xiao" / "FastLED",
    REPO_ROOT / ".pio" / "libdeps" / "seeed_xiao_rp2040" / "FastLED",
)


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


def stage_sketch(sketch: Sketch) -> Path:
    stage_dir = STAGE_ROOT / sketch.name
    reset_directory(stage_dir)
    copy_tree_without_platform_entrypoints(stage_dir)
    shutil.copy2(sketch.source_file, stage_dir / f"{sketch.name}.ino")
    return stage_dir


def run_fastled_compile(stage_dir: Path) -> None:
    HOME_ROOT.mkdir(parents=True, exist_ok=True)
    CLANG_TOOLCHAIN_ROOT.mkdir(parents=True, exist_ok=True)
    TOOLCHAIN_SHIM_ROOT.mkdir(parents=True, exist_ok=True)

    emcc_shim = TOOLCHAIN_SHIM_ROOT / "clang-tool-chain-emcc"
    emar_shim = TOOLCHAIN_SHIM_ROOT / "clang-tool-chain-emar"
    for shim_path, tool_name in ((emcc_shim, "emcc"), (emar_shim, "emar")):
        shim_path.write_text(
            "\n".join(
                [
                    "#!/bin/sh",
                    "set -eu",
                    f'for candidate in "{CLANG_TOOLCHAIN_ROOT}"/emscripten/*/*/emscripten/{tool_name} "{CLANG_TOOLCHAIN_ROOT}"/emscripten/*/emscripten/{tool_name}; do',
                    '  if [ -x "$candidate" ]; then',
                    '    exec "$candidate" "$@"',
                    "  fi",
                    "done",
                    f'echo "Unable to find {tool_name} inside {CLANG_TOOLCHAIN_ROOT}." >&2',
                    "exit 1",
                    "",
                ]
            ),
            encoding="utf-8",
        )
        shim_path.chmod(0o755)

    command = ["fastled", str(stage_dir), "--just-compile", "--release"]
    fastled_library = next(
        (candidate for candidate in FASTLED_LIBRARY_CANDIDATES if candidate.exists()),
        None,
    )
    if fastled_library is not None:
        command.extend(["--fastled-path", str(fastled_library)])

    env = dict(os.environ)
    env["HOME"] = str(HOME_ROOT)
    env["CLANG_TOOL_CHAIN_DOWNLOAD_PATH"] = str(CLANG_TOOLCHAIN_ROOT)
    env["PATH"] = f"{TOOLCHAIN_SHIM_ROOT}{os.pathsep}{env.get('PATH', '')}"

    try:
        subprocess.run(
            command,
            cwd=stage_dir,
            check=True,
            env=env,
        )
    except FileNotFoundError as exc:
        raise SystemExit(
            "fastled CLI not found. Install it with `pip install fastled` before building the web demos."
        ) from exc


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


def build_site() -> None:
    reset_directory(STAGE_ROOT)
    reset_directory(DIST_ROOT)

    for sketch in SKETCHES:
        stage_dir = stage_sketch(sketch)
        run_fastled_compile(stage_dir)
        output_dir = find_build_output(stage_dir)
        shutil.copytree(output_dir, DIST_ROOT / sketch.name)

    shutil.copy2(LANDING_PAGE, DIST_ROOT / "index.html")


def main() -> int:
    build_site()
    return 0


if __name__ == "__main__":
    sys.exit(main())
