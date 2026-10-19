# Roadmap

Where the project stands, what is done, what comes next. Details of every session are in
[STATUS.md](STATUS.md); each phase is a GitHub milestone. A step only counts as done when it was
observed in a real build or a real play test, not inferred.

## Overview (2026-10-17)

| Phase | State | Issues | What is left |
|---|---|---|---|
| 0 Repository and scaffold | ✅ done | 3/3 | – |
| 1 Recompile and boot | ✅ done | 8/8 | – |
| 2 In-game | ✅ done | 11/11 | – |
| 3 Portal | 🟡 almost | 6/7 | a swapper on the real USB portal (#42) |
| 4 4K and polish | 🟡 in progress | 8/17 | controller portal menu (#40, #46, #47), performance and picture quality (#44, #41, #50), installer (#27), ultrawide (#24), wiki (#4) |
| 5 Online co-op (stretch) | ⬜ not started | 0/3 | architecture proposal first |

**Where we are:** the first level is played through at over 100 FPS (1440p), the hub runs at about
80 FPS, with the emulated and the real portal. Gameplay works; the focus now is performance, picture
quality, then the controller-only Skylanders-style menus and a release.

## Now, next, later

| When | What | Who |
|---|---|---|
| **Now** | Test the new menu parts: Guide while running (menu after the zoom-out), Start > options and "Leave the game" as player 2, Back+Y for the item, L3+R3 player menu with two controllers and a Bluetooth reconnect (#40) | you |
| **Now** | Try the new settings menu (F4 or Back + RB): presets, frame rate page, changes marked "next start" (#43); new on the graphics page: glow (bloom) and motion blur strength (#50) | you |
| **Now** | At 200 FPS, jump off a long drop: the game used to put the player back after 3 s of falling, now fixed (`fix_stuck_falling`) | you |
| Next | USB portal device list in the settings menu (#47) | me |
| Research | Native 1280x720 3D scene (#41): size found and switchable (`scene_size`, experimental), but the picture is black because the EDRAM is too small; needs the engine's EDRAM layout plus a bigger EDRAM in the SDK | me |
| Research | Performance (#44): GPU-bound, ~25 % EDRAM emulation; the real fix is a direct resolve (new SDK shaders), about 10 % | me |
| Research | Best default settings (#50): D3D12 vs. Vulkan, what RPCS3 / Cemu / Dolphin do for a clean Skylanders picture, measured presets | me |
| Then | Installer for a first release (#27) | me, test on a clean VM |
| Any time | Keep playing; report anything black, flickering, crashing or too fast | you |
| Later | Ultrawide (#24), online co-op proposal (Phase 5), GitHub wiki (#4), profiles with name, picture and own saves / figures (#51) | me |
| After Swap Force | Own recompile of the next Skylanders game (Giants), then the launcher hub | both |

## Phase 0: repository and scaffold ✅

Set up the repository from the GiantsRecomp template, without any game data.

- [x] Reference clone of GiantsRecomp read; scaffold copied and renamed, submodules pinned (#1)
- [x] License, credits, project docs (#2)
- [x] `just configure-release` works on a clean clone without a ROM (#3)

**Done when:** configure succeeds without a ROM and its limits are documented. Observed 2026-10-16.

## Phase 1: recompile and boot ✅

- [x] Disc extracted, `default.xex` hash recorded: EU/PAL, v0.0.0.26, title `415608E9` (#5)
- [x] Headless Ghidra tooling (#6)
- [x] The game uses the XAM portal ordinals 1185/1186, like Giants (#7)
- [x] Codegen and build; one function and four jump tables needed config entries (#8, #9)
- [x] Engine compared with Giants: different studio, deferred renderer, own Bink 2 path (#10)
- [x] Bink 2 videos play (#11), long build paths handled (#31)

**Done when:** the logos or the intro video play. Observed 2026-10-16.

## Phase 2: in-game ✅

- [x] Main menu, loading into the first level, saving and loading (#12, #13, #15)
- [x] Audio (#14, #32)
- [x] Graphics: black characters fixed, first with ROV, then properly for the fast RTV path (#16, #33, #39)
- [x] Crash on the first Bink KB2f movie fixed (#34); green video half only in a test sequence (#36)
- [x] Tutorial playable with controller and keyboard, German and English
- [x] First level played to the end, over 100 FPS including fights (#17)
- [x] Name cards: English in the German original too, not a port bug (#38)

**Done when:** the first level can be played through with a controller. Observed 2026-10-17 (played
with keyboard; controller input verified in the tutorial).

## Phase 3: portal 🟡

- [x] `portal_mode` software / usb / none (#18)
- [x] Normal figure on the emulated portal, progress written back (#19)
- [x] Swappers on the emulated portal: both halves combine and save (#20)
- [x] Real USB portal, also the older Spyro's Adventure and Giants portals (#21, #35); fixes on
  the fork branch `swapforce/old-portal-support` (#22)
- [ ] A swapper on the real USB portal (#42)

**Done when:** a swapper can be placed, swapped and saved with both the emulated and the USB portal.

## Phase 4: 4K and polish 🟡

- [x] Intro skip, opening movie optional (#25)
- [x] Settings overlay F4 (#26)
- [x] 60 FPS and more (`present_interval`, `frame_rate_limit`), game speed verified (#37)
- [x] Fast RTV render path without black characters: 106 FPS at 1440p with `resolution_scale` 2,
  237 FPS at scale 1 (#39)
- [x] Resolution options measured; `present_effect = "cas"` recommended at scale 2 (#23)
- [ ] Performance: GPU-bound; about 25 % of GPU time is EDRAM emulation (transfers, dumps) (#44)
- [x] Smoother edges: the game's own FXAA now works at the internal resolution, sharp and smooth at scale 2+ (#45)
- [ ] Portal menu for controller and keyboard in Skylanders style, per player, replacing F6 (#40):
  works with items, swapper builder, "Taken" marks, keyboard as any player, opens by itself when
  the game asks for a Skylander, waits for the game's zoom-out after Guide, options (shown games,
  only my figures), "Leave the game" for player 2, Back+Y takes the item off
- [x] Portal menu: each character in every game that released a version, with series badges (#49)
- [x] Player menu (L3 + R3 / F9): which controller plays as which player
- [ ] Player assignment at game start, Switch style: each controller presses a button and gets a
  player number (the player menu does this by hand; automatic at start not started)
- [ ] Ideas for the menu (not started): the game's own font from the game files for more Skylanders
  feel; more information per variant (#48); in the launcher hub, pictures from each game's own
  files, so an owner with only Spyro's Adventure gets that game's pictures
- [x] Pictures and names from the owner's game data: collection icons for every figure and item,
  German item names (#46)
- [ ] USB portal selection and portal compatibility table (#47)
- [x] Settings menu in Skylanders style, usable with controller, replacing F4 (#43): six pages,
  presets, changes saved line by line; the SDK's list moved to F10
- [ ] Native 1280x720 3D scene, the game renders it at 1040x620 (#41)
- [ ] Best default settings, researched and measured: renderer, presets (#50); done so far:
  measured presets, the game's FXAA at the internal resolution, bloom and motion blur strength,
  long falls at high frame rates
- [ ] Installer tested on a clean Windows VM (#27)
- [ ] Ultrawide: projection hook for 21:9 (#24)
- [ ] Docs mirrored into the GitHub wiki (#4)

**Done when:** 4K and 21:9 render correctly, the portal can be used with a controller only, and the
installer works on a clean Windows VM.

## Phase 5 (stretch): online co-op with two real portals ⬜

- [ ] Architecture proposal in `docs/online-coop.md`, not code (#28)
- [ ] Remote play options researched (#29)
- [ ] Figure sync and write-back designed (#30)

**Done when:** the proposal is written and agreed; building it is a separate decision.

## Risks and how they turned out

| Risk | Outcome |
|---|---|
| Swap Force's engine differs a lot from Giants | Yes (different studio, deferred renderer), but the toolchain held up |
| Game does not use the XAM portal ordinals | It does; no game hook needed for the portal |
| Bink 2 videos stall | One crash from under-counted jump tables, fixed in the config |
| Swapper halves need extra protocol behaviour | Not needed on the emulated portal; USB still to test |
| Fast render path unusable | Fixed: an SDK default (`execute_unclipped_draw_vs_on_cpu`) |
