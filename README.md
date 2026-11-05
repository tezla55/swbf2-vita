# Star Wars Battlefront II — PS Vita (source)

Source for the native PS Vita port of **Star Wars: Battlefront II (2005)**. The playable build
reads the data files of your own PC copy at runtime. The source contains no game data; the
included VPK bundles the LiveArea art, the shader compiler and the Zeroes fonts for convenience.

- **Latest build:** [`swbf2-vita.vpk`](swbf2-vita.vpk)
- **User-facing beta page and release notes:** [tezla55/swbf2-vita-beta](https://github.com/tezla55/swbf2-vita-beta)

## Build

```sh
git clone --recursive https://github.com/tezla55/swbf2-vita.git
cd swbf2-vita
scripts/build_ffmpeg_bink.sh          # once: the Bink movie decoder
scripts/push_vita.sh                  # build -> build/swbf2render.vpk
scripts/push_vita.sh 192.168.0.7      # build, install and launch (VitaCompanion)
```

You need the Vita SDK (`VITASDK`) and Docker for the build image. The game data is not built in:
copy your PC `GameData` folder to `ux0:data/swbf2/GameData/` (the app reads
`ux0:data/swbf2/GameData/data/_lvl_pc/`). No game data is distributed with this repository.

## License

GPL-3.0 (see [LICENSE](LICENSE)). Star Wars Battlefront II and all of its content are the
property of their respective owners; this project is not affiliated with or endorsed by them.
