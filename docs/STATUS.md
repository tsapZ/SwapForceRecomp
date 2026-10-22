# Status

Updated at the end of every session. Only record what a build actually did.

## Current state (2026-10-21)

| Area | State |
|---|---|
| Boot, menus, saves | work; logo movies skipped (`skip_intro`), opening movie optional (`skip_opening_movie`) |
| Tutorial level | playable with controller or keyboard (`mnk_mode`); cutscenes and dialogue work |
| Graphics | correct with the fast `render_target_path_d3d12 = "rtv"` (default in templates again) since `execute_unclipped_draw_vs_on_cpu` is on (app default); `"rov"` still works but is much slower; 4K works. The 3D scene renders at 1040x620 like on the Xbox 360, HUD at 1280x720; experimental `scene_size = "1280x720"` draws it natively (D3D12, sharper, about a fifth of the frame rate, tried in the hub). The game's own FXAA now works at the internal resolution (`fxaa_follow_resolution_scale`) |
| Audio | works (check the Windows output device). The game mixes 5.1: the narrator on the center, menu music partly on the rear channels, in the hub mostly the front pair (measured 2026-10-20 with a temporary per-channel peak log of the guest's 6-channel render frames). With Windows' output device set to 5.1 the SDK passes the six channels through, otherwise it folds them to stereo |
| Language | English and German (`language`) |
| Portal | emulated, driven by the portal menu (opens by itself when the game asks for a Skylander; Guide, Back+Start, hold Back, F6; with the button it fades in during the game's zoom-out), real USB incl. old PS3/Wii portals (fork `tsapZ/skylanders-portal`), `none`. Back+Y / F5 take the item off, L3+R3 / F9 open the player menu, player 2 leaves through Start > "Leave the game" |
| Frame rate | 30 FPS default; experimental `present_interval = 1` -> 60 FPS, more with `frame_rate_limit` (0 keeps the game at 30). Tutorial, 1440p output, RTV: 237 FPS at scale 1, 106 at scale 2, 46 at scale 3 (ROV scale 2: 67). Game speed at 60 FPS verified by playing the tutorial (#37, closed); long falls at high FPS fixed (`fix_stuck_falling`) |
| Stutter | short one-time hitches when an effect or attack appears for the first time (shader/pipeline creation); stored in the persistent shader cache, so expected once per install |
| Videos | Bink 1 and Bink 2 movies play; the green logo (#36) was stale vertex data in the SDK, fixed |

Open next: first level full playthrough (#17), native 1280x720 3D scene in the levels (#41,
works in the hub as an experimental option), ultrawide HUD (#24, the 3D view works), installer (#27), German name cards (#38), controller portal overlay (#40).
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
- **Portal menu (#40), first version: works.** Scripted test at the Wolkenbruch checkpoint: the
  Guide button opens the menu for player 1 and lifts Chop Chop (the game shows its portal screen),
  choosing a card places that figure and the game continues with it, B puts the lifted figure back,
  and a swapper from the Swap Force tab asks whole / top / bottom and places both halves (Blast Zone
  playable). Cards are coloured by element, owned figures first, tabs per game plus "Recent",
  German texts. Not tested yet: two controllers (each player changes only their own figure),
  holding View, the real Guide button with the Windows Game Bar. Code in the
  `feature/portal-menu` branch of the skylanders-portal fork; the old overlay is on F8 now.

## 2026-10-18: session log (portal menu)

- **Portal menu, verified with scripted runs and by the owner:** Items tab (one shared item spot,
  any player takes it off with Y), swapper builder (combination named like the game, "Hoot
  Charge"), "Taken · P1" for characters another player has on the portal, `keyboard_player`,
  later figures by character id (Eon's Elite and Series 3/4 yes, SuperChargers no), redrawn
  element symbols, rounded cards. German "Varianten".
- **Open buttons no longer leak into the game:** Back is held back until it's clear it doesn't
  open the menu (`back+start`, `hold_back`); a short tap still reaches the game. Found with a
  Logitech Dual Action whose button 9 (Back) opened the game's Skylander menu on the way.
- **Menu opens by itself** when the game shows "Player N: place your Skylander": the app polls the
  game's `CMagicMoment` object (`src/game/magic_moment_watch.cpp`). Verified for player 1
  after loading a save and for player 2 one second after pressing A to join. The object sits in
  physical memory (0xBF6B816C in the test), which a first range check wrongly excluded.
- **Ghidra project:** about 15,400 functions were cut short because the register-save helpers at
  0x82FF2D00-0x82FF2D3C were marked as non-returning; a repaired copy was made outside the repo
  for this search. The project in `logs/ghidra_project` itself is still unrepaired.
- **Images from the game data are feasible (#46):** `rom/archives/toycollectionimages.pak` holds
  208 collection icons (every Skylander and variant, items, adventure packs); pak = Alchemy IGA
  v0x0A with LZMA/deflate blocks, textures in IGZ v7 as linear DXT5. A stdlib-only Python proof of
  concept decoded them correctly (outside the repo).
- Test script: `player N` makes the scripted controller act as another player.
- **Pictures in the portal menu (#46):** the collection icons and the German/English toy names now
  come from the owner's game files at runtime (`src/game/toy_images.cpp`, readers in
  `src/game/alchemy/`). The game's toy type enum (read from the loaded executable at 0x820FD058)
  has exactly the catalog ids. Indexed in about 150 ms (136 toys, 208 materials, 208 textures).
  Two traps found: object names in the toydata files are only unique per file (every swapper's
  toy is `ToyData`), and the icons are stored bottom row first (they come from .tga files).
- **Installer built:** `SwapForceRecompSetup-0.1.0-alpha.1.exe`; the owner installed it from the
  ISO, the game ran at 60 FPS and asked for a figure. Language choice and desktop shortcut added.

## 2026-10-19: session log (portal menu, player menu)

- **Every game tab lists its versions (#49):** a character shows up in each game that released a
  version of it, with the badge under the name (S2, LightCore, Sidekick). The Giants tab went from
  the few new Giants characters to full rows (Spyro S2, Drobot S2, Trigger Happy S2, Wrecking Ball
  S2 next to Bouncer, Sprocket, Pop Fizz). Verified in a scripted run with screenshots.
- **Options in the menu (Start):** which games are listed and "only my figures". They are written
  into the settings file line by line. Found on the way: the F4 overlay's save
  (`rex::cvar::SaveConfig`) rewrites the whole file from every changed setting, so it drops the
  comments and would also store command-line settings such as `--input_script` or
  `--user_data_root` (noted for #43).
- **Opening with the game:** the `CMagicMoment` states after lifting a figure, measured: 1 and 2
  within 0.1 s, 3 for about 0.8 s (the camera zooms out to the portal), then 5 (the prompt). The
  menu opened with Guide now stays hidden through the game's own figure-leaving animation and
  fades in 0.6 s into state 3: 0.92 s after the button, the game's text came at about 1.2 s. Same
  sequence for player 2 in co-op.
- **Player 2 leaving:** lifting player 2's figure does not just drop them out; the game shows its
  portal screen for player 2 with "B leave the game". The menu covered it. Start > "Leave the game"
  now closes the menu and presses that B for player 2; the game let player 2 go (scripted run).
- **Item off without the menu:** Back + Y (`portal_item_off_buttons`) or F5 takes the item off at
  once, with a short message; verified with the new chord support in input scripts (`tap back+y`).
- **Player menu (L3 + R3 / F9):** each controller moves itself between players 1-4 with left /
  right, Y back to connection order. Verified with the script device (moved to player 2 and back);
  with two real controllers and a Bluetooth reconnect still to test by the owner.
- **Sidekick pictures:** the 8 sidekicks had no icon, because `CPetToyData` names no collection
  material; their icons follow the character name (`collection_pet_terrabite_normal`, Giants ones
  with "lil"). All 8 show now; only UFO Hat has no icon. The toy index now builds right after
  start-up (it waits until the toy type table is readable), so the first menu already has it.
- **One run hit a GPU device removal** (`DEVICE_RESET`, out of memory for resolve heaps) with
  `--log_level=debug`, before the game showed its first frame; the same build ran normally without
  debug logging, twice. Not looked into further; watch for it.
- **Settings menu (#43):** F4 (and Back + RB) opens a new settings menu in the portal menu's look
  with six pages (graphics with presets, frame rate, game, portal, controls, advanced); the SDK's
  raw list moved to F10 and opens from the last page. Changes are written into the settings file
  one line at a time and keep the comment on that line. Verified in scripted runs (texture filter
  16x -> 8x written as `anisotropic_override = 4` with its comment; the settings file was restored
  afterwards). Input scripts can now press keys for the key binds (`key F4`).
- **Presets measured (#50):** hub, 1440p, uncapped: scale 1 + FSR 169 FPS (sharper than bilinear
  and not slower), scale 2 + CAS 83, scale 3 + CAS 40; texture filtering 16x vs 4x costs about 1 %.
- **The game's own FXAA, sharp at any scale:** a RenderDoc capture showed the game runs NVIDIA FXAA
  3.11 (console / 360 variant) over the scene every frame (event 9310: c0-c5 = RcpFrameOpt,
  360 RcpFrameOpt2, 8, 0.125, 0.05, ConstDir), with offsets for its own 1040x620 frame, so at
  resolution_scale 2 it sampled two host pixels apart and blurred. A new write watch
  (`src/dev/write_watch.cpp`, input script `watch` / `unwatch`: debug registers on every thread, a
  vectored exception handler, call chains named through PPCFuncMappings) found the writer of the
  FXAA attr, sub_82B8E340, which takes the frame size in f1/f2. A midasm hook there multiplies them
  by the draw resolution scale (`src/hooks/fxaa_scale_hook.cpp`). Compared at the same spot at
  scale 2: clearly sharper (railings, grass, characters) with smooth edges; 87.4 vs 86.8 FPS.
- **Portal compatibility page (#47):** `docs/portal-compatibility.md`; the owner's PS3 portal is a
  Giants portal (R ID 01 3D); Xbox 360 portals are not HID and need another backend.
- **RenderDoc scripts:** `consts` (draws whose pixel constants hold given floats, with an event
  range) and `cbuf` (a draw's constant blocks and inputs).
- **Native 1280x720 scene (#41): size found and changeable, picture black.** The 1040x620 is data:
  `renderer\outdoor_shading_xenon.igx` in `archives\loosefiles.pak` (the game parses the pipelines
  of all six platforms at start-up; PS4 / Xbox One use 1920x1080, PS3 / Wii U 1024x576). Only the
  `mainScene` render target has a size, all others follow it. A file overlay of that pak had no
  effect (the game never opens it through `d:\`), so the write watch traced the 1040: written by the
  IGX parser (sub_82455FD8 -> sscanf), read only by sub_8258EBE8, which computes every target's
  size. A midasm hook there (`scene_size`, experimental, off by default) sets 1280x720: every pass
  follows (64 instead of 86 FPS in the hub at scale 2), but the 3D scene is black; the four G-buffer
  surfaces need 2880 EDRAM tiles of 2048 (SDK: "Resolve region 0 <= x < 1280 is outside the surface
  pitch 880"). The default run with the hook built in is unchanged (85-86 FPS). Next: the engine's
  EDRAM surface allocation plus the SDK fork's 4096-tile EDRAM.
- **Write watch extended:** read watches, guest registers and stack at the hit, memory snapshots,
  `dump`; late threads get the breakpoint too.
- **Long falls at high frame rates: found and fixed (`fix_stuck_falling`, on).** The game's
  "stuck while falling" check (`sub_82A4A828`, CPlayerJumpComponent) counts a distance per frame
  (20 units, tuned for 30 FPS) and puts the player back after 3 s of "stuck". At 240 FPS every
  fall looked stuck; reproduced in the hub by launching the player high with the jump component's
  bounce pad fields: the 5000 launch ended with the player put back after 3.00 s. The hook scales
  the threshold by the game's frame time; with it no put-back at 240 FPS, at 30 FPS nothing
  changes. Found through the reflection field names (`_stuckFallingDistanceThreshold`,
  `_stuckFallingTime`, `_stuckFallingTimer`), their offset tables and the classes' RTTI.
- **Figure details (#48):** the details view (X) shows XP and the rest to the next level, play
  time, hero points, upgrades and path, the hat by its German or English name, and when and where
  the figure was last placed. Found on the way: the menu read only the first of a figure's two
  save areas, so it showed old values whenever the second was newer (a test figure: XP 147 shown,
  325 real), nothing at all for newly created figures (their first save goes to the second area),
  and at most level 10 (the Giants and Swap Force parts of the XP live elsewhere). Checked against
  the owner's dumps and with unit tests on synthetic figures; hat name checked with a test copy
  wearing hat 22 ("Cowboyhut").
- **Portal menu like the game (#40, #48):** the details view shows the upgrade tree with the
  game's own icons, names and descriptions (German or English), bought ones marked, the hat with
  its picture and stats, the level with an XP bar; the preview card shows level bar, hat and the
  bought upgrades; every element medallion and the Recent cards use the game's element symbols.
  All read at runtime from the owner's game files (`src/game/game_art.cpp`). Sidekicks now go on
  next to the Skylander instead of replacing it (Barkley appeared next to Doom Stone in the hub).
  Recent keeps 12 entries. Verified in scripted runs with screenshots (Chop Chop with a test hat,
  Doom Stone's halves).
- **GPU device lost three times today** (`DEVICE_RESET (bad GPU command)`, no driver timeout in
  the system log): once before the first frame with debug logging, once while the pipeline cache
  loaded (75 s instead of 30 ms, 380 pipelines failed), once 61 s in (creating a resolve tile
  heap failed first). Not reproducible on demand; every other run today was fine. New
  `gpu_crash_diagnostics` (off by default) turns on D3D12 DRED before the device is created, so
  the next one logs the last GPU commands and the faulting address; scripted test runs use it now.
- **Ultrawide field of view (#24): works for the 3D camera.** The renderer sets up the scene
  camera every frame (`sub_82B992A8`): `_aspect` (16:9) and `_fov`, then the perspective builder
  `sub_824906D8` (horizontal FOV 80 deg in radians, aspect, near, far). A midasm hook at that call
  (0x82B993A4) widens the aspect and the horizontal FOV for a window wider than 16:9
  (`ultrawide_fov`, on), keeping the vertical FOV; the app turns `present_letterbox` off so the
  16:9 guest frame fills the window. 2560x1080 window in the hub: without it the picture is
  stretched (the pond an ellipse), with it the proportions are right and more of the hub shows at
  both sides. The HUD is still stretched; 16:9 is unchanged.
- **Shadow map size (#44): tried, dropped.** The two shadow targets (`cascadeShadowsDepth`
  512x2048, `heroShadowsDepth` 256x512, both d24s8; igRenderTarget `_formatName` +0x14, from its
  reflection table at 0x822CEB40) can be resized in the existing size hook. At half size the hub
  ran 91-92 instead of 86-87 FPS at scale 2, but the characters' shadows disappeared; at double
  size the picture went black ("Resolve region is empty"). The game sets the shadow passes up
  for the original size elsewhere; not worth chasing for 6 %.
- **Details for sidekicks and items:** they keep nothing on the figure, so the details view was
  an empty two-column panel. Now a smaller one says what a sidekick or a magic item does and, for
  items, how long the effect lasts, read from each item's own data (`_itemDuration`: Anvil Rain
  45 s, the older adventure packs 5 s; 20 items have one). Checked with Barkley, Anvil Rain and
  Darklight Crypt.
- **Sidekick in co-op (open):** scripted run, player 2 brings Barkley: the menu places him (slot
  3), the game plays his sidekick intro and then shows his icon on player 1's side of the HUD,
  but within 50 s he was not to be seen in the hub. Alone, he appears next to the Skylander. Not
  looked into further; to check with two real controllers.
- **Owner's test (evening):** menu, details and sidekicks fine. Found: with one sidekick per
  player (Gill Runt for player 1, Barkley for player 2) the game showed "too many Mini Skylanders
  on the portal" and there was no way to take one off but leaving the game; F4 did nothing while
  the portal menu was open; the settings menu had no save step; the texture filter text spoke of
  test numbers. Fixed:
  - **One sidekick, shared** like the item (bringing one replaces the one there), and Recent
    starts with the item and the sidekick on the portal so A takes them off.
  - **Portal check:** the game's error screen is `CMagicMoment` state 16 (`eMMS_Error`, no
    player), its reason 1 = too many Skylanders, 2 = too many items or sidekicks
    (`EMagicMomentReason`). The check lists everything on the emulated portal, marks what there
    is too much of and takes off what the player marks. Reproduced with a second sidekick put on
    past the menu (`place` in input scripts): reason 2, listed, one taken off, the game went on.
  - **F4 / Back + RB over the portal or player menu** closes that one like B and opens the
    settings. **Settings are saved with Y**, applied at once to be seen, and closing with unsaved
    changes asks (save / don't save / back); checked: "don't save" put bloom back, Y wrote it,
    the real settings file untouched (`--config_file` for test runs).
  - Texts in the settings menu say what a setting does and what it costs, without test numbers.
  - **Motion blur off by default** (owner's choice).
  - **Hybrid portal** (`portal_mode = "hybrid"`, settings > Portal > "Both"): the portal menu and
    a real USB portal together (docs/hybrid-portal.md, 15 cases). Unit-tested with a second
    SoftwarePortal as the real one (copy, write-back, protected blocks, lifted by the menu,
    unplugged, backups kept); in the game it runs like `software` without a portal plugged in.
    Not tried with a real portal yet: none is connected here, and a figure on it would be written
    to.
  - The license notice named Skylanders: Giants (left from the template); now Swap Force.
- **Ultrawide HUD (#24): not built.** The UI (Juice) cameras are orthographic cameras that the
  size-change handler `sub_82576AA8` resizes, the same as the full-screen post-processing
  cameras, so a blanket hook there would break the post passes; the field offsets and the
  functions are noted in #24.
- **D3D12 vs Vulkan (#50):** the build now has Vulkan next to D3D12 (`REXGLUE_USE_VULKAN`, which
  the SDK leaves off on Windows) and `gpu_backend` picks it (settings menu, frame rate page).
  Hub, 1440p, scale 2, CAS, 240 limit, same binary: D3D12 86.6 / 85.9 FPS, Vulkan 87.6 / 87.0;
  Vulkan drew the hub correctly, the screenshot looked a little softer (not looked into). D3D12
  stays the default (render path choice, the measured presets); Vulkan is a fallback to try.
- **Jump height at high frame rates (measured):** from 60 FPS on, jumps are about 5 % lower
  than at 30 (full jump 131 vs 138 units at 240 FPS, short 127 vs 133); 60 and 240 FPS are
  nearly equal. Most likely the hold phase of the jump lasts up to a frame longer at 30 FPS. Not
  changed; `log_jumps` logs the heights.
- **Bloom and motion blur strength (#50): work.** `bloom_strength` scales the inner and outer bloom
  amounts the renderer copies from the level's `CBloomData` into `CBloomParametersAttr` each
  frame (`sub_82BA31A0`); `motion_blur_strength` scales the motion blur exposure time
  (`CMotionBlurData`, 0.35 in the hub) that `sub_82BA3390` stores into the motion blur attr, so 0
  is the state the game itself uses on camera cuts and the pass still applies its colour LUT. Both
  in the settings menu (graphics page). Verified in the hub: bloom 3 gives 6x as many very bright
  pixels as 1 (1902 vs 320 of 57600 samples), 0 a slightly clearer picture; during a camera pan
  motion blur 0 is sharp, 1 smears the grass as the game does, 2 more. Switching the passes off
  with `igRenderPass._active` was tried and dropped: it is only read when the pipeline is built,
  and the bloom and motion blur passes write targets that later passes read (the motion blur pass
  also applies the colour LUT).

## 2026-10-20: session log (native scene)

- **Native 1280x720 3D scene (#41): works (experimental, off by default).** The black picture had
  three causes, found in this order with RenderDoc and by logging every resolve's EDRAM base at
  both sizes:
  1. The per-pass fallback allocation (`sub_82514448`) puts the depth surface at the top of a
     2048-tile EDRAM (`subfic` at 0x82514608), which at 1280x720 overlaps the colours.
  2. Passes above 2048 tiles are split into predicated tiling strips (`sub_825138F8`, 880 + 400
     pixels), which the SDK cannot resolve ("outside the surface pitch 880").
  3. The frame planner (`sub_82593AD0`) gives every render target an EDRAM range for its lifetime
     from a heap of 2048 tiles. At 1280x720 that heap runs out, every target falls back to the
     per-pass allocation, and later passes overwrite what earlier ones left in the EDRAM: the
     G-buffer and the lit scene were right, the scene texture the water pass read was not.

  The SDK (pinned at b46d9d2) emulates a 4096-tile EDRAM (`edram_tile_count`, D3D12, resolution
  scale area up to 25). Three hooks use it: the planning heap gets 4096 tiles (0x82593B04),
  passes up to 4096 tiles stay untiled (0x82513A08), and a fallback depth surface moves to the top
  of the larger EDRAM (0x8251460C). They do nothing with the normal 2048-tile EDRAM. Setting
  `scene_size` turns on the larger EDRAM by itself unless `edram_tile_count` is set; without it
  (Vulkan, or a scale above 5) the hook warns once and keeps 1040x620 (verified with Vulkan:
  warning, normal picture, 90 FPS).

  Hub, 1440p window, 240 limit: scale 1 156.5 FPS (about 167 without), scale 2 73.3 FPS (about 93
  without). Screenshots are visibly sharper at both scales. In
  the settings menu as "Native 3D picture (experimental)" on the graphics page (restart). Not yet
  tried in the levels.
- **UFO Hat card:** the item has no picture of its own in the game data; it is a hat item
  (`_collectibleTracker` Hat_UFOHat), so its card now shows the game's hat icon (verified in the
  Items tab, Swap Force row).
- **The game's own words in the menus (#52): done.** The portal menu now says what the game
  says (its text files: `loosefiles.pak` `data\systemdata_*.mua`, `permanent_xenon.pak`
  `strings\xenonplatformstrings_*.igx`): German "MAGISCHES PORTAL", "Mini-Skylander" for
  sidekicks, "Magische Objekte" for magic items, "Verbesserungen" for upgrades, element names
  "Technologie" and "Gespenster" (the German game's word for Undead); English "Magic Items".
  Verified on a screenshot of the menu. The game has no words of its own for the menu's ideas
  ("Recent", "Place").
- **Languages on the disc:** this PAL disc has English and German only: 212 voice banks and the
  text files in each (`_en` / `_de`); only the Xbox licence agreement comes in nine more languages.
  Both are in the `language` setting already. More languages would need another regional disc's
  files (EU discs come in pairs: EN+DE, FR+NL, IT+ES), so not planned.
- **Controls page (settings menu): every menu button, on the game's controller picture.** Rows
  for the portal menu, taking the magic item off, the player menu and the settings menu, each with
  a few choices that don't clash (and "off"); the info panel shows the choice on the game's own
  Xbox 360 controller picture (`controller_schemexenon.pak`, the picture of its Controls screen)
  with a coloured ring per button entry, and an overview row shows all of them at once. The
  picture's callout lines are painted out when it is loaded (`CleanController` in
  `game_art.cpp`). Verified on screenshots. The game's own buttons: A jump, X attack 1, B or RB
  attack 2, Y or LB attack 3, LT / RT interact, Back Skylander stats, Start pause (its Controls
  screen, Pause > Options > Controls). The buttons count for every controller; buttons per player are
  not built (for a controller without an Xbox button there are choices without it).
- **x86-64-v3 build (measured, not switched):** about 8 % more FPS at scale 1 (163.5 vs 151.9),
  the same at scale 2 (GPU-bound); details and the research on what emulators do for smoothness
  in docs/performance.md.
- **Co-op sidekick (re-checked):** player 1 brings Barkley, player 2 joins and brings Mini Jini:
  the shared sidekick moves over (Barkley off, Mini Jini on another slot), the game shows Mini
  Jini and reports no "too many" error; both players keep playing (scripted, screenshot).
- **The GPU device losses are at least partly system-wide.** Windows logged GPU engine timeouts
  and driver resets (LiveKernelEvent 141, then 117 about 25 s later) on 2026-10-19 at 11:57,
  18:14, 18:24 and 21:45 and on 2026-10-20 at 09:42, 10:12 and 10:18. The three device losses of
  2026-10-19 (`DEVICE_RESET (bad GPU command)` after resource creation started failing) each come
  right after one of them; three of the others fell in gaps with no SwapForceRecomp or SSARecomp
  process running. So the game is not the only thing hanging the GPU: it loses its device when
  the driver resets, and then aborts. Running alongside: Parsec host (with its virtual display
  adapter) and Wallpaper Engine; AMD driver 32.0.21045.11001, no hardware errors (WHEA). For the
  owner to try: play without Parsec streaming and Wallpaper Engine, Adrenalin tuning at default.
  Open on our side: survive a device loss instead of aborting.
- **`stacks` input script command** (from Spyro's Adventure's first boot, src/dev/write_watch.cpp):
  logs every thread's call chain, recompiled frames by guest function.
- **SDK from our fork for every game:** the submodule now follows `tsapZ/rexglue-sdk` branch
  `skylanders`, the same commit as before plus the two fixes Spyro's Adventure needed (a ring
  buffer set up again starts empty; the last frame of an XMA stream is output, which only adds
  the missing last 512 samples of every sound here). Build and tests pass; a scripted run to the
  hub (portal menu, Doom Stone placed, 30 guest FPS) showed nothing new in the log.
- **One figure folder for every game** (the owner's call): `Documents\Skylanders\Figures`
  (portal fork `SharedFiguresDir`). The first start copies `Documents\swapforcerecomp\figures`
  into it and leaves a note in the old folder, which stays as a backup. Verified on the test copy
  (24 files, the recent lists with them, the menu's "Zuletzt" list intact).
- **Online co-op and profiles moved to the hub repo** ([Skylanders: Core of Light](https://github.com/tsapZ/SkylandersCoreOfLight),
  the owner's call): the proposal `docs/online-coop.md` and the issues #28, #29, #30 and #51
  (there #1-#4), because they apply to every game. Nothing of them was built here.

## 2026-10-20: session log (the owner's test round, evening)

- **A save came out empty after a crash.** The owner turned on the native 3D picture, restarted,
  and the game crashed while loading save slot 2; the slot then read "0 %" and its file
  `SWAPFORCE_2.xsg` was 0 bytes. Swap Force saves through `XamContentCreate` with CREATE_ALWAYS
  (create, write, close: 16 ms in a measured save), and the SDK deleted the old package at once,
  so the crash in that window left an empty one. Restored from the newest copy (a test copy of
  2026-10-19; the owner's hub sessions since then were mostly menu tests); both states are kept
  next to the repo in `dump/save-backup/`.
- **Saves can't be lost that way any more** (SDK fork `fix/save-commit-on-close`, on `skylanders`):
  as on the console, a saved game only changes when the game closes it. The old package and its
  header wait in `<user data>/save-backup/pending/` until then, and move on to
  `save-backup/previous/` (the version before the last save, kept for a manual restore). A start
  after a crash in between puts the old one back and logs it. Checked: a simulated interrupted save
  came back intact; normal saves log "saving ... anew" and "saved".
- **Crash with Guide, then B, with the real USB portal** (owner's report). With no emulated portal
  the portal menu only shows a message; B finished it in the middle of the frame and the footer
  then looked up a figure in the menu's empty library. Fixed in the portal fork (1566231); checked
  with `portal_mode = none` (the same path): Guide, B, Guide, A, no crash; the old code crashed on
  the first B.
- **Crash report in the log** (`src/dev/write_watch.cpp`, `InstallCrashReport`): a real crash now
  logs the faulting thread's call chain as guest functions and its guest registers ("Crash: ..."),
  through an unhandled-exception filter, so exceptions the SDK handles never show up.
- **The native 3D picture crashes now and then while a save loads** (#41, open). Reproduced with
  the owner's settings in about half of the runs; two places, found with the crash report:
  - `sub_82513358` (called per render pass from `sub_82514448`) at 0x82513630: for each render
    target it looks up a pooled surface record and, when that record's size matches the target's,
    reads the record's resource pointer at +60 and then +8 of it; at 1280x720 the pointer is
    sometimes 0 (read of guest 0x00000008). At 1040x620 that match never seemed to happen.
  - `sub_82476C40`, the game heap's free: the block's "previous block" pointer has lost its top
    byte (0x00DEBA60 for 0xB5DEBA60), so something wrote one byte past the end of a buffer before.
    Not found yet; the EDRAM planning heap the scene size enlarges is not it (it grows its lists
    dynamically).
  The setting's text now says it can crash while loading, and that the save stays safe. Skipping
  the logo movies doesn't help (tried); the Bink 2 movies (the opening movie) play right with it.
- **The Vicarious Visions logo is green on the right and bottom** when the logo movies are not
  skipped (owner's report; #36 reopened): it is the game's second Bink 1 movie (1040x620) after the
  Activision logo (1280x720) and shows in the top-left 1040x620 of a 1280x720 picture. Seen at 30,
  60 and 200 FPS, with RTV and ROV, at resolution scale 1 and 2, with and without the native scene,
  and with `execute_unclipped_draw_vs_on_cpu` off, so none of those cause it. The default
  (`skip_intro = true`) never shows it.
- **The native 3D picture loads reliably now** (#41): the crashes needed the scene to be exactly
  the screen's 1280x720. At 1264x704 neither crash site came up in 14 runs of the owner's settings
  (6 loading, 8 loading and walking in the hub), against 5 crashes in 10 runs at 1280x720. So
  `scene_size = "1280x720"` (the menu's "Nativ (720p)") draws 1264x704, 1.3 % stretched to the screen
  instead of 23 % from 1040x620 (`scene_size_hook.cpp`). It fits the Cemu graphic pack for the Wii U
  version, which renders that version's scene at 1280x720 and needs a shader fix for "the skipped
  upscale step": with a scene of the screen's size the game seems to skip its upscale pass and share
  the screen's targets, which the emulated EDRAM path doesn't survive. Research on how other ports
  get 4K: tsapZ/SkylandersCoreOfLight docs/other-ports.md.
- **The green Vicarious Visions logo is fixed** (#36, SDK fork `fix/vertex-buffer-cpu-writes`). A
  RenderDoc capture of each logo (new script `tools/renderdoc/vsout.py`: the vertex shader's outputs
  per vertex) showed both drawn from the same 2048x1024 / 1024x512 YUV textures with the same
  texture coordinates, 0.6248 x 0.7026: right for the 1280x720 Activision logo, too large for the
  1040x620 one (0.5076 x 0.6050), hence the green. A scan of guest memory during the second logo
  found its quad at 0xA556A044 with the right values, so the game had rewritten the vertex buffer in
  place and the GPU kept the old copy: the SDK skips RequestRange for a vertex fetch constant with
  the same address and size as before, and its per-frame reset only cleared the in-sync bits. Now the
  whole residency resets once per frame. Checked: the logo plays full screen; hub 90.9-92.5 FPS with
  the fix against 89.2-91.2 without (1440p, scale 2). Any game that rewrites a vertex buffer in
  place gains from it.

## 2026-10-21: session log (night)

- **Player menu with two players** (portal library, #56 in SwapForceRecomp): Player 1, Player 2
  and "Not playing" (guest users 2 and 3, e.g. a third controller) instead of four player columns;
  left / right moves a controller through the three. Checked by script.
- **Controls page for every device** (#55 / #29): the game's own Xbox 360 controller picture stays
  for Xbox pads; any other pad (PlayStation, Nintendo, a generic one like the Logitech Dual Action),
  and a game without a picture of its own, gets a drawn controller in its layout and with its
  button names. Used with the keyboard (F4, or a key pressed in the menu) the page shows a keyboard
  (QWERTZ in German): a button setting marks its key (F6, F5, F9, F4), the overview every menu key
  and the keys of the keyboard's controller emulation (`keybind_*`) with the button each one
  stands for ("Laufen" on WASD, "A" on Space). Checked by script on both views.
- **Achievements like on the Xbox 360** (#59): an unlocked one pops up in the bottom middle (the
  round icon opens into a dark bar, "Erfolg freigeschaltet", "25G - name", no sound), and the list
  opens with F7 or from F4 > Spiel > Erfolge with a controller (unlocked first with the date,
  totals; up / down, LB / RB, B). Both replace the SDK's own.
- **English checked** (title, portal menu, details, settings) with `--language=english`.
- **The game's gold symbol** (#57): the HUD's coin with the square hole
  (juicedomain_permanent.pak `textures\juicestandard_coin_icon_10.igz`) next to the gold and before
  every upgrade price.
- **Figure change measured** (#60, Grim Creeper -> Scarlet Ninjini in the hub, script): about 7.4 s
  from choosing to playing. The menu takes 0.35 s; the rest is the game's CMagicMoment sequence
  (state names from the class's enum at 0x820921C4: 7 IdleOutro 1.1 s, 8 Load 0.6 s, 10 PlayIntro
  4.1 s, 11 TransitionOut 0.3 s, 12 SpawnInLevel 0.8 s), so almost all of it is the figure's intro,
  as with a real portal. A pressed A doesn't cut it; writing TransitionOut into the instance as soon
  as PlayIntro starts skips it but leaves the camera on the sky (the intro's own ending sets the
  camera back). Not committed; next is ending PlayIntro the game's way.

## 2026-10-21: session log (morning)

- **Shorter figure intro** (#60, option `figure_intro_seconds`, F4 > Spiel > "Skylander-Vorstellung":
  "Wie im Spiel" or "Kurz", 1.5 s; default the game's). A write watch on CMagicMoment's state field
  for 11 (TransitionOut) named the writer: sub_829AA900, called from sub_829BFA10, called from
  sub_829BFFF8, which checks every frame whether the intro is over (time since it began in f0
  against its length in f30, fcmpu at 0x829C0114) and ends it the game's way. The hook
  (src/hooks/figure_intro_hook.cpp) shortens f30 there. Measured: the intro is 4.10 s long (logged
  for each figure); with "Kurz" Doom Stone's intro ends after 1.5 s and the game goes back to the
  hub with the camera and the HUD right (screenshots every second), so a figure change is about
  2.6 s quicker. Writing the state by hand, last night's try, had left the camera on the sky.
- **Input script `save`** (src/dev/input_script.cpp, from the later games): `save <hex addr>
  <hex bytes> <file>` writes guest memory to a file as it is (unmapped pages as zeros), for
  snapshots compared offline and for code dumps.
- **20 function starts the analyzer put one instruction late** (config/default.toml `[functions]`):
  a code dump with `save` searched for an instruction right after a `blr` that is no known start,
  followed by one that is, the shape that crashed SuperChargers and Imaginators at boot ("Call to
  invalid or unregistered function"). Swap Force had not hit one of them yet; registered so it
  can't. A boot into the hub and a level afterwards: no error.
- **The menus in the game's own fonts (#54)** (src/game/game_fonts.cpp, skylanders-portal
  b804d9c): fonts\proximanova-title.igz (permanent.pak, 88 glyphs, a 512x1080 page) and
  fonts\proximanova-body.igz (essentialui.pak, 167 glyphs) are igBitmapFont files, laid out as in
  SuperChargers / Imaginators where this was worked out (version 9 there, 7 here): each glyph's
  cell from its igCharMetrics, 16-bit tiled pages with the coverage in the first byte. The portal
  library draws the menus' titles and text with them (Segoe UI fills in missing glyphs), scaled so
  their capitals are as tall as Segoe's. The IGZ reader is the later games' version now (a
  superset: version 9, the object list, raw images). Checked by script in the hub: the portal
  menu's title in the same letters as the game's "... DAS MAGISCHE PORTAL" behind it.
- **The HUD for ultrawide, experimental** (`ultrawide_hud`, off; F4 > Grafik > "HUD bei
  Ultrawide", #24): the UI (Juice) lays itself out on its contexts' expected screen width
  (_expectedScreenWidthMain, +0x24, found from the reflection field names and three contexts in a
  hub snapshot). Set to the window's width at the UI's height, the HUD and the menus keep their
  proportions at the window's edges; of the three widths (viewer, setting, expected) only the
  expected one does it. Off by default: the pause menu's dimming is laid out 1280 wide and leaves a
  light strip at the right. Checked by script in a 2560x1080 window: the title, the hub's HUD at
  the left edge, the pause menu.
- **Movies on ultrawide** (2026-10-21): the logo movies and the opening movie are 16:9 Bink
  movies; while one plays the presenter shows it with black bars at the sides instead of
  stretched (present_use_actual_aspect off and present_letterbox on for that time, not saved).
  Start: BinkOpen's header hook; end: a hook at the start of BinkClose (sub_83084128, called only
  from the game's movie close). Checked by script in a 2560x1080 window with skip_intro off: the
  logos 16:9 in the middle, the screen after them fills the window again.

## 2026-10-22: session log (night)

- **Portal library** (cbc71c6, 0eb2564): the later games' cards say "Fahrzeug", "Sensei", "Falle" and "Serie 2" in the German menu; a plain hat symbol where a game has no hat picture.
- **Issues closed** that were done: #24 ultrawide (in all six games now), #48 the figure details (#40 and #41 stay open until the owner's play test, as the roadmap says).

## 2026-10-22: later the same night

- **Settings file read even with a repeated key** (SDK fork 96273bb): a key set twice in a table makes the whole TOML file invalid, and the game then ignored every setting in it without a visible error (the error went to a log that didn't exist yet). The SDK now reads such a file with the repeats left out and logs which key was repeated.
- **`--settings_schema=<file>`** (skylanders-portal 25dec20): the game writes its F4 menu's pages with their values and quits before a window opens; the Core of Light launcher shows them as the game's settings and writes into this game's settings file.
- **Figure pictures for the launcher** (skylanders-portal e87feb3): while the game runs, every figure picture it reads from the disc goes into `Documents\Skylanders\Pictures\<game>` once, for the launcher's figure library.
