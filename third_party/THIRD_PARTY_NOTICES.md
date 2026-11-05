# Third-party components

| Component | Where | License | Use |
| --- | --- | --- | --- |
| [LibSWBF2](https://github.com/Ben1138/LibSWBF2) (our fork) | `libs/LibSWBF2` (git submodule) | GPL-3.0 | Reads the game's `.lvl` files: models, textures, animations, terrain, sounds, scripts |
| [lua5.0-swbf2-x64](https://github.com/Ben1138/lua5.0-swbf2-x64) | `libs/lua50` (vendored) | GPL-3.0 (see `libs/lua50/LICENSE`) | Runs the game's compiled mission scripts |
| FFmpeg 6.1.2 (libavformat, libavcodec, libavutil) | `third_party/ffmpeg-bink` (built by `scripts/build_ffmpeg_bink.sh`) | LGPL-2.1-or-later | Bink demux + decode for the shell's background movies |
| [vitaGL](https://github.com/Rinnegatamante/vitaGL), vitashark, FreeType, libpng, zlib, bzip2 | the vitasdk | their own licenses | Rendering, runtime shader compile, font rasterisation |

Because LibSWBF2 and the Lua fork are GPL-3.0, this project is GPL-3.0 as well (see `LICENSE`).

## FFmpeg

`scripts/build_ffmpeg_bink.sh` downloads the unmodified source
(https://ffmpeg.org/releases/ffmpeg-6.1.2.tar.xz) and builds static libraries with only the
Bink demuxer and decoders enabled; the exact `configure` line is in that script.

LGPL: the application links these libraries statically. To relink against a modified FFmpeg,
rebuild `third_party/ffmpeg-bink/` from modified source and rebuild the app (its full source is
in this repository).

## Not included

No game data is in this repository or in the VPK. Star Wars Battlefront II's files are read at
runtime from the user's own copy of the game (see `docs/game-data.md`). Sony's runtime shader
compiler (`libshacccg.suprx`) is not included either; see `docs/building.md`.

## Xelu's Free Controller Prompts

`third_party/xelu_prompts/` holds two PS Vita d-pad images (`Vita_Dpad_Left.png`, `Vita_Dpad_Right.png`) and the
pack's `Readme.txt`, from Nicolae "Xelu" Berbece's Free Controller & Key Prompts
(https://thoseawesomeguys.com/prompts/), released under CC0. `tools/gen_glyphs.py` merges them into
`src/glyphs_gen.h` for the hero prompt's d-pad glyph.
