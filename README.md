# SwapForceRecomp

An unofficial native Windows port of **Skylanders: Swap Force** (Xbox 360), made with static
recompilation through ReXGlue. Planned: 4K and ultrawide, a virtual and a real USB Portal of
Power, and a settings overlay.

> **Unofficial fan project.** It is not affiliated with or endorsed by Activision, Toys for Bob
> or Microsoft. This repository contains **no game data**: you need your own copy of the game.

## Status

First level played through. Where each part stands, and what comes next:
**[docs/roadmap.md](docs/roadmap.md)**. Session details and known bugs: [docs/STATUS.md](docs/STATUS.md).

| Area | State |
|---|---|
| Boot, menus, saving | ✅ works, logo movies skipped |
| First level | ✅ played through with keyboard, controller works too; cutscenes included |
| Graphics | ✅ correct with the fast RTV path; 4K output works |
| Frame rate | ✅ 60 FPS and more (`present_interval = 1` + `frame_rate_limit`): 100+ FPS in the first level at 1440p, ~80 in the hub |
| Portal | ✅ emulated and real USB, also older portals; swappers work; portal menu for controller and keyboard (Guide / Back+Start / hold Back / F6), per player, with items, a swapper builder, options and a way out for player 2; player menu (L3 + R3) to reassign controllers |
| Languages | ✅ English and German |
| Rest of the game | 🟡 not played through yet |
| Picture quality | 🟡 3D scene renders at 1040x620 and is scaled up; sharper native scene planned |
| Settings menu | ✅ in the portal menu's style, for controller and keyboard (F4 / Back + RB): presets, resolution, sharpening, frame rate, portal, controls |
| Ultrawide | 🟡 the 3D view widens for 21:9 and wider; the HUD is still stretched |
| Installer tested on a clean PC | ⬜ planned |

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
