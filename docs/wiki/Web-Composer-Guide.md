# Web Composer Guide

The composer is a browser app that runs the PolarShader engine compiled to WebAssembly. You build scenes
visually and preview them live on a canvas that mirrors your chosen display geometry.

There are two ways to run it:

- **Hosted** — [pthomain.github.io/PolarShader/composer/](https://pthomain.github.io/PolarShader/composer/).
  No install. Good for designing, downloading, and loading scenes in the browser.
- **Local** — `web/serve.sh`, served at `http://localhost:8000/composer/`. Adds the local server API, so
  you can save playlists to `build/psc/` and deploy to hardware. See [Getting Started](Getting-Started.md).

See [Hosted vs local](#hosted-vs-local) below for exactly what differs.

## Selecting a display

The composer renders one display at a time. Pick it from the dropdown, or set it via the `?display=`
query parameter (legacy alias `?ps_display=`):

| id | `?display=` values | Display |
|----|--------------------|---------|
| 0 | `fabric` (default) | Fabric (20×20 matrix) |
| 1 | `round`, `polar`, `1` | Round (241-pixel radial) |
| 2 | `fabric32x8`, `2` | Matrix (32×8) |
| 3 | `smartmatrix`, `matrix128`, `3` | SmartMatrix (128×128 HUB75) |
| 4 | `fibonacci`, `4` | Fibonacci (324-pixel phyllotaxis) |

Changing the display reloads the page with the new query string. Display geometry itself is defined in
C++ at compile time — see [Displays](Displays.md).

## Building scenes

A scene is a layered composition. In the composer you:

1. **Pick a pattern** — the base procedural sampler (noise, cellular automata, ShaderToy ports, …). See
   [Patterns](Patterns.md).
2. **Stack transforms** — geometric warps applied in order (rotation, zoom, translation, vortex,
   kaleidoscope, tiling, flow field) plus palette operations. See [Transforms](Transforms.md).
3. **Drive parameters with signals** — most parameters accept a signal (sine, noise, triangle, square,
   sawtooth, linear/quadratic easing, constant) so they animate over time. See [Signals](Signals.md).
4. **Choose a palette and brightness.**

The canvas updates on every edit. Under the hood each change re-encodes the scene and pushes it to the
WASM renderer.

## Playlists

A playlist is the collection of `.psc` scene files that will be embedded into firmware. Playlist actions
differ by mode:

- **Add to playlist** — saves the current scene into `build/psc/` (local server only).
- **Load / Replace** — load a playlist bundle (a JSON file containing base64-encoded `.psc` entries).
  On the hosted build this populates an in-browser, read-only playlist held in memory.
- **Download** — export the current playlist as a bundle you can share or re-load later.
- **Clear** — remove all compositions (with a confirmation prompt). Locally this empties `build/psc/`; in
  hosted mode it clears the in-browser playlist.

The `.psc` files use the [PSC binary format](https://github.com/pthomain/PolarShader/blob/main/docs/psc-format.md).

## Embed mode

Append `?embed=1` to render **only** the animated canvas — no editor panel, menu bar, title, or debug
console — so the composer can be dropped into an `<iframe>` on another page:

```html
<iframe src="https://pthomain.github.io/PolarShader/composer/?embed=1"
        width="480" height="480" style="border:0" allow="autoplay"></iframe>
```

Embed mode combines with `?display=`, e.g. `?embed=1&display=round`. Any value other than `0`/`false`
enables it.

## Hosted vs local

| Capability | Hosted (GitHub Pages) | Local (`web/serve.sh`) |
|------------|-----------------------|------------------------|
| Design & preview scenes | ✅ | ✅ |
| Download a scene / playlist bundle | ✅ | ✅ |
| Load a playlist bundle (in-browser) | ✅ (read-only) | ✅ |
| **Add to playlist** (`build/psc/`) | ❌ | ✅ |
| Per-item edit / delete in `build/psc/` | ❌ | ✅ |
| **Deploy** to a board | ❌ | ✅ |
| Persistence | in-browser only (lost on reload) | `.psc` files on disk |

The static GitHub Pages build has no server, so anything that writes `build/psc/` or flashes hardware is
disabled there — the UI shows hints like *"Run web/serve.sh locally…"* and *"available only from
web/serve.sh"*. To save and deploy, run the composer locally.
