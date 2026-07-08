# PSC Format

PSC is the binary scene format used by the web composer, the WASM renderer, and the embedded MCU playlist.

The supported runtime format is **PSC v1**. Older PSC files are not supported by the production codecs; they must be manually re-saved or rebuilt as v1 before they are loaded or embedded.

## Byte Order

All multi-byte integers are little-endian.

## File Layout

```text
offset  size  field
0       4     magic        "PSC\0" (0x50 0x53 0x43 0x00)
4       1     version      0x01
5       1     palette_id   composer palette id
6       ...   pattern      record(tag, body_len, body)
...     1     transform_n  transform count
...     ...   transforms   transform_n records
```

Every pattern, transform, and signal is encoded as a length-prefixed record:

```text
record:
  tag       u8
  body_len  u16
  body      body_len bytes
```

Signals are recursive. A pattern or transform body may contain signal records, and a signal body may contain other signal records.
The browser decoder, runtime decoder, and shared Python validator all cap nesting depth at 64 records.

Example: `Noise - Basic` with `constant(550)` and a `Zoom` transform with `constant(200)`:

```text
50 53 43 00        magic "PSC\0"
01                 version
00                 palette id

00 05 00           pattern tag 0x00, body length 5
  00 02 00         signal tag 0x00, body length 2
    26 02          constant permille 550

01                 one transform

02 05 00           transform tag 0x02, body length 5
  00 02 00         signal tag 0x00, body length 2
    c8 00          constant permille 200
```

## Body Layouts

Record bodies are schema-defined:

- Pattern bodies encode static config fields first, then signal slots.
- Transform bodies encode static config fields first, then signal slots.
- Signal bodies encode that signal type's parameters.

The authoritative tag tables are:

- C++ decoder: `src/composer/SceneCodec.cpp`
- Web schema/encoder: `web/sketches/composer/schema.js` and `web/sketches/composer/codec.js`

These tables must stay in lockstep.

## Tag Stability Rules

PSC files identify patterns, transforms, and signals by numeric tag. These tags are part of the file format.

Rules:

- Never renumber an existing tag.
- Never reuse a removed tag for a different meaning.
- Append new tags for new concepts.
- If an existing body layout changes incompatibly, allocate a new tag instead of changing the old tag's body.
- Hidden/deprecated tags may remain decodable, but they must keep their old semantics.

Version changes are for wrapper-level format changes. Incompatible record body changes should normally be handled by a new tag.

## Why v1 Is More Robust Than v0

PSC v0 inferred record sizes entirely from the current schema. If JS and C++ drifted, or if a pattern body changed shape, the decoder could become misaligned and read later bytes as the wrong signal, transform count, or transform tag.

PSC v1 adds explicit record boundaries:

- The decoder knows exactly where each pattern, transform, and signal ends.
- Extra bytes inside a record are rejected instead of being silently consumed by the next field.
- Truncated bodies fail at the record that is actually broken.
- Recursive signals cannot misalign the rest of the scene when one nested signal is malformed.

This does not remove the need for tag discipline, but it makes codec drift fail early and predictably.

## Validation And Tests

Codec coverage is split across C++ and JS:

- `pio test -e native_composer` exercises the C++ decoder, tag table coverage, malformed inputs, v1 record boundaries, raster-transform rejection, legacy v0 rejection, and embedded playlist decode behavior.
- `node --test web/test_codec.mjs` exercises the web encoder/decoder, exact v1 golden bytes, schema tag uniqueness, round-trips, unsupported version rejection, and malformed length-prefixed records. The GitHub Pages workflow runs this before building the static site.
- `web/test_local_server.py` exercises the shared Python validator against current cross-implementation fixtures, including XOR, Conway, paletteClip, and nested signal graphs.
- `scripts/generate_psc_playlist.py` and `web/local_server.py` run that same shared validator before playlist files are saved or embedded.

Run both codec tests after changing `SceneCodec.cpp`, `SceneCodec.h`, `schema.js`, or `codec.js`.
