# PolarShader Web Demos

Build the browser demos locally with:

```bash
python -m pip install -r web/requirements.txt
python web/build_site.py
```

The script stages self-contained sketches under `web/.stage/`, resolves a pinned FastLED `3.10.4` source tree under `web/.fastled/`, compiles with `fastled --just-compile --release`, and writes the deployable site to `web/dist/`.
The pinned requirements install the FastLED CLI plus the Meson/uv tools needed by FastLED's WASM build.

## Environment overrides

`build_site.py` honours three optional environment variables so external orchestrators (e.g. a downstream repo that consumes PolarShader as a submodule) can drive the build without dirtying the PolarShader checkout:

- `POLARSHADER_WORK_ROOT_OVERRIDE` — when set, reparents the four write roots (`.stage/`, `.home/`, `.fastled/`, `dist/`) under this directory instead of `web/`.
- `POLARSHADER_SKETCHES_OVERRIDE` — comma-separated list of sketch names to build. Defaults to all of `fabric,round,composer`. Unknown names abort the build.
- `POLARSHADER_FASTLED_VERSION_OVERRIDE` — pins a different FastLED release tag (read before `FASTLED_ARCHIVE_URL` / `FASTLED_LIBRARY_ROOT` are derived).
