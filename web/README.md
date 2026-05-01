# PolarShader Web Demos

Build the browser demos locally with:

```bash
pip install fastled
python web/build_site.py
```

The script stages self-contained sketches under `web/.stage/`, compiles them with `fastled --just-compile --release`, and writes the deployable site to `web/dist/`.
