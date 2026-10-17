# Roadmap

Where the project stands, what is done, what comes next. Details of every session are in
[STATUS.md](STATUS.md); each phase is a GitHub milestone. A step only counts as done when it was
observed in a real build or a real play test, not inferred.

## Overview (2026-10-17)

| Phase | State | Issues | What is left |
|---|---|---|---|
| 0 Repository and scaffold | ✅ done | 3/3 | – |
| 1 Recompile and boot | ✅ done | 8/8 | – |
| 2 In-game | 🟡 almost | 9/11 | play the first level to the end (#17), German name cards (#38) |
| 3 Portal | 🟡 almost | 6/7 | a swapper on the real USB portal (#42) |
| 4 4K and polish | 🟡 in progress | 5/10 | controller portal menu (#40), native 720p scene (#41), installer (#27), ultrawide (#24), wiki (#4) |
| 5 Online co-op (stretch) | ⬜ not started | 0/3 | architecture proposal first |

**Where we are:** the game is playable through the tutorial at 60 FPS and more, with the emulated
and the real portal. The remaining work is testing further into the game and polish.

## Now, next, later

| When | What | Who |
|---|---|---|
| **Now** | Play with the fast RTV path at 1440p and 60+ FPS, as far into the game as you like; report anything black, flickering, crashing or too fast (#17) | you |
| **Next** | Controller-only portal menu: Guide button opens a figure picker, no mouse needed (#40) | me |
| Then | Installer for a first release (#27) | me, test on a clean VM |
| If wanted | Native 1280x720 3D scene, only if the picture with CAS is still too soft (#41) | me |
| Later | Ultrawide (#24), online co-op proposal (Phase 5), GitHub wiki (#4) | me |
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

## Phase 2: in-game 🟡

- [x] Main menu, loading into the first level, saving and loading (#12, #13, #15)
- [x] Audio (#14, #32)
- [x] Graphics: black characters fixed, first with ROV, then properly for the fast RTV path (#16, #33, #39)
- [x] Crash on the first Bink KB2f movie fixed (#34); green video half only in a test sequence (#36)
- [x] Tutorial playable with controller and keyboard, German and English
- [ ] First level played to the end in one go (#17)
- [ ] German name card for enemy intros, if the original has one (#38)

**Done when:** the first level can be played through with a controller.

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
- [ ] Controller-only portal menu (#40)
- [ ] Native 1280x720 3D scene, the game renders it at 1040x620 (#41)
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
