# Status

Updated at the end of every session. Only record what a build actually did.

## Current state (2026-10-17)

| Area | State |
|---|---|
| Boot, menus, saves | work; logo movies skipped (`skip_intro`), opening movie optional (`skip_opening_movie`) |
| Tutorial level | playable with controller or keyboard (`mnk_mode`); cutscenes and dialogue work |
| Graphics | correct with the fast `render_target_path_d3d12 = "rtv"` (default in templates again) since `execute_unclipped_draw_vs_on_cpu` is on (app default); `"rov"` still works but is much slower; 4K works. The 3D scene renders at 1024x624, HUD at 1280x720 |
| Audio | works (check the Windows output device) |
| Language | English and German (`language`) |
| Portal | emulated (F6), real USB incl. old PS3/Wii portals (fork `tsapZ/skylanders-portal`), `none` |
| Frame rate | 30 FPS default; experimental `present_interval = 1` -> 60 FPS, more with `frame_rate_limit` (0 keeps the game at 30). Tutorial, 1440p output, RTV: 237 FPS at scale 1, 106 at scale 2, 46 at scale 3 (ROV scale 2: 67). Game speed at 60 FPS verified by playing the tutorial (#37, closed) |
| Stutter | short one-time hitches when an effect or attack appears for the first time (shader/pipeline creation); stored in the persistent shader cache, so expected once per install |
| Videos | Bink 2 (KB2f/KB2g) play; green half only in a test-only Bink 1-after-KB2f sequence (#36, closed) |

Open next: first level full playthrough (#17), native 1280x720 3D scene for a sharp picture at
scale 2 (#41, leads recorded), ultrawide hook (#24), installer (#27), German name cards (#38),
controller portal overlay (#40).
Testing rule: copied `--user_data_root`; scripted runs use `--input_script` (docs/development.md).

## 2026-10-16: session log (chronological)

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
- **Intro skip ported** (midasm hook in BinkOpen at `0x83081EB8`, header at `r1+112`): the
  Activision and Vicarious Visions logos are cut to one frame (`skip_intro`, default on), the
  5-minute new-game movie `FMV01.bk2` too if `skip_opening_movie` is on (default off). Movies are
  recognized by header size and frame count. Verified in the log for all three; the title screen
  appears after about 7 s. The earlier local video swap is undone.
- **Black characters: fixed with `render_target_path_d3d12 = "rov"`** (manual test, release, AMD
  RX 6800 XT): characters, enemies, intro cards and the portal swap screen render correctly. With the
  default RTV path they were dark: base lighting missing, specular, effects and shadows still visible.
  `readback_resolve` (`fast`, `full`), `d3d12_readback_resolve` and `d3d12_readback_memexport` changed
  nothing. ROV is now set in `swapforcerecomp.toml.example` and the installer template. It costs
  performance (ROV emulates EDRAM in pixel shaders), and the first run recompiles all shaders.
- **Frame rate:** steady 30 FPS (33 ms) in the tutorial, the game's own cap (F3 debug overlay,
  "Guest FPS"). Occasional hitches in fights while new shaders compile; the cache was fresh after the
  switch to ROV.
- **German works** (`--language=german`, or `language = "german"` under `["Game"]`).
- **Real USB portal works with an old portal** (PS3 portal, USB `1430:0150`, ready reply
  `52 01 3D`), tested with Hex. Two fixes in `skylanders-portal`, now pointed at the fork
  `tsapZ/skylanders-portal`, branch `swapforce/old-portal-support`:
  - Swap Force rejects old portal IDs ("Wrong Portal version"); the USB backend reports `52 02 1B`
    for `52 01 xx`.
  - The portal streams status frames faster than Swap Force polls (~10 Hz). Replies reached the game
    seconds late (an `R` answered 6 s later), the game reset the portal every ~20 s and never read a
    figure. `Read()` now drains the HID queue.
  The game never sent `J` or `M` in the traces, so nothing is synthesized for those.
- **Green video half (#36) does not affect the game.** Screen timelines show KB2f and KB2g movies
  render correctly, alone and back to back; a normal boot (both logos, Bink 1) has no green either.
  Only a Bink 1 movie playing right after a KB2f movie shows a green right half, which happened only
  in my test setup (KB2f video in the boot-logo slot, followed by the VV logo). Earlier notes saying
  "KB2f videos are green" were this artifact. Closed; reopen if a real cutscene shows it.
- **Note:** after changing graphics settings, the first start rebuilds all pipelines from the shader
  cache (about 35 s of black screen with `resolution_scale=1`).
- **4K:** `--resolution=4k --resolution_scale=1` holds the 30 FPS cap, and with
  `--present_interval=1` runs at a steady 60.1 FPS in the tutorial (RX 6800 XT, ROV).
- **60 FPS unlocked (experimental, off by default):** the cap is the D3D presentation interval 2 in
  the swap queue (`sub_82F5C258`). `--present_interval=1` gives 60.1 FPS on the title screen and
  ~56-60 FPS in the tutorial; walking speed is unchanged (same distance in 1.5 s as at 30 FPS).
  In the tutorial no frame-rate-bound logic found so far (call-rate check of the 1/30 and 1/60
  users). Above 60: add `--frame_rate_limit=144` (raises the emulated vblank rate): 122.8 FPS on
  the title screen; in the tutorial ~58 FPS with `resolution_scale` 2 and 104.7 FPS with
  `resolution_scale` 1 (supersampling is the limit). Other levels, bosses and minigames
  not checked yet (#37). Details in `docs/high-framerate.md`.
- **Next:** play further into the game and collect real bugs; frame rate above 30, 4K and
  ultrawide (Phase 4).

## 2026-10-17: session log

- **60 FPS game speed (#37): verified by playing.** Manual tutorial run (release,
  `--present_interval=1`, software portal): attacks and animations, enemy behaviour, effects and
  particles, crates, coins and timers all look normal at 60 FPS. Also runs with
  `--frame_rate_limit=144 --resolution_scale=1`; picture quality at scale 1 (720p internal) is
  visibly lower. Issue closed; reopen if another level, boss, minigame or cutscene runs too fast or
  out of sync.
- **One-time stutter:** short hitches the first time a new attack or effect appears, none when it
  repeats. That is shader/pipeline creation. `store_shaders` is on: the session updated
  `cache/shaders/shareable/415608E9.xsh` and `415608E9.rov.d3d12.xpso` in the user data folder, so
  the same effect should not stutter again in later sessions (not confirmed yet).
- **Swappers (#20): work.** Manual test: Blast Zone Top (slot 0) plus Fire Kraken Bottom (slot 1)
  combine into one character; both figure files are written back. Closed.
- **Black characters with RTV: root cause found and fixed (#39).** The black silhouettes (portal
  intro cards, the in-engine cutscene before the first summon, parts of levels) were not a lighting
  problem. Diagnosis with RenderDoc captures of whole guest frames in both paths: G-buffer and
  lighting are correct in RTV too (Chop Chop is lit at the end of the lighting pass). Later the game
  resolves the lit scene (k_8_8_8_8, EDRAM base 0, 1024x624) into a texture for its DOF/composite
  pass; in RTV that texture was pure white, in ROV correct. Logged resolve parameters are identical
  in both paths, so the EDRAM contents differ: before the resolve, the render target cache
  transferred the contents of a k_32_FLOAT target at base 507 over the scene. That target had
  claimed the whole EDRAM (2048 tiles from 507, wrapping into tiles 0-507) because its full-screen
  draws have clipping disabled and the SDK's `execute_unclipped_draw_vs_on_cpu` is off by default,
  so the draw extent is the 8192 scissor. Xenia defaults it to on for exactly this reason. The app
  now turns it on unless the user set it (`swapforcerecomp_app.h`), and the templates use RTV again.
  Verified by screenshots: intro card, airship cutscene and level, and the Wolkenbruch checkpoint
  look the same as with ROV; no banding difference at scale 3. Cvar experiments that did not help
  (for the record): snorm16 full range, gamma as unorm16, native 2x MSAA, float24 depth rounding and
  conversion, direct host resolve, native stencil output, MRT range clamp, depth transfer test,
  occlusion queries, bindless.
- **Performance with RTV** (tutorial start, 2560x1440 output, `present_interval=1`,
  `frame_rate_limit=240`, RX 6800 XT): scale 1 237.3 FPS (at the limit), scale 2 105.6, scale 3
  46.1 (same with 3840x2160 output); ROV scale 2 67.2. `frame_rate_limit=0` keeps the game at 30 FPS
  even with interval 1 (the SDK paces the guest at 30 Hz then); the template comment said
  "unlimited" and is corrected.
- **Picture size:** the game renders its 3D scene at 1024x624 (13 EDRAM tiles wide, 39 rows) and the
  HUD/final image at 1280x720. With `resolution_scale` 2 the scene is 2048x1248, scaled up to a
  1440p window, which explains the slightly soft picture.
- **Scripted input (`--input_script`)** for automated runs: controller timeline, frame rate logging
  and RenderDoc captures of whole guest frames, independent of the window focus. A Windows firewall
  prompt (RenderDoc's target control port) had blocked focus-based keyboard automation. See
  docs/development.md.
- **Sharper picture at scale 2 (#23):** `present_effect = "cas"` visibly sharpens edges (1:1 crops at
  1440p) for less than 1 FPS (105.1 -> 104.4). `fsr` changes little here because the final image is
  already window-sized; scale 3 is barely sharper because its 4K image is scaled down again. Added as
  a commented option to the templates; default stays `bilinear`.
- **Native 720p scene (#41): leads only.** The engine (Alchemy) has `_expectedResolutionX/Y` and
  `_overrideResolutionX/Y` reflection fields on its app/scene manager, Xenon surface objects with
  `_actualWidth/_actualHeight/_edramBase/_edramTileCount`, and supports predicated tiling. A native
  1280x720 G-buffer needs 2880 EDRAM tiles; the rexglue-sdk fork has started a 4096-tile EDRAM
  (`edram_tile_count`, not in our pinned commit). Input scripts gained `scan`/`poke` for finding
  such objects at runtime. Where the 1040x620 is computed is not found yet.
