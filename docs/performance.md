# Performance and smoothness

Goal: a steady, smooth 60 FPS with the best picture the PC allows, and headroom above that.
Measured numbers are from the hub, 2560x1440 window, RTV render path, `present_interval = 1`,
`frame_rate_limit = 240`, Ryzen 5 5600 + Radeon RX 6800 XT (#44, #50).

## Where the time goes

- **The GPU sets the limit from `resolution_scale` 2 up:** about 167 FPS at scale 1, 93 at scale 2,
  46 at scale 3. Roughly a quarter of the GPU time is the EDRAM emulation: on the RTV path every
  resolve copies the host render target into the EDRAM buffer, resolves that into guest memory
  (or the scaled resolve buffer), and the texture cache turns it back into a texture.
- **The native 1280x720 scene** (`scene_size`, #41) costs about a fifth: 156.5 instead of about
  167 FPS at scale 1, 73.3 instead of about 93 at scale 2.
- **The CPU matters only at scale 1.** A build with `-march=x86-64-v3` (AVX2, FMA, MOVBE: the
  generated code's `std::fma` becomes `vfmadd` at about 8,900 places and the byte-swapped guest
  loads `movbe` at about 540,000) against the default `x86-64-v2`, same code, alternating runs
  (2026-10-20, two runs of two 10 s measurements each):

  | Build | Scale 1 (FPS) | Scale 2 (FPS) |
  |---|---|---|
  | x86-64-v2 (default) | 146.2, 158.0, 154.8, 148.4: **151.9** | 90.9, 90.8, 90.4, 88.5: **90.2** |
  | x86-64-v3 | 168.0, 154.2, 165.7, 166.1: **163.5** | 90.6, 90.5, 90.0, 89.9: **90.3** |

  About 8 % more where the CPU limits, nothing where the GPU does. v3 needs a CPU with AVX2
  (Intel from 2013's Haswell, AMD from Zen / Excavator); the default stays v2 until the owner
  decides (one exe for everyone, or a second v3 exe).
- **The guest vblank** comes from the SDK's timer thread on a fixed schedule (`graphics_system.cpp`,
  `last_frame_time += interval`), so it doesn't drift; it sleeps 1 ms at a time, and SDL3 sets the
  Windows timer resolution to 1 ms at start-up (`SDL_timer.c`), so the steps are about 1 ms.
- **First-time hitches** come from creating shaders and pipelines; the SDK has
  `async_shader_compilation` (on) and keeps them in its shader storage, so each happens once per
  install.

## What emulators and other recompilations do

Research of 2026-10-20 (source code and official docs; "unverified" where only a README was read).

- **Xenia Canary** ([GPU source](https://github.com/xenia-canary/xenia-canary/tree/canary_experimental/src/xenia/gpu)):
  RTV is the recommended path on AMD (its help text calls ROV "much slower now" and crash-prone
  on AMD drivers). `readback_resolve` none/fast/full, `readback_memexport`, fast occlusion
  queries, `d3d12_queue_priority`: our SDK has these with the stall-free defaults. Two ideas we
  don't have: `draw_resolution_scale_threshold` (small render targets such as blur and bloom
  chains stay at 1x) and non-square scaling (`draw_resolution_scale_x/y`, 2x1 plus FXAA/FSR at
  about half the cost of 2x2). The code has a TODO for direct host render target to guest memory
  resolves when no format conversion is needed: that skips the EDRAM dump pass.
- **UnleashedRecomp** ([video.cpp](https://github.com/hedge-dev/UnleashedRecomp/blob/main/UnleashedRecomp/gpu/video.cpp)):
  doesn't emulate the Xenos GPU at all. It hooks the game's statically linked Direct3D functions
  (CreateTexture, SetRenderTarget, StretchRect, Draw*, Present) and draws with its own renderer
  ("plume"): render targets are host textures, a resolve is a copy, shaders are translated ahead
  of time. Pipelines are compiled while the game loads its assets, plus a shipped list of
  pipeline states collected by testers. Frame pacing: flip model with a waitable swap chain and a
  limiter on a fixed schedule. MarathonRecomp works the same way; Xerenge (Burnout Revenge) is a
  ReXGlue port that replaces the command processor with such a renderer (unverified, README only).
- **Cemu** ([LatteTiming.cpp](https://github.com/cemu-project/Cemu/blob/main/src/Cafe/HW/Latte/Core/LatteTiming.cpp)):
  "match emulated display" drives the guest vsync from the monitor's real vblank
  (`D3DKMTWaitForVerticalBlankEvent`, skipping vblanks that come sooner than the guest period, so
  120 Hz gives 60). Async shader compiles skip the draw until ready.
- **RPCS3** ([tooltips.h](https://github.com/RPCS3/rpcs3/blob/master/rpcs3/rpcs3qt/tooltips.h)):
  "minimum scalable dimension" (small framebuffers not upscaled, like Xenia's threshold), an
  interpreter shader while the real one compiles; warns that its PS3-native frame limit can hurt
  frame pacing.
- **Dolphin** ([ubershaders](https://dolphin-emu.org/blog/2017/07/30/ubershaders/)): ubershaders
  while specialised shaders compile; "store EFB copies to texture only" keeps copies on the GPU,
  the same idea as resolves that never leave the GPU.
- **Windows:** flip model with a waitable swap chain and a maximum frame latency of 1 or 2
  ([Microsoft](https://learn.microsoft.com/en-us/windows/uwp/gaming/reduce-latency-with-dxgi-1-3-swap-chains));
  VRR needs tearing allowed with sync interval 0; 60 FPS on a 144 Hz screen without VRR can't be
  even. [PresentMon](https://github.com/GameTechDev/PresentMon) measures frame-to-frame display
  times and whether the game runs in independent flip.

## Next steps, most promising first

1. **Measure the frame pacing** with PresentMon at 60 FPS (present mode, frame-to-frame times);
   if it is uneven, drive the guest vblank from the monitor's vblank like Cemu, or present with a
   sync interval that matches the refresh rate.
2. **Resolution scale threshold** (Xenia's `draw_resolution_scale_threshold`, RPCS3's minimum
   scalable dimension): blur and bloom targets at 1x; more FPS at scale 2 and 3. SDK work.
3. **Direct resolves** for the cases without format conversion (#44, the Xenia TODO): skips the
   EDRAM dump; estimated about 10 % before, not measured. SDK work.
4. **Pre-built pipelines:** create the pipelines the shader storage already knows in the
   background at start-up. A pipeline list built from the game's shaders is derived from game
   data, so it can't be shipped in the repository; it has to come from the owner's own runs.
5. **x86-64-v3** as a second exe or the default (above): +8 % where the CPU limits. Decided
   2026-10-22 (the owner left it to me): not the default, since at resolution scale 2, the
   owner's setting, the GPU limits and v3 gains nothing; it would need a CPU from 2013 on and a
   full rebuild of all six games. Worth it again only if the CPU becomes the limit (scale 1 on a
   fast GPU, or the native renderer).
6. **Long term:** resolves that stay on the GPU by hooking the game's own Direct3D resolve and
   render target calls, as UnleashedRecomp does for its whole renderer. Large and game-specific;
   it would take most of the EDRAM cost away.

Not planned: a full renderer of our own, ubershaders, a Xenos shader interpreter.
