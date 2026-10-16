# Architecture

How SwapForceRecomp is put together. See also [build.md](build.md) (codegen fixes),
[portal.md](portal.md) (Portal of Power) and [reverse-engineering.md](reverse-engineering.md).

## Repository layout

```
SwapForceRecomp/
  rom/                           your own game dump, git-ignored (except rom/README.md)
  swapforcerecomp_manifest.toml  ReXGlue project manifest
  config/default.toml            codegen fixes for default.xex (included by the manifest)
  generated/                     codegen output, git-ignored (rexglue.cmake is tracked)
  src/
    main.cpp                     REX_DEFINE_APP entry point
    swapforcerecomp_app.h        the app class: lifecycle hooks, portal, overlay, XEX check
    xex_verify.*                 SHA-256 check of rom/default.xex at startup
    localization.*               `language` cvar
    game/kernel_stubs.cpp        kernel/XAM imports the runtime does not implement
    hooks/                       midasm hooks (currently disabled Giants templates)
    tools/                       xex check command-line tool (used by the installer)
  tests/                         unit tests (XEX verification, CLI)
  tools/                         headless Ghidra scripts, TOML sync
  installer/                     Inno Setup script, redistributables, extract-xiso
  thirdparty/
    rexglue-sdk/                 ReXGlue SDK (submodule, TheBiemGamer fork)
    skylanders-portal/           portal emulation and F6 overlay (submodule)
  docs/                          this wiki
```

## From disc to executable

```
rom/default.xex
   │  swapforcerecomp_codegen   (ReXGlue codegen; reads the manifest and config/default.toml)
   ▼
generated/default/*.cpp          one C++ function per guest function, sub_<address> or named
   │  CMake + Ninja + clang-cl  (together with src/ and both submodules)
   ▼
swapforcerecompiled.exe          native x64 program; loads game data from rom/ at runtime
```

Codegen needs `rom/default.xex`. Without it CMake can still configure, but there is no game code
to compile. After codegen, CMake has to be re-run so it picks up the generated files; the
`just regen-*` recipes do that.

Generated code is a mechanical translation. When it is wrong (missed function boundaries, jump
tables without bounds checks), the fix goes into `config/default.toml`, never into `generated/`.

## App lifecycle

`src/main.cpp` registers `SwapforcerecompApp` with `REX_DEFINE_APP`. The class in
`src/swapforcerecomp_app.h` derives from `rex::ReXApp` and overrides these hooks:

| Hook | What it does |
|---|---|
| `OnPreSetup` | adjusts the runtime configuration |
| `OnPostInitLogging` | runs `VerifyXex` against the expected SHA-256 and refuses wrong dumps |
| `OnConfigurePaths` | sets up the user data folder (default `Documents\swapforcerecomp`) |
| `OnPostSetup` | `skylanders::InstallPortal(...)`: creates the portal backend |
| `OnConfigureFonts`, `OnConfigureStyle` | ImGui look for the overlays |
| `OnCreateDialogs` | `skylanders::RegisterPortalOverlay(...)`: binds F6 |

The expected SHA-256 is read from `docs/game/default.xex.sha256` at configure time and compiled
in as `SWAPFORCERECOMP_XEX_SHA256`. CMake stops with an error if that file does not start with 64
hex characters.

## Game-specific code

- **Midasm hooks** (`src/hooks/`): declared at a guest address in `config/default.toml`, implemented
  in C++. `intro_skip_hook.cpp` is ported (hook in BinkOpen at `0x83081EB8`); `ultrawide_hook.cpp`
  is still a disabled Giants template (`#if 0`).
- **Kernel stubs** (`src/game/kernel_stubs.cpp`): `REX_EXPORT_STUB_RETURN(Import, value)` for imports
  the runtime lacks, each with a comment why returning that value is safe.

## Portal path

```
Game (guest) ── XamInputNonControllerGetRaw/SetRaw (xam.xex ordinals 1185/1186)
      │
rexglue-sdk fork          forwards both exports to one handler the app registers
      ▼
skylanders-portal         SoftwarePortal (emulated) or UsbPortal (real portal via hidapi)
```

In Giants no game address is hooked for the portal; the game reaches it only through those two
XAM exports. **Whether Swap Force does the same is unverified** and is one of the first Phase 1
checks. Details in [portal.md](portal.md).

## Settings

Runtime settings are ReXGlue cvars, loaded from `swapforcerecomp.toml` next to the executable,
overridable with `--flag` or `REX_*` environment variables. `swapforcerecomp.toml.example` lists
them with descriptions:

- Portal: `portal_mode` (`software`, `usb`, `none`), `portal_figures_dir`, `portal_figure`.
- Graphics: `resolution`, `resolution_scale`, `frame_rate_limit`, `anisotropic_override`,
  `present_use_actual_aspect`, `gpu_allow_invalid_fetch_constants`, `vulkan_device`.
- Other: `language`, `mnk_mode` (keyboard and mouse), `ui_scale`, `user_data_root`, `skip_intro`
  (logo movies, on by default), `skip_opening_movie` (the long new-game movie, off by default).
- **Not available yet:** `ultrawide_fov`, `ultrawide_hud`. They are defined in the disabled
  ultrawide hook template and come back when it is ported (Phase 4).

`gpu_allow_invalid_fetch_constants` is on by default because Giants needs it (scenes render blank
without it). Whether Swap Force needs it is unverified.
