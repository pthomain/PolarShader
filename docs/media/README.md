# Documentation media

Drop documentation GIFs/images here. The README and wiki reference these paths in HTML comments; once a
file exists, convert its comment into a real `![alt](path)` image tag.

## Expected assets

| File | Used in | What it should show |
|------|---------|---------------------|
| `banner.webp` | `README.md` (top) | Hero banner: a scene animating on a round display beside the baked-in "PolarShader" title. |
| `composer-ui.gif` | `README.md` (See it in action) | Editing patterns/transforms/signals live in the composer panel. |
| `display-round.gif` | `README.md` (See it in action) | A scene rendered on the 241-pixel round display. |
| `display-matrix.gif` | `README.md` (See it in action) | A scene rendered on a matrix display (e.g. 32×8 or 128×128). |

## Guidelines

- Prefer short, looping GIFs (or MP4/WebM if the docs later switch to `<video>`).
- Keep file sizes reasonable (a few MB each) so the README stays quick to load.
- Capture from the composer canvas so the aspect ratio matches the target display.

Until an asset is added, its reference stays an HTML comment so nothing renders as a broken image.
