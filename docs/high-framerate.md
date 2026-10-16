# High frame rates (60, 120, 144 FPS)

Research notes for unlocking the frame rate in Swap Force and later Skylanders recomps. Nothing here
is implemented yet.

## Where Swap Force stands

- Original: a steady **30 FPS (33.3 ms)** (F3 overlay, "Guest FPS").
- **The limiter is found:** the game paces itself through the D3D library's swap queue.
  `sub_82F5C258` queues a swap; bits 8-11 of its swap word (`r3`) are the presentation interval
  (vblanks per frame). It computes "earliest vblank = last swap + interval", and the vblank handler
  `sub_82F5C128` (called from the interrupt callback `0x82F58198`) only flips once the vblank counter
  reaches it. Swap Force passes swap word `0x00000200`: interval 2, i.e. 30 FPS at 60 Hz.
- **60 FPS works (title screen):** `src/hooks/frame_rate_hook.cpp` hooks `0x82F5C258` and rewrites the
  interval. `--present_interval=1` gives **60.1 FPS (16.63 ms)**. Off by default (`present_interval = 0`).
- **Game speed, first test:** in the tutorial the level runs at about 56-60 FPS with interval 1,
  and holding forward for 1.5 s moves the character the same distance as at 30 FPS. Player movement
  uses real elapsed time. Not yet checked: attacks, enemies, particles, physics objects, timers,
  cutscene sync. A call-rate check of the functions in the table below (tutorial, 30 vs 60 FPS):
  only `sub_82BCC438` runs every frame, and it converts an integer counter to seconds (`x * 1/60`)
  rather than stepping by a fixed 1/60; the six 1/30 functions are not called in that scene.
- **Above 60 FPS:** `--video_mode_refresh_rate=120` does not speed up the vblank pacing (30 FPS with
  interval 2), and together with interval 1 and `--vsync=false` it dropped to 31.8 FPS. Getting past
  60 needs work on the vblank/swap timing (the swap code also uses the refresh rate for its late-frame
  logic) or one of the approaches below.
- How it was found: the main thread waits on an engine semaphore (`sub_823F2358` -> the game's
  `WaitForSingleObject`), signalled after the swap. A dbghelp `StackWalk64` of all game threads in the
  debug build, `VdSwap`'s only caller (`sub_82F5D018`) and the vblank handler led to the swap queue.

## The problem: game speed tied to frame rate

Many games of that era advance their logic by a fixed amount per frame. Unlock the frame rate and
everything runs faster (2x at 60 FPS). Every approach below deals with this in a different way.

## Approach 1: game-side unlock with delta time (Xenia patch style)

The Xenia Canary community keeps about 134 "60 FPS" patches in
[xenia-canary/game-patches](https://github.com/xenia-canary/game-patches) (none for any Skylanders
game). A typical one, "60 FPS - Game Speed Fix" for Midnight Club Los Angeles, needs only two edits:
one branch that raises the vsync target to 60, and one byte that switches on the engine's existing
delta-time mode, "preventing game speed fluctuations based on frame rate". Above 60 FPS, Xenia also
needs host vsync off.

For a recomp this maps directly onto ReXGlue: a midasm hook or a small patch in
`config/default.toml` instead of a patch file, version-locked by the XEX hash like everything else.

How to find the pieces in Swap Force (Phase 4):

1. The frame limiter: search the main loop for the frame wait (timer reads via `mftb` or
   `KeQueryPerformanceCounter`, waits on the vblank event, the `VdSwap` call and its interval).
2. The timestep: float constants like `1/30 = 0.033333` (`0x3D088889`) or `30.0` (`0x41F00000`)
   near the game update. If the engine already computes a real delta time (many do), set it free
   instead of patching every system.
3. Test for speed: animations, physics, timers and cutscene sync at 60 FPS against 30 FPS.

First survey of Swap Force (2026-10-16), from the running game's image and the generated code:

| Constant | Addresses in the XEX data | Functions that load it (`lis` + `lfs`) |
|---|---|---|
| 1/30 (`0x3D088889`) | `82143D10`, `82192468`, `822940A8`, `8312640C` | `sub_8263B1F8`, `sub_8263B3B0`, `sub_82B34610`, `sub_82BA0958`, `sub_82D1F508`, `sub_82F6A600` |
| 1/60 (`0x3C888889`) | `82105018`, `8214E3B0`, `82294100` | `sub_82B9E308`, `sub_82BCC438`, `sub_82D14328`, `sub_82E59A30`, `sub_83014190`, `sub_83014AC8` |

`sub_8263B1F8` and `sub_8263B3B0` (adjacent, both 1/30) do float integration (`fsubs`, `fmuls`,
`fdivs`) without reading a timer: likely fixed-timestep physics or animation, i.e. places that would
speed up at 60 FPS. None of them is confirmed as the frame limiter yet. 30.0 and 60.0 also appear
(10 and 8 times) and were not traced.

## Approach 2: unlock and fix glitches one by one (Unleashed Recompiled)

[Unleashed Recompiled](https://cdn.jsdelivr.net/gh/hedge-dev/unleashedrecomp@main/README.md)
(Sonic Unleashed, Xbox 360, XenonRecomp) defaults to 60 FPS with higher targets and an unlocked
mode in its options. Its README says many high-frame-rate glitches were fixed as part of the
recompilation, and that some issues can still appear above 60 FPS. In practice: unlock first, then
find and patch each system that misbehaves. Most work, best result. Reviews report it "works like a
charm" at 60 FPS ([DSOGaming](https://www.dsogaming.com/news/download-and-play-an-unofficial-pc-port-of-sonic-unleashed/)).

## Approach 3: interpolate in the renderer (Zelda64Recomp / RT64)

[Zelda64Recomp](https://github.com/JS-mylove/Zelda64Recomp) keeps the game logic at its original
rate. Its renderer [RT64](https://www.GitHub.com/rt64/rt64) replays the draw calls and interpolates
object and camera matrices between game frames, so the game "is unaware of the new frames" and its
logic is unchanged ([Snowboard Kids 2 write-up](https://blog.chrislewis.au/snowboard-kids-2-is-recompiled/)).
That gives arbitrary frame rates (120, 144, more) with no speed problems.

Catch for Xbox 360 games: N64 games hand fixed-function matrices to the renderer. Xbox 360 games use
their own shaders, and the matrices sit in shader constants without labels. Doing this for Swap
Force would mean identifying the world/view/projection constants per draw and interpolating them.
Possible in principle, but a large project.

## Approach 4: frame generation (no game changes)

- **FSR 3 frame generation:** ReXGlue already pulls in the AMD FidelityFX SDK, but builds it with
  `FFX_API_ENABLE_FRAMEGEN_PROVIDER OFF` (`thirdparty/rexglue-sdk/cmake/rexglue_fidelityfx.cmake`).
  Enabling it would generate in-between frames on the GPU for every ReXGlue game at once. Quality
  depends on motion vectors, which an emulated Xbox 360 renderer does not provide, so expect more
  artifacts than in native PC games.
- **Driver or tool frame generation** (AMD Fluid Motion Frames, Lossless Scaling): works today
  without changes, doubles the presented frames, but adds input latency and artifacts.

## Recommendation for the hub

| Goal | Best approach |
|---|---|
| Real 60 FPS with correct game speed | Approach 1 per game (find limiter + delta time), plus Approach 2 for leftover glitches |
| 120/144 FPS | Approach 1 or 2 if the engine handles delta time well; otherwise Approach 3 (big) |
| Quick win for all games | Approach 4, as an option in the settings, off by default |

The games come from two studios (Toys for Bob: Spyro's Adventure, Giants, Trap Team, Imaginators;
Vicarious Visions: Swap Force, SuperChargers). Only Swap Force's engine (Alchemy) is verified
here; if siblings share an engine, a limiter found in one game may point the way in the other.
