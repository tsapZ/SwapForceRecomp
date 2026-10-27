# A native renderer for Swap Force? (feasibility, 2026-10-20)

Today the game's CPU code is recompiled, but its graphics still go through the SDK's emulation of
the Xbox 360 GPU (Xenos): command buffers, shaders translated at run time, the 10 MB EDRAM and its
resolves. That is where the limits come from (the 1040x620 scene, resolution only by scaling the
render targets, about a quarter of the GPU time in EDRAM emulation, docs/performance.md). A native
renderer would replace that emulation with code that draws the game's frames with D3D12/Vulkan
directly. This page records what was measured to estimate the work, and what other ports did.
Research behind it: tsapZ/SkylandersCoreOfLight docs/other-ports.md and docs/ai-recomp.md.

## Measured in this game

| What | Value | How |
|---|---|---|
| Recompiled functions | 81,508 | generated code |
| Shaders used so far | 320 (403 pipelines) | the owner's shader storage after the tutorial, the first level and the hub |
| One hub frame | about 75 render passes, about 1,200 draws | RenderDoc capture, `thirdparty/skylanders-portal/tools/renderdoc/list.py` + `group.py` |
| Frame layout | deferred: shadow maps (one pass alone ~670 draws), G-buffer of 3 colour targets + depth (~290 draws), lighting, water, bloom / depth of field / motion blur, FXAA, HUD | same capture |
| Xbox 360 D3D library | about 800 functions at 0x82F50000-0x82F80000 (found from the callers of the kernel's `Vd*` imports, 0x82F59BC0-0x82F70D00), about 130 of them called from the game | `d3dsurface.py` (scratch) over the generated code; the C runtime (memcpy, memset, XMemCpy) sits right below it at 0x82F40000 |
| GPU packets written outside D3D | none found (the hits were status-code compares in the game's exception handler) | same |
| Predicated tiling | used: passes above 2048 EDRAM tiles are cut into strips (scene_size_hook.cpp) | earlier work on #41 |
| Memory export | not counted yet | |

For comparison, WWE SvR 2011's D3D block had 562 functions, and 37 named ones covered 80 % of the
D3D calls per frame; it used 467 shaders.

## The three ways other ports went

1. **Hook the game's D3D functions and implement them on a modern API** (Unleashed Recompiled,
   re:Blue, WWE SvR 2011, reNut, Xerenge, Dead Rising 2 after its switch). Shaders offline with
   XenosRecomp (MIT, needs patches per game: DR2 needed 23), drawing through plume (MIT). EDRAM is
   gone: a resolve is a copy, predicated tiling becomes one pass. Reusable as a pattern (the hook
   list, the guest device layouts, plume, the shader cache), not by address.
2. **A scene renderer that reads the game's own structures** (Skate 3 Recomp 2.0): the fastest
   result for one game, but tied to its engine and materials; nothing carries over.
3. **A game-specific GPU command executor** built on a count of what the game sends (Forza Horizon
   "Pinyon Shift"). A hybrid of native and emulated draw families was tried there and dropped: 91-95
   ms per frame against 23.5 ms emulated.

Reported effort (public history; work before it unknown): Unleashed about 5 months with 4-5 core
developers (who also built the tools); Skate 3's native renderer about 3 weeks by one person;
Dead Rising 2 switched to the D3D way on 2026-08-06 and "boots, renders and plays" five days later
(one person with Claude Code, whole port about 10 weeks); WWE's native renderer was written by Claude
Code. Skate 3 reports more than twice the frame rate at about a quarter of the power.

## Estimate for Swap Force (way 1)

| Step | What | Rough size |
|---|---|---|
| 1 | A counting build: a counting hook on each of the ~130 D3D entry points, plus render target configurations, resolve kinds, formats, memory export and tiling per scene (menu, hub, level, a figure swap), written to a `d3d_map.csv` | 1-2 sessions |
| 2 | Name the D3D functions and decode the guest device and resource layouts (the device may keep a mirror of the GPU registers, as in WWE) | 2-4 sessions |
| 3 | First native picture: the title screen and menus through plume, shaders through a patched XenosRecomp, the emulated renderer kept as the switchable fallback and reference | 3-6 sessions |
| 4 | The hub and a level: G-buffer, shadow maps, lighting, water, post effects, Bink movies, the HUD, the portal effects | several weeks |
| 5 | Whole-game parity, played through by the owner, with A/B against the emulated renderer | months, alongside |

Gains if it works: well over today's frame rate, any resolution without the EDRAM limit (the native
scene would simply be the screen size), MSAA, larger shadow maps, clean ultrawide. The way carries
over to Giants and the later games as a method (each needs its own hook search; the D3D names could
be moved between the executables with Ghidra Version Tracking or BSim if they link the same XDK
version, unchecked). Risks: visual parity across the whole game, the time, and licences (Unleashed
is GPLv3: read it, don't copy it; plume and XenosRecomp are MIT).

Decision: not made. The cheap first step (the counting build) also helps the performance work
(#44) and decides whether step 3 is worth starting.
