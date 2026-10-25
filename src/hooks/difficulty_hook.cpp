// Nightmare ("Albtraum") from the start, as an option (SwapForceRecomp #74).
//
// The game unlocks its hardest difficulty only after Story Mode was finished once. The difficulty
// screen (the class with _difficultyLevel, _launchNewGameOnSelect, _dialogConfirmed; its table at
// 0x820785C0) asks sub_82C12BC0 whether a level can be picked: levels below 3 always, level 3
// (eDL_Nightmare) only when sub_829CC788 says yes. sub_829CC788 reads the profile value
// "nightmare_difficulty_unlocked" (0x821140A0, through sub_823E96C0) and answers whether it is 1;
// sub_829CC750 writes the 1 when the game unlocks it (tryUnlockNightmareDifficultyLevel_Internal).
// Both are also in a table at 0x82396368, so other callers go through the same two functions.
//
// With nightmare_from_start on, sub_829CC788 answers yes without reading. Nothing is written: the
// profile keeps its own value, so with the setting off again Nightmare is locked until the game
// unlocks it the usual way. Found 2026-10-25 from the profile key's name to its getter and the
// difficulty screen's check.

#include <cstdint>

#include <rex/cvar.h>
#include <rex/hook.h>
#include <rex/logging.h>
#include <rex/ppc/context.h>

REXCVAR_DEFINE_BOOL(nightmare_from_start, false, "Game",
                    "Nightmare difficulty can be picked from the start, without finishing Story Mode "
                    "first (the save is not changed)");

REX_EXTERN(__imp__sub_829CC788);  // whether Nightmare is unlocked (original)

REX_HOOK_RAW(sub_829CC788) {
  if (REXCVAR_GET(nightmare_from_start)) {
    static bool logged = false;
    if (!logged) {
      logged = true;
      REXLOG_INFO("Nightmare difficulty open from the start (nightmare_from_start)");
    }
    ctx.r3.u64 = 1;
    return;
  }
  __imp__sub_829CC788(ctx, base);
}
