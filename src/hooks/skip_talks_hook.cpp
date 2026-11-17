// Skip talks (F4 > Game): every cutscene can be skipped, also those the game only lets you skip on a replay
// (the owner's wish 2026-11-16, "die labern zu viel"; Giants and Trap Team have the same option).
//
// The game's scripts ask the native CutsceneSkipIsAllowed_Internal (sub_82916270, found 2026-11-17 by scanning the
// running game for its name's address 0x820EF090: its registered entry is the name followed by the native) whether a
// scene may be skipped. The native passes its argument to sub_82ABFB58, which asks the cutscene manager
// (sub_829B05A8, then sub_829BB800) and returns the answer in r3; the native hands that bool back to the script.
// sub_82ABFB58 has no other caller. With skip_talks on, its answer is "yes". Nothing is written: the scene and the
// save keep their own flags, so with the option off again the game decides as before.

#include <atomic>
#include <chrono>
#include <cstdint>

#include <rex/cvar.h>
#include <rex/hook.h>
#include <rex/logging.h>
#include <rex/ppc/context.h>

REXCVAR_DEFINE_BOOL(skip_talks, false, "Game",
                    "Every cutscene can be skipped, also those the game only lets you skip on a replay");

REX_EXTERN(__imp__sub_82ABFB58);  // whether a scene may be skipped (original)

REX_HOOK_RAW(sub_82ABFB58) {
  const uint32_t scene = ctx.r3.u32;
  __imp__sub_82ABFB58(ctx, base);
  const bool allowed = (ctx.r3.u32 & 0xFF) != 0;
  // TEMP DIAG (dev kit tests 2026-11-17): when the scripts ask, and what the game answers; at most every 2 s.
  {
    static std::atomic<int64_t> last_ms{0};
    const int64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(
                            std::chrono::steady_clock::now().time_since_epoch()).count();
    int64_t prev = last_ms.load();
    if (now - prev >= 2000 && last_ms.compare_exchange_strong(prev, now)) {
      REXLOG_INFO("TEMP DIAG skip check: scene {:08X}, game says {}", scene, allowed ? "yes" : "no");
    }
  }
  if (!REXCVAR_GET(skip_talks)) return;
  if (!allowed) {
    static std::atomic<bool> logged{false};
    if (!logged.exchange(true)) REXLOG_INFO("Skip talks: a scene the game alone wouldn't let you skip");
  }
  ctx.r3.u64 = 1;
}
