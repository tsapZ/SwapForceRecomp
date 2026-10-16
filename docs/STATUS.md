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
  **No audio is heard**; the runtime opened the endpoint "LG ULTRAGEAR (AMD High Definition Audio
  Device)", which may simply be a monitor without speakers. Four one-off XMA errors ("XmaContext N:
  There are no bits to copy!") appear at that point. Not yet clear whether this is a device issue or
  an audio bug.
- **Portal path confirmed:** with `portal_mode = software` the game asks to place a Skylander instead
  of "can't find the Portal of Power", so the emulated portal answers through the XAM handler, as in
  Giants. No game hook is needed for the portal.
- **Next (old):** press Start with a controller and see how far it gets; check whether the game calls
  XAM ordinals 1185/1186 (portal); set up the Ghidra tooling for the first real crash.
