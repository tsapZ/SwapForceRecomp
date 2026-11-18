// Skip talks (F4 > Game): B or Y skips every cutscene, also those the game only lets you skip on a replay (the owner's
// wish 2026-11-16, "die labern zu viel", and 2026-11-17 "mach B einfach fest"; Giants, Trap Team, SuperChargers and
// Imaginators have the same option).
//
// Vicarious Visions' developer switch "auto skip cutscenes": the scripts' native AutoSkipCutscenes_Internal
// (sub_82916F58, found 2026-11-17 by scanning the running game for its name's address 0x820EEAF0: the registered entry
// is the name followed by the native) returns what sub_82BABDA8 says, and in this game that is "li r3,0; blr": the
// switch is built out. The game asks it every frame from three places of its own, and while a scene plays the scene's
// script asks it too, every frame, through the native (return address 0x82916F78; measured 2026-11-18 on ww1's
// SawmouthIntro scene, which runs about 6.5 s without input). With skip_talks on, after a new press of B or Y during
// such a run of the script's asks the switch says yes to the script (only to it) until the asks stop (the scene is
// over), at most 20 s. Also with the option on, the scripts' CutsceneSkipIsAllowed_Internal (sub_82916270 ->
// sub_82ABFB58, whose only caller it is; name 0x820EF090) is answered yes. Test scene: ww1's SawmouthIntro, started by
// a teleport into its trigger box from the map file (debug kit route sf_skip_scene_b.txt). Nothing is written: the
// scenes and the save keep their own flags. The presses come from the lib's own collector for this
// (TakeNewSkipPresses), so the movie skip keeps its presses.

#include <chrono>
#include <cstdint>
#include <mutex>

#include <rex/cvar.h>
#include <rex/hook.h>
#include <rex/logging.h>
#include <rex/ppc/context.h>

#include "portal_rex/portal_menu_input.h"

REXCVAR_DEFINE_BOOL(skip_talks, false, "Game",
                    "B or Y skips every cutscene, also those the game only lets you skip on a replay");

REX_EXTERN(__imp__sub_82ABFB58);  // whether a scene may be skipped (original)
REX_EXTERN(__imp__sub_82BABDA8);  // the auto skip switch (original: always no)

namespace {

constexpr uint32_t kFromScript = 0x82916F78;        // the return address in AutoSkipCutscenes_Internal
constexpr uint16_t kSkipButtons = 0x2000 | 0x8000;  // X_INPUT_GAMEPAD_B | X_INPUT_GAMEPAD_Y
using Clock = std::chrono::steady_clock;
std::mutex g_mutex;
Clock::time_point g_last_ask{};
Clock::time_point g_skip_until{};  // the scene's script hears yes until then (or until its asks stop)

}  // namespace

REX_HOOK_RAW(sub_82ABFB58) {
  __imp__sub_82ABFB58(ctx, base);
  if (REXCVAR_GET(skip_talks)) ctx.r3.u64 = 1;
}

REX_HOOK_RAW(sub_82BABDA8) {
  const bool from_script = uint32_t(ctx.lr) == kFromScript;
  __imp__sub_82BABDA8(ctx, base);
  if (!from_script || !REXCVAR_GET(skip_talks)) return;
  const Clock::time_point now = Clock::now();
  std::lock_guard lock(g_mutex);
  // A new run of asks is a new scene: presses from before it (gameplay, a load, a menu) are dropped, and a skip
  // from the scene before ends.
  const bool new_run = now - g_last_ask > std::chrono::milliseconds(250);
  g_last_ask = now;
  const uint16_t presses = skylanders::menu_input::TakeNewSkipPresses();
  if (new_run) {
    g_skip_until = {};
    return;
  }
  if (now >= g_skip_until && (presses & kSkipButtons) && !skylanders::menu_input::MenuOpen()) {
    g_skip_until = now + std::chrono::seconds(20);
    REXLOG_INFO("Skip talks: {} skips the scene", (presses & 0x2000) ? "B" : "Y");
  }
  if (now < g_skip_until) ctx.r3.u64 = 1;
}
