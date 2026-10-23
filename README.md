# SwapForceRecomp

An unofficial native Windows port of **Skylanders: Swap Force** (2013, Xbox 360), made with
static recompilation (ReXGlue). Work in progress: the tutorial and Woodburrow play, with swappers, on the emulated or a real portal.

> **Unofficial fan project.** Not affiliated with or endorsed by Activision or Vicarious Visions. This
> repository contains **no game data**: you need your own copy of the game.

## Status

| Area | State |
|---|---|
| Boot, menus, saving | ✅ logo movies (skippable), title screen, save slots, opening movie |
| Tutorial and Woodburrow | ✅ played through with controller or keyboard; cutscenes and dialogue |
| Rest of the game | 🟡 not played through yet |
| Frame rate | ✅ 60 FPS and more (game speed checked by playing), over 100 at 1440p in the tutorial |
| Audio | ✅ 5.1 as the game mixes it, or stereo |
| Portal | ✅ emulated and real USB (also the older PS3 / Wii portals); swappers |
| Portal menu | ✅ per player, a swapper builder, items, the game's pictures, hats and upgrade trees; player menu (L3 + R3) to reassign controllers |
| Ultrawide | ✅ the 3D view; the HUD at the edges is an experimental option |
| Installer tested on a clean PC | ⬜ planned |

Where each part stands and what comes next: [docs/roadmap.md](docs/roadmap.md); session details and
known bugs: [docs/STATUS.md](docs/STATUS.md). Part of six ports of the Xbox 360 Skylanders games,
started together from [Skylanders: Core of Light](https://github.com/tsapZ/SkylandersCoreOfLight).

## What it adds

- **Native Windows program**: the game's Xbox 360 code recompiled to x86-64 (no emulator of the
  CPU), the GPU through Xenia's D3D12 path.
- **Picture**: internal resolution 1x, 2x or 3x of the console's 720p, sharpened with AMD CAS,
  16x texture filtering, presets *Performance*, *Quality* (2x, recommended) and *Best*. The game's
  own 3D picture at its native size instead of the console's smaller one is an experimental
  option.
- **60 FPS** (what the installer sets); 30 as on the console, or 144, 200 and more.
- **Ultrawide**: a wider window shows more to the sides (the HUD at the edges is an experimental option here).
- **The Portal of Power** emulated (figures as `.dump` files), a real USB portal, or both at once.
  The **portal menu** (Guide, Back + Start or F6) is drawn in the game's own fonts with its own
  pictures of every figure, their hats and upgrade trees, per player, for controller and keyboard.
- **A settings menu** in the same style (F4 or Back + RB): graphics, frame rate, language, portal,
  controls with the game's controller picture.
- **Achievements**, shown in the portal menu and in the Core of Light launcher.
- English and German menus.

Screenshots are left out of this repository on purpose: they would show the games' artwork,
which belongs to Activision.

## Which disc

The European Xbox 360 disc with English and German (PAL, title `415608E9`). Its SHA-256 is pinned in
[docs/game/default.xex.sha256](docs/game/default.xex.sha256): the game, the installer and the launcher
check it. Other regions have another `default.xex` and are refused at start.

## Installing

- **Installer (beta):** `just package-installer` builds `installer/Output/SwapForceRecompSetup-<version>.exe`
  (Inno Setup; there are no prebuilt downloads yet). It installs into your user folder
  (`%LOCALAPPDATA%\Programs\Swap Force Recompiled`, no admin rights) and asks for your disc, as an ISO or an
  already extracted folder (it checks the version, then copies the files next to the game), the
  game language, the portal (emulated, real USB or none), the window size and the internal
  resolution. Everything else starts at the settings above (60 FPS, sharpening, 16x filtering) and is changed later in the F4 menu.
- **[Skylanders: Core of Light](https://github.com/tsapZ/SkylandersCoreOfLight)**, the launcher
  for all six ports, finds the installed game, sets up its disc from your ISO, keeps profiles with
  their own saves and figures and starts the game.
- **From source:** build it (below), put the extracted disc into `rom/` ([rom/README.md](rom/README.md))
  and run `just play-release`.

Your figures are `.dump` files in `Documents\Skylanders\Figures`, shared by every game, like real
figures next to the portal. The saves are in `Documents\swapforcerecomp`, the settings file next to the
game's exe.

## Building

Needs Windows, Visual Studio 2022 (C++ workload and C++ Clang tools), CMake, Ninja, just, Git and
Python 3. Run everything from the **x64 Native Tools Command Prompt for VS 2022**:

    git clone --recursive https://github.com/tsapZ/SwapForceRecomp.git
    cd SwapForceRecomp
    just regen-release      (code generation; needs the game's default.xex in rom/)
    just build-release
    just play-release

Details: [docs/development.md](docs/development.md) and [docs/build.md](docs/build.md); all docs in
[docs/README.md](docs/README.md).

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
