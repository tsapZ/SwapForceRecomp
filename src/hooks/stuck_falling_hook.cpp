// The game's "stuck while falling" check at any frame rate.
//
// CPlayerJumpComponent (vtable 0x820AF574) checks every frame, in sub_82A4A828, whether a falling
// player (state +0x18 == 3) is stuck: if it moved less than CPlayerSystemData's
// _stuckFallingDistanceThreshold (+0x5C, 20, times the component's +0x68) since the last frame,
// its _stuckFallingTimer (+0x8C) runs, and after _stuckFallingTime (+0x60, 3 s) sub_82A4A5D0(..,
// 4, 0) puts the player back. Moving more resets the timer. The threshold is a distance per frame,
// so it was tuned for 30 FPS (600 units per second): at 240 FPS it means 4800, faster than a
// player falls, so every fall counts as stuck and one longer than 3 s ends with the player put
// back, over and over on a long drop. Cemu's Wii U FPS pack has the same fix ("infinite fall fix":
// 20 * 30 / target FPS in place of the threshold load).
//
// Here the threshold is scaled by the game's own frame time (30 * dt, at most 1), so it means the
// same speed as at 30 FPS at any frame rate, also one that varies. At 30 FPS and below nothing
// changes. dt is the update context's +0x14 (sub_82A40670 integrates the fall speed with it); the
// only caller, sub_82A4A950, keeps that context in r30, which sub_82A4A828 saves only after its
// entry.
//
// Found 2026-10-19: the field names come from the game's reflection tables (CPlayerSystemData's
// names at 0x820AF9C8 with offsets at 0x820AFA84, CPlayerJumpComponent's at 0x820ACB30 /
// 0x820ACC50), the classes from their RTTI, and sub_82A4A828 is the one function that reads +0x5C
// and +0x60 of the system data and the timer at +0x8C of the component. Measured in the hub with
// log_stuck_falling, launching the player with the component's bounce pad fields (+0x30 on, +0x3C
// up speed 1500 / 3000 / 5000): at 30 FPS each fall counts as stuck for 0.43-0.50 s (the slow part
// after the top), at 240 FPS without the fix for 0.84 s, 2.05 s and over 3 s, and the 5000 one
// ended with the player put back; with the fix 0.26-0.39 s and never put back.

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <mutex>

#include <rex/cvar.h>
#include <rex/logging.h>
#include <rex/memory/utils.h>
#include <rex/ppc/context.h>
#include <rex/system/kernel_state.h>

REXCVAR_DEFINE_BOOL(fix_stuck_falling, true, "Game",
                    "Above 30 FPS, keep the game from taking a normal fall for being stuck (its "
                    "check counts the distance per frame, tuned for 30 FPS)");
REXCVAR_DEFINE_BOOL(log_stuck_falling, false, "Game",
                    "Log how long each fall counted as stuck (for testing fix_stuck_falling)");
REXCVAR_DEFINE_BOOL(log_jumps, false, "Game",
                    "Log each jump's height and time to the top (for testing high frame rates)");

namespace {

using Clock = std::chrono::steady_clock;

// The threshold scale for the check that is running on this thread.
thread_local double t_scale = 1.0;

float LoadFloat(uint32_t guest_address) {
  return rex::memory::load_and_swap<float>(
      rex::system::kernel_state()->memory()->virtual_membase() + guest_address);
}

// log_stuck_falling: per component, the current run of checks that counted as stuck.
struct Run {
  uint32_t component = 0;
  Clock::time_point since{};
  bool stuck = false;
  bool checked = false;  // the threshold check ran since the last entry
  // log_jumps: the component's state (+0x18) at the last check, and when it went to 2 (rising).
  uint32_t state = 0;
  double rise_time = 0;  // game time, summed from dt
  float top_z = 0;
};

std::mutex g_runs_mutex;
std::array<Run, 8> g_runs;

Run& RunFor(uint32_t component) {
  for (auto& r : g_runs) {
    if (r.component == component) return r;
  }
  for (auto& r : g_runs) {
    if (!r.stuck) {
      r = {component};
      return r;
    }
  }
  g_runs[0] = {component};
  return g_runs[0];
}

void EndRun(Run& r, Clock::time_point now) {
  if (!r.stuck) return;
  r.stuck = false;
  const double seconds = std::chrono::duration<double>(now - r.since).count();
  if (seconds >= 0.05) {
    REXLOG_INFO("Stuck falling: counted as stuck for {:.2f} s (component {:08X})", seconds,
                r.component);
  }
}

}  // namespace

// sub_82A4A828's entry: r3 = the CPlayerJumpComponent, r30 = the caller's update context.
void StuckFallingCheckHook(PPCRegister& r3, PPCRegister& r30) {
  const float dt = LoadFloat(r30.u32 + 0x14);
  t_scale = dt > 0.0f && dt < 1.0f ? std::min(1.0, double(dt) * 30.0) : 1.0;
  if (REXCVAR_GET(log_stuck_falling) || REXCVAR_GET(log_jumps)) {
    std::lock_guard lock(g_runs_mutex);
    Run& run = RunFor(r3.u32);
    if (!run.checked) EndRun(run, Clock::now());  // the previous check was not a fall
    run.checked = false;
    if (REXCVAR_GET(log_jumps)) {
      // State 2 = rising, 3 = falling; +0x98 = the position's z at the last check, +0x54 = the
      // jump's start z (_jumpStartLocation).
      const uint32_t state = rex::memory::load_and_swap<uint32_t>(
          rex::system::kernel_state()->memory()->virtual_membase() + r3.u32 + 0x18);
      const float z = LoadFloat(r3.u32 + 0x98);
      if (state == 2) {
        if (run.state != 2) run.rise_time = 0;
        run.rise_time += dt;
        run.top_z = z;
      } else if (state == 3 && run.state == 2) {
        REXLOG_INFO("Jump: top {:.1f} above the start after {:.3f} s (dt {:.4f})",
                    std::max(run.top_z, z) - LoadFloat(r3.u32 + 0x54), run.rise_time, dt);
      }
      run.state = state;
    }
  }
}

// Before 0x82A4A8A0 (fmuls f11,f13,f12): f13 = _stuckFallingDistanceThreshold, f12 = the
// component's factor, f0 = the squared distance since the last check, r30 = CPlayerSystemData,
// r31 = the component.
void StuckFallingThresholdHook(PPCRegister& f0, PPCRegister& f13, PPCRegister& f12,
                               PPCRegister& r30, PPCRegister& r31) {
  static std::atomic<bool> logged{false};
  if (!logged.exchange(true)) {
    REXLOG_INFO("Stuck falling: threshold {} per frame, factor {}, time {} s, frame scale {:.3f}",
                f13.f64, f12.f64, LoadFloat(r30.u32 + 0x60), t_scale);
  }
  if (REXCVAR_GET(fix_stuck_falling)) f13.f64 = double(float(f13.f64 * t_scale));
  if (REXCVAR_GET(log_stuck_falling)) {
    std::lock_guard lock(g_runs_mutex);
    Run& run = RunFor(r31.u32);
    run.checked = true;
    const double limit = f13.f64 * f12.f64;
    if (f0.f64 < limit * limit) {
      if (!run.stuck) {
        run.stuck = true;
        run.since = Clock::now();
      }
    } else {
      EndRun(run, Clock::now());
    }
  }
}

// Before 0x82A4A8D8: the timer ran past _stuckFallingTime (f31; f1 = the timer) and the game puts
// the player back.
void StuckFallingRespawnHook(PPCRegister& f1, PPCRegister& f31) {
  REXLOG_INFO("Stuck falling: player put back after {:.2f} s (limit {:.2f} s)", f1.f64, f31.f64);
}
