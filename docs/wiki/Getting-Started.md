# Getting Started

This guide takes you from a fresh checkout to a composition running on your board. It covers the
**local** workflow — running the composer on your own machine — which is what unlocks saving
compositions and flashing hardware. If you only want to design and download scenes without installing
anything, use the **[hosted composer](https://pthomain.github.io/PolarShader/composer/)** instead (see
[Web Composer Guide](Web-Composer-Guide.md) for what differs).

## Prerequisites

- **Python 3.** The build and local server run under Python. `web/serve.sh` uses `.venv/bin/python` if
  present, otherwise `python3`.
- **Composer dependencies:** `pip install -r web/requirements.txt`. This installs the FastLED WASM CLI
  used to compile the composer sketch to WebAssembly. (Node.js is **not** required — the WASM toolchain
  is handled by the FastLED CLI, not npm.)
- **PlatformIO** — only needed for the deploy step. The deploy flow auto-detects `pio`, `platformio`, or
  `python -m platformio` on your `PATH`.

> The web build compiles against a pinned FastLED **master** commit (not a tagged release); see
> `web/build_site.py` for why. You don't need to install FastLED yourself for the web build.

## 1. Start the composer

```bash
web/serve.sh            # rebuild the site, then serve on port 8000
web/serve.sh 8080       # use a different port
web/serve.sh --no-build # serve the existing web/dist without rebuilding
```

`web/serve.sh` rebuilds `web/dist` (via `web/build_site.py`) and serves it through `web/local_server.py`
with the COOP/COEP headers the threaded WASM build needs. It also exposes the local playlist and deploy
API and writes playlists to `build/psc/`.

Then open:

```
http://localhost:8000/composer/
```

## 2. Pick your display

The composer renders one display geometry at a time. Choose it from the dropdown in the UI, or set it in
the URL with `?display=`:

```
http://localhost:8000/composer/?display=round
```

See [Displays](Displays.md) for the full list of layouts and their ids.

## 3. Compose a scene

Build up an effect in the composer panel:

1. Choose a **pattern** (the base procedural sampler).
2. Stack **transforms** (rotation, zoom, kaleidoscope, vortex, …) to warp it.
3. Drive parameters with **signals** (sine, noise, linear easing, …) for motion.
4. Pick a **palette** and adjust brightness.

The canvas updates live as you edit. See the [Web Composer Guide](Web-Composer-Guide.md) for a full tour,
and [Core Concepts](Core-Concepts.md) for how patterns, transforms, and signals fit together.

## 4. Save to your playlist

Click **Add to playlist** to write the current scene as a `.psc` file into `build/psc/`. This requires
the local server (the hosted build can only download individual scenes or load bundles in-browser).

A playlist is just the set of `.psc` files in `build/psc/`. It's what gets embedded into firmware when
you deploy.

## 5. Deploy to your board

1. Connect your board over USB.
2. Select your **target** in the composer (e.g. Seeed XIAO RP2040 Round).
3. Click **Deploy**. The server runs PlatformIO (`pio run -e <env> -t upload`), streams the build/upload
   log back to the UI, and reports success or failure.

For the list of targets, the deploy mechanics, and a command-line alternative, see
[Deploying to Hardware](Deploying-to-Hardware.md).
