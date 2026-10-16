# Status

Updated at the end of every session. Only record what a build actually did.

## 2026-10-16: Phase 0 complete, Phase 1 boots to the title screen

- **Works:** `just configure-release` without a ROM succeeds on Windows 11 (VS 2022 17.14,
  clang-cl 19.1.5, CMake 4.4.4, Ninja), about 2 minutes. It builds the ReXGlue SDK from the
  submodule source tree and prints the codegen steps, because `generated/default/sources.cmake`
  does not exist yet.
- **Phase 1 started:** disc extracted into `rom/` (2,474 files) and the `default.xex` hash recorded:
  version `0.0.0.26`, PAL, title `415608E9`. The XEX check now accepts this dump.
- **Codegen works** (release preset, about 3 minutes): 445 files, 232 MB of C++ in
  `generated/default/`. It needed two things:
  - `amd_fidelityfx_dx12.dll` on `PATH`. `rexruntime.dll` imports it, but it is not copied next to
    the codegen tool, so `rexglue.exe` exited with 0xC0000135 (DLL not found). The `justfile` now
    adds FidelityFX's `PrebuiltSignedDLL` folder to `PATH`.
  - One `[functions]` entry: `0x82F60BD8`, reached only by a tail call from `0x82515C04`. That was the
    only analysis error in the whole XEX.
- **Codegen warnings:** 20 × "Unexpected float16_4 pack instruction", all between `0x82F87E84` and
  `0x82F88A10` (one small region, likely vector/half-float packing in a library). Watch for wrong
  vertex data or graphics glitches that trace back there.
- **Build works:** `just regen-release` (codegen up to date, configure, build) finished with 0
  errors in about 3 minutes; `swapforcerecompiled.exe` is 74 MB. `amd_fidelityfx_dx12.dll`,
  `rexruntime.dll` and `rexgpu-xenos.dll` are copied next to it. 178 compiler warnings, none fatal.
  The long-path warning from the configure did not break the release build.
- **First run (release, 45 s, no controller input): reaches the title screen ("PRESS START")** on an
  AMD RX 6800 XT via Direct3D 12. XEX check passes, portal mode `software`, audio endpoint opens.
  Log observations, none fatal:
  - `E:\` is not mapped (`ResolvePath(E:\) failed`); the game continues.
  - `D:\archives\gsbp_atvi.bk2` not found. The disc has `archives/GSBP_ATVI.bik` (Bink 1), so the
    game probably tries `.bk2` first. Whether the logo video played was not observed.
  - `gamecontrollerdb.txt` missing (SDL warning).
- **Not tried yet:** controller input, getting past the title screen, menus, levels. The disc has
  92 `.bk2` (Bink 2) videos, mostly level intros/outros, so the Bink 2 risk shows up in Phase 2.
- **Warnings from the configure** (7, none fatal):
  - FidelityFX (pulled in by the SDK): an object file directory under `out/build/` is 194
    characters long; CMake warns the build "may not work correctly" (limit 250). The repository sits
    in a deep folder. If the first build fails on long paths, use a shorter build or checkout path.
    Tracked as an issue in Phase 1.
  - The rest are upstream CMake policy/version notices in the SDK and FidelityFX.
- **Placeholders:** the hooks in `src/hooks/` are disabled Giants templates; `ultrawide_fov`,
  `ultrawide_hud` and `skip_intro` are commented out in the settings examples.
- **Known bugs:** none recorded.
- **Manual run with a controller (release):** title screen → save slot → new save → the intro movie
  plays (cannot be skipped) → "Place your Skylander on the Portal of Power". Controller input works.
  No audio at first: the runtime used the Windows default output, a monitor without speakers.
- **Portal path confirmed:** with `portal_mode = software` the game asks to place a Skylander instead
  of "can't find the Portal of Power", so the emulated portal answers through the XAM handler, as in
  Giants. No game hook is needed for the portal.
- **First gameplay (release, software portal):** a figure created with the F6 overlay is accepted, the
  tutorial level loads with textures, the character can be moved, and progress is written back to
  the figure file ("Portal: saved changes back to slot 0's figure file"). Problems seen:
  - **All characters and enemies render black** (player, Chompy, Creedle), and so do the character
    intro cards and the in-engine cutscene after the intro video (audio plays, picture black). The
    level geometry and textures are fine. Open, see below.
  - Low frame rate (not measured yet).
  - Repeated `XmaContext 6–9: There are no bits to copy!` errors; audio itself works.
- **Audio works.** "No audio" was the Windows output device (monitor without speakers).
- **Crash after the first tutorial fight: fixed (debug verified).** Not caused by attacking. The game
  dies when a Bink 2 video in format `KB2f` starts (`Tutorial_B_Intro.bk2`). 47 of the 92 `.bk2`
  files are `KB2f`, 44 are `KB2g` (those play), 1 is `KB2i`. Exit code `0xC000001D` (illegal
  instruction) from `__builtin_trap` in `sub_83094710`: an inline jump table in the Bink bit-stream
  decoder, index `(bits & 3) + 7`, codegen emitted 4 of 11 cases. A scan of `generated/` plus
  Ghidra found four under-counted tables (three in the Bink decoder, like GiantsRecomp; one 40-entry
  bounds-checked switch at `0x8247A658`), now in `config/default.toml`. Repro without input: copy a
  `KB2f` video to `rom/archives/gsbp_atvi.bk2`; the game plays it at boot. Before the fix it died
  after 24 s, after the fix it runs.
- **`float16_4` warnings are harmless.** All 20 are `vpkd3d128 …,5,3,0` (mask 3, shift 0). The SDK's
  own test notes and its D3DCOLOR handler treat mask 3 like mask 2 except with shift 3, so the
  generated code is correct. Not the cause of the black characters.
- **Tooling:** headless Ghidra works under Windows PowerShell 5.1 (`setup`, `import` done; project in
  `logs/ghidra_project/`). The debug build works after a `justfile` fix (debug codegen needs
  `amd_fidelityfx_dx12d.dll`, which the SDK writes to `bin/`).
- **Local workaround, not in the repo:** `rom/archives/FMV01.bk2` (the 5-minute new-game intro) is
  replaced with a short `KB2g` video to skip it; the original is kept outside the repo. A 540-byte
  `KB2f` placeholder crashed (before the jump table fix).
- **Black characters: fixed with `render_target_path_d3d12 = "rov"`** (manual test, release, AMD
  RX 6800 XT): characters, enemies, intro cards and the portal swap screen render correctly. With the
  default RTV path they were dark: base lighting missing, specular, effects and shadows still visible.
  `readback_resolve` (`fast`, `full`), `d3d12_readback_resolve` and `d3d12_readback_memexport` changed
  nothing. ROV is now set in `swapforcerecomp.toml.example` and the installer template. It costs
  performance (ROV emulates EDRAM in pixel shaders), and the first run recompiles all shaders.
- **Next:** confirm ROV is picked up from the settings file without the command-line flag; measure
  frame rate; the green right half of KB2f videos (#36); German language (`language = "german"`).
