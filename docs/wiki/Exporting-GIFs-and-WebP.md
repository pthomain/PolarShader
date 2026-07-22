# Exporting GIFs and WebP

PolarShader can turn any composition into a seamlessly looping animation (GIF, WebP, or animated
PNG) that matches hardware output. This is how the README hero banner and the pattern previews are
produced.

Export is a two-stage pipeline:

1. **`psc_render`** — a native desktop build that samples a `.psc` composition over a `.pds` display
   through the real C++ pipeline and dumps raw per-LED frames.
2. **`scripts/render_gif.py`** — a Python assembler that draws each LED as a glowing dot and encodes
   the frames into a looping GIF / WebP / APNG.

Splitting the two means the heavy pixel math runs once in C++ (identical to firmware), and you can
re-encode into different formats and styles without re-rendering.

## Prerequisites

- PlatformIO (for `psc_render`), same toolchain as the native unit tests.
- Python with `pillow` and `numpy`:

  ```bash
  python3 -m pip install pillow numpy
  ```

## Stage 1 — sample frames with `psc_render`

Build the native tool and run it against a composition and a display geometry:

```bash
pio run -e native_psc_render && \
  .pio/build/native_psc_render/program \
    tools/gif/hero.psc displays/fibonacci.pds build/gif/ \
    --period-ms 10000 --fps 25 --require-loop
```

Arguments:

| Argument | Default | Meaning |
|----------|---------|---------|
| `<composition.psc>` | — | The scene to render (see [PSC Format](https://github.com/pthomain/PolarShader/blob/main/docs/psc-format.md)). |
| `<display.pds>` | — | Display geometry (e.g. `displays/fibonacci.pds`, `displays/round.pds`). |
| `<out_dir>` | — | Where the frame data is written. |
| `--period-ms` | `10000` | Loop length in milliseconds. |
| `--fps` | `25` | Frames sampled per second (must be `>= 10`). |
| `--require-loop` | off | Fail if the composition does not wrap seamlessly at the loop seam. |

It writes three files into `<out_dir>`:

- `geometry.csv` — `led_index,x,y` with coordinates normalised to `[-1, 1]`.
- `frames.bin` — `N x ledCount x 3` bytes of raw RGB, ordered by `(frame, led)`.
- `meta.json` — `fps_eff`, `period_ms`, `ledCount`, `frame_ms`, `frames`.

> Don't have a composition yet? `node tools/gif/build_hero_psc.mjs` writes the default hero scene to
> `tools/gif/hero.psc`, or export your own `.psc` from the [Web Composer](Web-Composer-Guide.md).

## Stage 2 — assemble with `render_gif.py`

Point the assembler at the `psc_render` output directory; the output format is inferred from the
`-o` extension (`.webp`, `.png`/`.apng`, otherwise GIF):

```bash
python3 scripts/render_gif.py build/gif/ -o media/polarshader.webp
```

### Output formats

| Format | Extension | Notes |
|--------|-----------|-------|
| `webp` | `.webp` | 24-bit colour + 8-bit alpha, transparent background. Smallest and highest quality; renders on GitHub. Recommended. |
| `apng` | `.png` / `.apng` | Lossless RGBA with transparency; wider fallback than WebP. |
| `gif` | `.gif` | 256-colour palette on an opaque black background. Legacy default. |

Use `--format` to override the inferred format.

### Common options

| Option | Default | Meaning |
|--------|---------|---------|
| `--style {dots,glow,disc}` | `dots` | `dots`: hard opaque LEDs; `glow`: soft additive bloom; `disc`: solid core + alpha fade (reads on any background). |
| `--size` | `640` | Canvas edge in pixels. |
| `--dot-radius` | `14` | LED sprite radius in pixels. |
| `--gamma` | `1.8` | Output gamma (bloom strength). |
| `--brightness` | `1.0` | Linear gain before tone-mapping. |
| `--full-value` | off | Normalise every lit LED to full brightness. |
| `--alpha-boost` | `3.0` | Alpha gain so dot bodies stay fully opaque (WebP/APNG). |
| `--round-bg` | off | Fill an opaque black round background disc; transparent outside the rim. |
| `--quality` | `80` | WebP quality (0-100, lossy). |
| `--lossless` | off | WebP lossless mode. |
| `--colors` | `128` | GIF palette depth (`<= 256`). |

Run `python3 scripts/render_gif.py --help` for the full list, including the hero-banner options
(`--banner-title`, `--pill-radius-frac`, `--shader-noise`, ...) used to bake the README banner.

### Looping and timing

The loop is seamless because the field itself loops (the assembler never cross-fades). Frame delays
are stored in centiseconds (10 ms), so playback timing is exact only when `meta.json`'s `frame_ms`
is a whole multiple of 10 ms — e.g. `10000 ms / 25 fps = 40 ms`. Choose `--period-ms` / `--fps` in
stage 1 accordingly; the assembler warns if the total drifts from the sampled period.
