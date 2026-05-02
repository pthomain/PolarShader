# PolarShader Web Demos

Build the browser demos locally with:

```bash
python -m pip install -r web/requirements.txt
python web/build_site.py
```

The script stages self-contained sketches under `web/.stage/`, resolves a pinned FastLED `3.10.3` source tree under `web/.fastled/`, compiles with `fastled --just-compile --release`, and writes the deployable site to `web/dist/`.
