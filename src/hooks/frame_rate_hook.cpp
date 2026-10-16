// Experimental frame rate unlock: overrides the presentation interval of the game's D3D swaps.
//
// The game paces itself through the D3D library's swap queue, not by counting frames itself:
// sub_82F5C258 queues a swap and takes a swap word in r3 whose bits 8-11 are the presentation
// interval (vblanks per frame). It computes "earliest vblank = last swap + interval", and the
// vblank handler (sub_82F5C128, called from the interrupt callback at 0x82F58198) only flips to
// the frame once the vblank counter reaches that value. Swap Force uses interval 2, i.e. 30 FPS at
// 60 Hz. The midasm hook at 0x82F5C258 (config/default.toml) rewrites the interval before the
// function reads it. Found by stack-walking the main thread (waits on a semaphore the render side
// signals) and reading the vblank handler; see docs/high-framerate.md.
//
// Off by default. Game logic may assume 30 FPS (fixed 1/30 timesteps exist in several places), so
// parts of the game can run too fast with a lower interval.

#include <atomic>
#include <cstdint>

#include <rex/cvar.h>
#include <rex/logging.h>
#include <rex/ppc/context.h>

REXCVAR_DEFINE_INT32(present_interval, 0, "Graphics",
                     "Override the game's presentation interval (vblanks per frame): 0 = game "
                     "default (2, 30 FPS), 1 = 60 FPS. Experimental: game speed may change.");

void SwapIntervalHook(PPCRegister& r3) {
  static std::atomic<bool> logged{false};
  if (!logged.exchange(true)) {
    REXLOG_INFO("Swap: game presentation interval {} (swap word {:08X})", (r3.u32 >> 8) & 0xF, r3.u32);
  }
  const int32_t interval = REXCVAR_GET(present_interval);
  if (interval > 0 && interval <= 15) {
    r3.u32 = (r3.u32 & ~0xF00u) | (static_cast<uint32_t>(interval) << 8);
  }
}
