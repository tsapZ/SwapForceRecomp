// Opens the portal menu when the game asks a player for a Skylander.
//
// The game runs everything around "place a Skylander on the Portal of Power" through one engine
// object, the CMagicMoment singleton. Found statically in Ghidra (2026-10-18): the UI string
// "MM_pleaseplaceText" (0x82110260) is opened by EnterIdle (sub_829B8C10), which then sets the
// state to Idle; the reflection tables of the class (name "CMagicMoment" at 0x82099FD0, field
// names at 0x82099020, u16 field offsets at 0x820993E0, enum value tables at 0x822E18B0) give the
// fields read here:
//   0x83131BE0  global pointer to the instance (set by vtable[5] sub_82858F70, cleared by
//               vtable[6] sub_82858FC0; GetInstance is sub_82865A10)
//   +0x20       _magicMomentState: 5 = Idle (the prompt is up, waiting for a figure),
//               7 = IdleOutro (the figure arrived), 0 = Inactive
//   +0xE8       _reason: 0 = ToyRemoved, 5 = PlayerDeath, 6 = FirstTime, 7 = MapStart, ...
//   +0x148      _currentPlayerId: 0 = player 1, 1 = player 2, -1 = none
// When a second player joins without a figure, the activation check (sub_829C2888) requests the
// same sequence for player index 1, so player 2's prompt shows up here as well. With several
// players waiting, sub_829BFA10 moves on to the next player while the state stays at Idle.
//
// Polling instead of a mid-asm hook keeps the generated code untouched and costs three guest
// reads every 30 ms. It only reads: every address is checked to be committed guest memory before
// it is touched, and implausible values are ignored, so a stale pointer can't do harm.

#include "magic_moment_watch.h"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <optional>
#include <thread>

#include <rex/logging.h>
#include <rex/system/xmemory.h>

#include "guest_memory.h"
#include "portal_rex/portal_menu.h"

namespace swapforcerecomp {
namespace {

using Clock = std::chrono::steady_clock;

constexpr uint32_t kMagicMomentInstance = 0x83131BE0;
constexpr uint32_t kStateOffset = 0x20;
constexpr uint32_t kReasonOffset = 0xE8;
constexpr uint32_t kPlayerOffset = 0x148;
constexpr uint32_t kStateIdle = 5;
// How long the game must keep waiting before the menu opens, so it comes up with the prompt
// rather than during a one-frame state flicker.
constexpr auto kOpenDelay = std::chrono::milliseconds(100);

std::thread g_thread;
std::atomic<bool> g_stop{false};
std::atomic<bool> g_active{false};  // state != Inactive at the last poll

struct Prompt {
  uint32_t player;
  uint32_t reason;
};

// The player the game is waiting for, if its "place your Skylander" prompt is up.
std::optional<Prompt> CurrentPrompt(rex::memory::Memory* memory) {
  const auto instance = ReadGuest32(memory, kMagicMomentInstance);
  // The instance lives in physical memory (seen at 0xBF6B816C); ReadGuest32 checks that whatever
  // the pointer says is committed.
  if (!instance || !*instance) {
    g_active.store(false);
    return std::nullopt;
  }
  const auto state = ReadGuest32(memory, *instance + kStateOffset);
  g_active.store(state && *state != 0 && *state <= 16);
  const auto player = ReadGuest32(memory, *instance + kPlayerOffset);
  const auto reason = ReadGuest32(memory, *instance + kReasonOffset);
  if (!state || !player || !reason || *state != kStateIdle || *player >= 4) return std::nullopt;
  return Prompt{*player, *reason};
}

void Watch(rex::memory::Memory* memory) {
  std::optional<uint32_t> waiting_for;  // player whose prompt is up
  Clock::time_point since{};
  bool opened = false;
  while (!g_stop.load()) {
    const std::optional<Prompt> prompt = CurrentPrompt(memory);
    const std::optional<uint32_t> player =
        prompt ? std::optional<uint32_t>(prompt->player) : std::nullopt;
    if (player != waiting_for) {
      if (waiting_for && opened) skylanders::CloseAutoOpenedPortalMenu();
      waiting_for = player;
      since = Clock::now();
      opened = false;
    }
    if (waiting_for && !opened && Clock::now() - since >= kOpenDelay) {
      REXLOG_INFO("Magic moment: the game asks player {} for a Skylander (reason {})",
                  *waiting_for + 1, prompt->reason);
      skylanders::AutoOpenPortalMenu(*waiting_for);
      opened = true;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(30));
  }
}

}  // namespace

void StartMagicMomentWatch(rex::memory::Memory* memory) {
  if (!memory || g_thread.joinable()) return;
  g_stop.store(false);
  g_thread = std::thread(Watch, memory);
}

bool MagicMomentActive() { return g_active.load(); }

void StopMagicMomentWatch() {
  g_stop.store(true);
  if (g_thread.joinable()) g_thread.join();
}

}  // namespace swapforcerecomp
