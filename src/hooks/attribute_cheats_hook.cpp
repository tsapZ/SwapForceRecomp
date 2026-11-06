// Always crit, elemental power everywhere, gold and XP multipliers, as cheats (CoL #48, TT #20 and
// the multipliers asked for in CoL #48, darkmattereli-hue).
//
// Swap Force keeps a Skylander's stats in a character attribute component: floats at its +36, one
// per eCA_* attribute (index i at +(i + 2) * 4; the enum's names at 0x8201DE80, registered by
// sub_8274FA98). The script bindings registered by sub_827584D0 (names beside their wrappers on the
// stack, found with debug/func_sim.py) lead to the getters, and each getter has one C++ caller
// besides its wrapper:
// - getCriticalHitChance_Internal, sub_82A0F220: eCA_CriticalHitChance (+124). The damage code
//   sub_82A09A68 adds it to the attack's own chance and rolls a number from 0 to 100 against the
//   sum (Park-Miller, the constants 0 and 100 at 0x8201DA0C); below it, the hit is critical.
// - sub_82A18978: whether the Skylander's element is one of the area's (the list at +1120 of the
//   level's data); only the attribute getters call it (food, resistance, strength, the element
//   bonus, XP and gold), which then add the eCA_*ElementalBoost attributes.
// - getGoldMultiplier_Internal, sub_82A19348: the pickup code sub_82985028 multiplies a gold
//   pickup's amount by it.
// - getXpMultiplier_Internal, sub_82A19260 (with the catch-up for a Skylander below the level's
//   target, _xpMultiplierPerLevelBelowTarget): sub_82980EA8 multiplies an XP pickup (type 14) by it.
//
// Four midasm hooks (config/default.toml): the crit one where the damage code adds the chance, the
// others at the getters' one return; each does nothing while its cheat is off. Nothing is written to the game's memory; the gold and the levels a Skylander
// gets are saved to its figure by the game as usual.

#include <algorithm>

#include <rex/cvar.h>
#include <rex/logging.h>
#include <rex/ppc/context.h>

#include "portal_rex/achievements_ui.h"

REXCVAR_DEFINE_BOOL(cheat_always_crit, false, "Extras",
                    "Every hit of your Skylanders is a critical hit");
REXCVAR_DEFINE_BOOL(cheat_elemental_power, false, "Extras",
                    "Your Skylanders get their element's bonus in every area, as if each area were "
                    "theirs");

namespace {

void LogOnce(bool& logged, const char* what, double before, double after) {
  if (!logged) {
    logged = true;
    REXLOG_INFO("Extras: {} ({:.2f} -> {:.2f})", what, before, after);
  }
}

}  // namespace

// Before 0x82A09B04 in sub_82A09A68 (fadds f31,f1,f31): f1 is the attacker's eCA_CriticalHitChance,
// just returned by sub_82A0F220. With 100 added, the roll (0 to 100, never 100) always lands below
// the sum. The getter itself is left alone, so the scripts (the stats) see the real value.
void AlwaysCritHook(PPCRegister& f1) {
  if (!REXCVAR_GET(cheat_always_crit)) return;
  static bool logged = false;
  const double before = f1.f64;
  f1.f64 = std::max(f1.f64, 100.0);
  LogOnce(logged, "always crit raised a critical hit chance", before, f1.f64);
}

// Before 0x82A18A28 in sub_82A18978, after mr r3,r30: r3 is whether the element is the area's.
void ElementalPowerHook(PPCRegister& r3) {
  if (!REXCVAR_GET(cheat_elemental_power)) return;
  static bool logged = false;
  LogOnce(logged, "elemental power made an area's element check pass", double(r3.u64 & 0xFF), 1.0);
  r3.u64 = 1;
}

// Before the blr of sub_82A19348: f1 is the gold multiplier.
void GoldMultiplierHook(PPCRegister& f1) {
  const int k = skylanders::CheatGoldMultiplier();
  if (k <= 1) return;
  static bool logged = false;
  const double before = f1.f64;
  f1.f64 = double(float(f1.f64 * k));
  LogOnce(logged, "the gold multiplier scaled the game's own", before, f1.f64);
}

// Before the blr of sub_82A19260: f1 is the XP multiplier.
void XpMultiplierHook(PPCRegister& f1) {
  const int k = skylanders::CheatXpMultiplier();
  if (k <= 1) return;
  static bool logged = false;
  const double before = f1.f64;
  f1.f64 = double(float(f1.f64 * k));
  LogOnce(logged, "the XP multiplier scaled the game's own", before, f1.f64);
}
