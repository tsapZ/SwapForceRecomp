// A shorter figure intro when a Skylander comes onto the portal (#60, option, off by default).
//
// When a figure is placed, the game's CMagicMoment (magic_moment_watch.cpp) runs through its
// states; PlayIntro (10) shows the figure with its name and catchphrase for about 4 s, measured
// 2026-10-21 as most of the 7.4 s a figure change takes. sub_829BFFF8 checks every frame whether
// the intro is over: it takes the intro's length in f30 (the magic moment's tuning, +0x34 -> +0xB4,
// or +0xC4 for a swapper, unless the figure's own intro data at +0xDC has one), compares the time
// since the intro began (+0x88 times a constant) against it at 0x829C0114 (fcmpu cr6,f0,f30) and,
// when it is longer, calls sub_829BFA10, which ends the intro the game's way (sub_829AA900 sets
// state 11, TransitionOut, and gives the camera back), and marks it done (+0x1AE).
// Found 2026-10-21 with a write watch on the state field for 11 (input script `watch`): the
// writer sub_829AA900, called from sub_829BFA10, called from sub_829BFFF8.
//
// Writing state 11 by hand during the intro skipped it but left the camera on the sky; this hook
// only shortens the length the game compares against, so the game's own ending runs.

#include <atomic>
#include <cstdint>

#include <rex/cvar.h>
#include <rex/logging.h>
#include <rex/ppc/context.h>

REXCVAR_DEFINE_DOUBLE(figure_intro_seconds, 0.0, "Game",
                      "How long a Skylander's intro may last when it comes onto the portal, in "
                      "seconds (0: as long as the game's, about 4 s)");

// Before 0x829C0114 (fcmpu cr6,f0,f30): f0 is the time since the intro began, f30 its length.
// Only f30 changes, and only to a shorter length; f30 is saved by the function and not read after
// the comparison.
void FigureIntroLengthHook(PPCRegister& f0, PPCRegister& f30) {
  const double limit = REXCVAR_GET(figure_intro_seconds);
  static std::atomic<int> logged{0};
  if (logged.fetch_add(1) == 0) {
    REXLOG_INFO("Figure intro: {:.2f} s long (at {:.2f} s), limit {:.2f} s", f30.f64, f0.f64,
                limit);
  }
  if (limit > 0.0 && f30.f64 > limit) f30.f64 = limit;
}
