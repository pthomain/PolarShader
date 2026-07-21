# PDS Format

PDS ("Polar Display Spec") is the binary display format used by the web composer and the WASM/native renderer to describe a **display**: its per-LED polar layout, an optional raster grid, and deployment/output configuration.

A `.PDS` file is the intended **source of truth for MCU deployment**, but flashing still needs a deploy tool that reads `.PDS` → generates build-time config/headers → compiles → uploads. FastLED/SmartMatrix drivers take compile-time/template config that cannot be changed by loading bytes into already-flashed firmware, so runtime loading (web/native preview) and on-hardware deployment are separate concerns (see [Deployment](#deployment)).

The supported runtime format is **PDS v1**.

## Byte Order

All multi-byte integers are little-endian.

## File Layout

```text
Header:
  offset  size  field
  0       4     magic          "PDS\0" (0x50 0x44 0x53 0x00)
  4       1     version        0x01
  5       1     section_count

Each section (repeated section_count times):
  0       1     section_type   (enum below)
  1       1     section_flags  (bit0 = CRITICAL; bits1..7 reserved, MUST be 0)
  2       4     body_len       u32
  6       ...   body           body_len bytes
```

Sections are length-prefixed, so an older reader can skip any section it does not recognise — **unless** the section's `CRITICAL` flag (bit0) is set. An unknown *critical* section is a hard decode error. All four v1 sections are written `CRITICAL=1` (they change geometry/rendering), so a v1 file never silently degrades on an older reader. Purely additive future metadata should be written `CRITICAL=0`.

### Section rules

- **Order:** the *known* sections, when present, MUST appear in ascending `section_type` order: `DISPLAY_INFO(0x01)` → `GEOMETRY_POLAR(0x02)` → `RASTER_GRID(0x03)` → `OUTPUT_CONFIG(0x04)`. This guarantees `GEOMETRY_POLAR.led_count` is known before `RASTER_GRID` needs it. Unknown non-critical sections may appear anywhere and are skipped by `body_len`; they do not affect the known-section ordering check.
- **Singletons:** each of the four known sections may appear at most once. A duplicate is rejected.
- **Exact consumption:** for a known section the parser must consume its body exactly — leftover bytes inside `body_len` are rejected. Only unknown non-critical sections may carry opaque trailing bytes (skipped whole).
- **Trailing bytes** after the last section are rejected.
- `GEOMETRY_POLAR` is **required**; a file missing it is rejected.
- Reserved section-flag bits set → reject.

## Sections

### `0x01 DISPLAY_INFO` (recommended, singleton)

```text
source_kind  u8    0=custom, 1=round, 2=fibonacci, 3=fabric, 4=fabric32x8, 5=matrix128
name_len     u8
name         name_len bytes, UTF-8
```

### `0x02 GEOMETRY_POLAR` (required, singleton)

```text
led_count    u16   1..4096 (reject 0)
per LED (led_count times):
  angle_u0x16  u16   16-bit modular angle (0..65535 = full turn)
  radius_u0x16 u16
```

### `0x03 RASTER_GRID` (optional, singleton, all-or-nothing)

```text
raster_width   u16
raster_height  u16
per LED (exactly led_count times, same order as GEOMETRY_POLAR):
  raster_x     u16
  raster_y     u16
```

Presence of this section means "display supports raster". It is **all-or-nothing**: either there is no `RASTER_GRID` (no LED has raster data) or it carries a full per-LED table. Validation: `raster_width > 0`, `raster_height > 0`, every `raster_x < raster_width`, `raster_y < raster_height`, and `raster_width * raster_height <= 4096` (`POLAR_SHADER_MAX_RASTER_CELLS`).

### `0x04 OUTPUT_CONFIG` (optional, singleton; deployment source of truth)

```text
backend_kind u8    0=NONE, 1=FASTLED, 2=SMARTMATRIX, 3.. reserved/future
+ backend body (below)
```

Absent `OUTPUT_CONFIG` decodes as `backend_kind = NONE` (valid for preview/web-only). All five generated built-ins include a concrete backend (never `NONE`).

Enums (`chipset`, `rgb_order`, `panel_type`) use **stable PDS enum numbers**, decoupled from FastLED/SmartMatrix internal values, so a future library renumber cannot corrupt existing `.PDS`. Append, never renumber:

- `PDS_CHIPSET`: `0=WS2812`, `1=WS2812B`, `2=SK6812`, `3=APA102`, …
- `PDS_RGB_ORDER`: `0=RGB`, `1=RBG`, `2=GRB`, `3=GBR`, `4=BRG`, `5=BGR`
- `PDS_PANEL_TYPE`: `0=HUB75_32x32`, `1=HUB75_64x32`, `2=HUB75_64x64`, …

Bitmask fields (`matrix_options`, `background_options`, `color_correction`) are stored as PDS-versioned raw payloads; the deploy tool owns their interpretation.

`target_id` is a `u16`-length-prefixed UTF-8 string (note: `DISPLAY_INFO.name` uses a `u8` prefix; `target_id` uses `u16` for headroom).

**FASTLED body:**

```text
target_id        u16-prefixed UTF-8   platformio env / board id
chipset          u8                   PDS_CHIPSET
data_pin         u16                  0xFFFF = none
clock_pin        u16                  0xFFFF = none
rgb_order        u8                   PDS_RGB_ORDER
brightness       u8
refresh_ms       u8
color_correction u32                  packed RGB, 0 = default
```

**SMARTMATRIX body:**

```text
target_id          u16-prefixed UTF-8
panel_width        u16
panel_height       u16
chain_width        u16
chain_height       u16
subsample          u8
refresh_depth      u8
dma_buffer_rows    u8
panel_type         u8    PDS_PANEL_TYPE
matrix_options     u32
background_options u32
brightness         u8
refresh_ms         u8
```

`NONE` body is empty. For an **unknown backend** (`backend_kind >= 3`), the whole backend body is preserved verbatim in `OutputConfig.rawBody`, so `encode(decode(x))` is byte-identical; it is treated as non-deployable.

## Validation Layers

There are two deliberately separate layers:

**(a) General decode** — enforced by every codec (C++, JS, Python). Byte-format rules (sections, order, singletons, flags, exact consumption), raster all-or-nothing, dim/coord bounds, cell limit, enum ranges, and per-backend structural validity:

- **FASTLED:** `data_pin != 0xFFFF`. Clock-based chipsets (`APA102`) require `clock_pin != 0xFFFF`; clockless chipsets (`WS2812*`, `SK6812`) require `clock_pin == 0xFFFF`.
- **SMARTMATRIX:** `panel_width/panel_height/chain_width/chain_height > 0`, `subsample >= 1`, `refresh_depth > 0`, `dma_buffer_rows > 0`. When `RASTER_GRID` is present, the raster grid is the post-subsample render resolution derived from the full physical size:

  ```text
  physical_w = panel_width  * chain_width
  physical_h = panel_height * chain_height
  physical_w % subsample == 0  &&  physical_h % subsample == 0
  raster_width  == physical_w / subsample
  raster_height == physical_h / subsample
  ```

  e.g. Matrix128 = `(64*2)/2 = 64` → 64×64.

**(b) Deployability** — a *separate* check (`validateDeployability` in C++ / `validate_deployable` in `pds_v1.py` `--deploy` mode) that the general decoder never calls. It rejects:

- `backend_kind == NONE` (nothing to deploy).
- unknown backend (`backend_kind >= 3`) — decodes/previews fine but has no generator.
- SmartMatrix without a *dense, complete* raster (`led_count == raster_width * raster_height` and every cell covered exactly once).

A sparse/no-raster/unknown-backend `.PDS` still decodes OK and previews; only deployability flags it.

## Deployment

`.PDS` is the source of truth, but MCU deployment cannot feed a `LoadedDisplaySpec` straight into today's drivers. `FastLedDisplay` hardcodes the `WS2812` chipset and reads static `SPEC::LED_PIN`/`SPEC::RGB_ORDER`; `SmartMatrixDisplay` requires a `MatrixDisplaySpec&` and file-scope `constexpr` allocation constants. A follow-up deploy tool therefore must read `.PDS` → **generate** a compile-time wrapper spec (geometry table + pin/order + panel dims), a specialised driver instantiation (chipset for FastLED; alloc/panel constants for SmartMatrix), and the build wiring → compile → upload. `LoadedDisplaySpec` is a web/native preview + codec vehicle, not an on-hardware driver.

## Loading A Custom Display (web composer)

The composer's **Load display** button reads a `.pds` file, decodes it, and swaps the active display in place (no page reload). The custom display is persisted in `sessionStorage` and restored on reload; picking a built-in from the selector clears it. If the loaded display has no raster grid, raster patterns are hidden and a raster scene is replaced with a UV-safe default (a raster pattern on a non-raster display renders black).

## Codec Implementations

The sectioned byte layout is the contract; the three codecs must stay in lockstep:

- C++: `src/display/DisplaySpecCodec.h` (+ `src/display/LoadedDisplaySpec.h`)
- Web: `web/sketches/composer/pds-codec.js`
- Python validator: `scripts/pds_v1.py`

## Validation And Tests

- `pio test -e native_composer` — C++ round-trip (polar + raster + output config), rejection cases, unknown-backend byte-identity, and the deployability layer.
- `node --test web/test_pds_codec.mjs` — web decode/encode against the `displays/*.pds` fixtures, rejection cases, and deployability.
- `python -m unittest scripts.test_pds_v1` — Python general-decode + deployability against the same fixtures.

The five built-in fixtures in `displays/` are generated by `pio run -e native_export_pds && .pio/build/native_export_pds/program`.
