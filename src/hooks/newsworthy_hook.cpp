// "Newsworthy" offline: Woodburrow's signpost without the Message of the Day download.
//
// Achievement 48, "Newsworthy" (15 G, "Use the signpost in Woodburrow to see the Message of the Day
// (online only)"), is the one of the 39 a single PC can't reach: the message came from Activision's
// servers (CNetworkTaskDownloadMessageOfTheDay, started only on an online event), which are gone,
// and this port is never signed in to Xbox LIVE anyway. The achievement itself is plain game logic
// with no online check, so the signpost is made usable here and the game awards it by itself:
//
//   - The signpost is entity "Mailbox" (model hub_signboard) in maps\town\town_stateN\*.mua
//     (town states 4-13). Its Scripts.InteractionSequenceComponentData has one interaction whose
//     EnableMessage is CMessageOfTheDayAvailableMessage; using it sends COpenMessageOfTheDayMessage
//     to the Mailbox, whose CMessengerComponentData then sends CAccoladeIncrementMessage for
//     AccoladeList.AchievementList_NewsWorthy_goals (accolades\accoladelist.igz: _achievementId
//     0x30, one goal, no requirements), and the accolade system queues achievement 48
//     (sub_829FA338 -> sub_82C57670 -> sub_82C573C8 -> XGIUserWriteAchievements).
//   - The only sender of CMessageOfTheDayAvailableMessage is sub_82A78838(X), at 0x82A788F4-
//     0x82A78920: with a downloaded message at X+8 that validated (X+36 == 1). X is the MOTD
//     manager (sub_82870AA8's singleton at 0x83132108, its +32), updated every frame by
//     sub_82A81E40(X) from sub_82A81FC0; that update looks only when X+33 is set (a fresh
//     download) and the player is in Woodburrow (sub_829EAC28(sub_82862D08()), the check at
//     0x82A81E64), then clears X+33. Offline X+8 stays 0 and X+33 is never set, so nothing is ever
//     sent and the signpost can't be used.
//   - Using the signpost also opens the MOTD screen: sub_82A27FC0 (CJuiceComponent's handler) reads
//     the downloaded message at X+8 and builds its screen from it (sub_82A715C0 reads +0x14 of it),
//     which would read through a null pointer offline.
//
// Here (motd_offline, on by default) sub_82A81E40 runs as recompiled, and then, when there is no
// downloaded message, about once a second: if the player is in Woodburrow (the game's own check),
// the same message is sent the same way as sub_82A78838 sends it. sub_82A27FC0 does nothing when
// there is no message (no screen to show; the accolade is the messenger's, not the screen's). With
// a real download (never, unless the servers came back) everything runs as recompiled. Found
// 2026-10-23 by following achievement 48 back from XGIUserWriteAchievements through the accolade
// data, and the message types (metadata 0x831280B0 / 0x83131F98) to their only sender; the calls
// are the game's own, on its own thread, in the order the game makes them.

#include <cstdint>

#include <rex/cvar.h>
#include <rex/hook.h>
#include <rex/logging.h>
#include <rex/memory/utils.h>
#include <rex/ppc/context.h>

REXCVAR_DEFINE_BOOL(motd_offline, true, "Game",
                    "Make Woodburrow's signpost usable without the Message of the Day download "
                    "(the servers are gone), so the achievement \"Newsworthy\" can be earned");

REX_EXTERN(__imp__sub_82A81E40);  // the MOTD manager's update (original)
REX_EXTERN(__imp__sub_82A27FC0);  // the MOTD screen (original)
REX_EXTERN(sub_82862D08);         // the current level
REX_EXTERN(sub_829EAC28);         // whether it is Woodburrow (the update's own gate)
REX_EXTERN(sub_823E0B48);         // the message pool of the object at 0x830E7B6C
REX_EXTERN(sub_8286FFC0);         // a new CMessageOfTheDayAvailableMessage from it
REX_EXTERN(sub_827AE120);         // the world's message target
REX_EXTERN(sub_82A210D8);         // send (target, message, 0)
REX_EXTERN(sub_82430498);         // release a reference

namespace {

uint32_t Load32(uint8_t* base, uint32_t address) { return rex::memory::load_and_swap<uint32_t>(base + address); }

// The MOTD manager X (sub_82870AA8's singleton at 0x83132108, its +32), or 0 before it exists.
uint32_t MotdManager(uint8_t* base) {
  const uint32_t owner = Load32(base, 0x83132108);
  return owner ? Load32(base, owner + 32) : 0;
}

// A frame of its own below the caller's, with the back chain, for calling the game's functions.
void OwnFrame(rex::CallFrame& frame, uint8_t* base) {
  const uint32_t caller = frame.ctx.r1.u32;
  frame.ctx.r1.u32 -= 128;
  rex::memory::store_and_swap<uint32_t>(base + frame.ctx.r1.u32, caller);
}

}  // namespace

REX_HOOK_RAW(sub_82A81E40) {
  const uint32_t x = ctx.r3.u32;
  __imp__sub_82A81E40(ctx, base);
  if (!REXCVAR_GET(motd_offline) || !x || Load32(base, x + 8) != 0) return;
  // About once a second at 60 FPS (the online game sends it once per download; the signpost only
  // needs it before it is used, and a level change ends any earlier one).
  static uint32_t frames = 0;
  if (++frames % 60 != 0) return;
  rex::CallFrame frame(ctx);
  OwnFrame(frame, base);
  PPCContext& c = frame.ctx;
  sub_82862D08(c, base);
  sub_829EAC28(c, base);
  if ((c.r3.u32 & 0xFF) == 0) return;  // not in Woodburrow
  static bool logged = false;
  if (!logged) {
    logged = true;
    REXLOG_INFO("Message of the Day: none downloaded (offline); Woodburrow's signpost enabled without one");
  }
  // 0x82A788F4-0x82A78920
  c.r3.u64 = Load32(base, 0x830E7B6C);
  sub_823E0B48(c, base);
  sub_8286FFC0(c, base);
  const uint64_t message = c.r3.u64;
  if (!message) return;
  sub_827AE120(c, base);
  c.r4.u64 = message;
  c.r5.u64 = 0;
  sub_82A210D8(c, base);
  c.r3.u64 = message;
  sub_82430498(c, base);
}

REX_HOOK_RAW(sub_82A27FC0) {
  if (REXCVAR_GET(motd_offline)) {
    const uint32_t x = MotdManager(base);
    if (x && Load32(base, x + 8) == 0) {
      REXLOG_INFO("Message of the Day: the signpost was used; no message to show (offline)");
      return;
    }
  }
  __imp__sub_82A27FC0(ctx, base);
}
