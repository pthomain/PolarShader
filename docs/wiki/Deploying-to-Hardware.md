# Deploying to Hardware

Deploying flashes your current playlist (the `.psc` files in `build/psc/`) onto a microcontroller. It
requires the **local** composer (`web/serve.sh`) and PlatformIO — the hosted GitHub Pages build cannot
deploy.

## Targets

Each target is a PlatformIO environment bound to a fixed display and firmware entry point:

| PlatformIO env | Board | Display | Entry point |
|----------------|-------|---------|-------------|
| `seeed_xiao` | Seeeduino XIAO (SAMD21) | Fabric (20×20) | `main_samd.cpp` |
| `seeed_xiao_rp2040_fabric` | Seeed XIAO RP2040 | Fabric (20×20) | `main_rp2040_fabric.cpp` |
| `seeed_xiao_rp2040_round` | Seeed XIAO RP2040 | Round (241 px) | `main_rp2040_round.cpp` |
| `seeed_xiao_rp2040_matrix32x8` | Seeed XIAO RP2040 | Fabric 32×8 | `main_rp2040_fabric32x8.cpp` |
| `seeed_xiao_rp2040_fibonacci` | Seeed XIAO RP2040 | Fibonacci (324 px) | `main_rp2040_fibonacci.cpp` |
| `teensy41_matrix` | Teensy 4.1 | SmartMatrix 128×128 | `main_teensy.cpp` |

Notes:

- The env name `seeed_xiao_rp2040_matrix32x8` maps to the `main_rp2040_fabric32x8.cpp` entry point.
- `platformio.ini` also defines a shared `seeed_xiao_rp2040_base` env and `native*` envs (desktop unit
  tests / tools). These are **not** deploy targets — use the six above.

## Deploy from the composer

1. Run `web/serve.sh` and open the composer.
2. Connect your board over USB.
3. Select your **target** and click **Deploy**.

The server runs PlatformIO for that environment and streams the build/upload log back to the UI, polling
status until it succeeds, fails, or times out.

## Deploy from the command line

The composer's deploy is a thin wrapper around PlatformIO. You can run the same thing directly:

```bash
pio run -e seeed_xiao_rp2040_round -t upload
```

Substitute any env from the table above.

## How it works

- **Entry point selection.** Each env sets a `build_src_filter` in `platformio.ini` that compiles exactly
  one `main_*.cpp` and excludes the others, so a single source tree produces per-board firmware.
- **Playlist embedding.** Every deploy env runs `pre:scripts/generate_psc_playlist.py` before building.
  That script reads the `.psc` files from `build/psc/` and embeds them into the firmware binary, so your
  saved compositions ship on the device.
- **Deploy command.** The local server invokes `pio run -e <env> -t upload` in the repo root and reports
  progress; see `web/local_server.py` for the exact orchestration.
