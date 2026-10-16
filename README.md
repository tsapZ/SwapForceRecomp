# SwapForceRecomp

An unofficial native Windows port of **Skylanders: Swap Force** (Xbox 360), made with static
recompilation through ReXGlue. Planned: 4K and ultrawide, a virtual and a real USB Portal of
Power, and a settings overlay.

> **Unofficial fan project.** It is not affiliated with or endorsed by Activision, Toys for Bob
> or Microsoft. This repository contains **no game data**: you need your own copy of the game.

## Status

Early but playable: the game boots, menus, saving and the tutorial level work, including cutscenes,
German and English, the emulated portal (F6) and real USB portals (also older Spyro's Adventure/Giants
portals). Experimental 60 FPS and more (`present_interval`). Details and known issues in
[docs/STATUS.md](docs/STATUS.md).

## Building

Needs Windows, Visual Studio 2022 (C++ workload and C++ Clang tools), CMake, Ninja, just, Git and
Python 3. Run everything from the **x64 Native Tools Command Prompt for VS 2022**.

    git clone --recursive https://github.com/tsapZ/SwapForceRecomp.git
    cd SwapForceRecomp
    just --list

Code generation needs the game's `default.xex` in `rom/` (see [rom/README.md](rom/README.md)).
Full build steps follow once the first build works.

## Credits

- **[GiantsRecomp](https://github.com/TheBiemGamer/GiantsRecomp)** by TheBiemGamer brought
  Skylanders Giants to PC and is the template for this project: build setup, tooling and workflow
  (MIT).
- **[ReXGlue SDK](https://github.com/rexglue/rexglue-sdk)** by Tom Clay is the recompiler and
  runtime everything here is built on. It comes from the [Xenia](https://xenia.jp) emulator (Ben
  Vanik and contributors) and is inspired by [XenonRecomp](https://github.com/hedge-dev/XenonRecomp)
  by hedge-dev. Used through TheBiemGamer's fork as a submodule; keeps its own license.
- **[skylanders-portal](https://github.com/TheBiemGamer/skylanders-portal)** by TheBiemGamer
  provides the portal protocol, software and USB portal, overlay and figure catalog. Submodule;
  keeps its own license.
- **[extract-xiso](https://github.com/XboxDev/extract-xiso)** is included (prebuilt, unmodified) in
  `installer/` to unpack an Xbox 360 ISO. It keeps its own license
  (`installer/extract-xiso.LICENSE.txt`).
- Figure save-data decoding builds on public reverse-engineering of the Skylanders NFC format,
  among others **[SkyReader](https://github.com/reedstrm/SkyReader)**.

## License

The code and documentation in this repository are released under the [MIT license](LICENSE). That
doesn't cover the game or its data, which you need to own and supply yourself, or the third-party
components in `thirdparty/`, which keep their own licenses.
