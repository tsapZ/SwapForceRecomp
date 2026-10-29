// Where the player is, for problem reports (skylanders-portal dev/where.h).
//
// The level: the level archive the game opened last (rom\archives: tutorial_a/b, Cloudbreak;
// town_state1-13, Woodburrow as the story goes on; verdant1, outlands01/02, winterkeep02,
// finale01, fg1-4, ww1-4, brn1-4; their sound banks, cutscenes and figure parts are other
// archives).
//
// The player: CPlayerJumpComponent's stuck-falling check, sub_82A4A828, runs once a frame for every
// player (stuck_falling_hook.cpp has a midasm hook at its entry). The component's +12 is a handle to
// its entity, resolved as sub_82A12CD8 does when the handle's first word has none of the bits
// 0x03000000 (its +20; otherwise it goes through a lock, and that frame is left out); the entity's
// +40, +44, +48 are its position (sub_82988E28). Found 2026-10-29; the first player checked of
// those seen in the last half second is player 1 (ReportPlayerFrame).

#include "hooks/where_hook.h"

#include <cstdint>
#include <regex>
#include <string>
#include <string_view>

#include <rex/hook.h>
#include <rex/memory/utils.h>
#include <rex/ppc/context.h>

#include "dev/where.h"

namespace {

uint32_t Load32(uint8_t* base, uint32_t address) { return rex::memory::load_and_swap<uint32_t>(base + address); }
float LoadFloat(uint8_t* base, uint32_t address) { return rex::memory::load_and_swap<float>(base + address); }
bool GuestPointer(uint32_t v) { return v >= 0x40000000 && v < 0xC0000000; }

// "game:\archives\town_state5.pak" -> "town_state5"; "" for any other file.
std::string LevelOfArchive(std::string_view path) {
  static const std::regex level(
      R"((?:^|[\\/])((?:tutorial_[ab]|town_state\d+|verdant\d+|outlands\d+|winterkeep\d+|finale\d+|fg\d|ww\d|brn\d))\.pak$)",
      std::regex::icase);
  std::cmatch m;
  if (!std::regex_search(path.data(), path.data() + path.size(), m, level)) return {};
  return m[1].str();
}

}  // namespace

namespace swapforcerecomp {

void InstallWhere() {
  skylanders::dev::WatchLevelArchives(&LevelOfArchive);
}

}  // namespace swapforcerecomp

extern "C" void __imp__sub_82A4A828(PPCContext& __restrict ctx, uint8_t* base);

REX_HOOK_RAW(sub_82A4A828) {
  const uint32_t component = ctx.r3.u32;
  const uint32_t handle = GuestPointer(component) ? Load32(base, component + 12) : 0;
  if (GuestPointer(handle) && !(Load32(base, handle) & 0x03000000)) {
    const uint32_t entity = Load32(base, handle + 20);
    if (GuestPointer(entity)) {
      skylanders::dev::ReportPlayerFrame(component, LoadFloat(base, entity + 40), LoadFloat(base, entity + 44),
                                         LoadFloat(base, entity + 48), false);
    }
  }
#if defined(SKYLANDERS_DEVKIT)
  // The developer kit (a private test build only) does its per-player work here as well.
  DevKitPlayerCheck(ctx, base);
#endif
  __imp__sub_82A4A828(ctx, base);
}
