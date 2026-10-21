// Ultrawide: a wider field of view for the game's 3D camera when the window is wider than 16:9.
//
// Every frame the renderer (sub_82B992A8, from sub_82BA36C8) sets up the scene camera: it stores
// the display's aspect ratio (16:9) at camera +0x34 (_aspect) and the field of view at +0x30
// (_fov, 80 in the hub), then builds the projection at camera +0xB0 (_projectionMatrix) with
// sub_824906D8(r3 = matrix, f1 = horizontal FOV, f2 = vertical FOV, f3 = aspect, f4 = near,
// f5 = far; radians). That builder takes the horizontal FOV when f1 >= 0 and derives the vertical
// one from it and the aspect (else the other way round), then makes an off-center frustum
// (sub_82490630). The scene camera passes the horizontal one. The guest frame stays 16:9
// (1280x720, the scene 1040x620); with present_letterbox off (the app turns it off while this is
// active, unless set by hand) the presenter stretches it across the wider window, so the camera
// has to see that much more to the sides for the picture to keep its proportions ("Hor+"): the
// hook at the call (0x82B993A4) widens the aspect and, if the call gives a horizontal FOV, that
// one too, keeping the vertical FOV. The HUD is drawn into the same 16:9 frame and would be
// stretched.
//
// The HUD (ultrawide_hud): the UI (Juice) runs in contexts whose reflection fields (the name list
// at 0x822F32C8) start _renderer, _viewerWidthMain, _viewerHeightMain, _settingWidthMain,
// _settingHeightMain, _screenWidthSub, _screenHeightSub, _expectedScreenWidthMain,
// _expectedScreenHeightMain, ...: in the objects (vtable 0x822F7D90) +0x0C / +0x10, +0x14 /
// +0x18 and +0x24 / +0x28, all 1280 x 720 (floats). Setting the three widths to the window's width
// at the UI's height (720 x 2560 / 1080 = 1707) in the hub, by the input script's poke on all
// three contexts, made the HUD keep its proportions and moved it to the window's left edge on the
// next frame: Juice lays out for that wide a screen and squeezes it into the 16:9 frame, which the
// presenter stretches back (2026-10-21). One field at a time: only the expected width does it (the
// viewer's or the setting's alone leave the HUD stretched), so the hook sets that one, where the
// context's sizes are read for drawing (sub_82642E20, lfs f0,12(r11) at 0x82642E5C, r11 = the
// context; the second-screen branch reads +0x1C / +0x20 and is left alone), and puts the game's own
// back when ultrawide is off. Off by default: the pause menu's dimming is laid out 1280 wide and
// leaves a light strip at the right, and full-screen fades may do the same.
//
// Found 2026-10-19: the camera fields from their reflection tables (_fov, _aspect, ...,
// _projectionMatrix, _updateProjectionOnSizeChange at 0x822CE280 / offsets 0x822CE330), the live
// camera by its 16:9 value in the hub, and a read watch on its _aspect.
//
// Movies: the logos and the opening movie are 16:9 Bink movies drawn into the 16:9 frame, which
// the presenter would stretch across the window like the 3D picture. While one plays the
// presenter keeps black bars at the sides instead (present_use_actual_aspect off and
// present_letterbox on just for that time, not saved). BinkOpen's header hook
// (intro_skip_hook.cpp) marks the start, the start of BinkClose (sub_83084128, called only from the
// game's movie close sub_825A7620, which then clears the movie's handle) the end.

#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <string>

#include "ultrawide_hook.h"

#include <rex/cvar.h>
#include <rex/graphics/video_mode_util.h>
#include <rex/logging.h>
#include <rex/memory.h>
#include <rex/memory/utils.h>
#include <rex/ppc/context.h>
#include <rex/ui/flags.h>
#include <rex/system/kernel_state.h>

REXCVAR_DECLARE(std::string, resolution);

REXCVAR_DEFINE_BOOL(ultrawide_fov, true, "Graphics",
                    "On a window wider than 16:9 (resolution), show more to the sides instead of "
                    "black bars: a wider field of view and the picture stretched across the "
                    "window");
REXCVAR_DEFINE_BOOL(ultrawide_hud, false, "Graphics",
                    "Experimental: with ultrawide_fov on a wider window, the HUD and the menus at "
                    "the window's edges in their own proportions instead of stretched; the pause "
                    "menu's dimming then leaves a strip at the right");

namespace {

// True while a movie plays with the presenter's bars turned on for it; the aspect setting it
// changed for that, to put back.
std::atomic<bool> g_movie_bars{false};
std::atomic<bool> g_movie_actual_aspect{true};

constexpr double kGameAspect = 16.0 / 9.0;

// The window's aspect ratio from `resolution`, or 0 if it isn't set.
double WindowAspect() {
  int32_t width = 0, height = 0;
  if (!rex::graphics::video_mode_util::TryParseResolutionPreset(REXCVAR_GET(resolution), width,
                                                                height) ||
      width <= 0 || height <= 0) {
    return 0;
  }
  return double(width) / double(height);
}

}  // namespace

namespace swapforcerecomp {

// Whether the picture should fill a wider window (the presenter then must not letterbox).
bool UltrawideActive() {
  return REXCVAR_GET(ultrawide_fov) && WindowAspect() > kGameAspect * 1.01;
}


void MovieOpened(uint32_t width, uint32_t height) {
  // Only when the picture fills the wider window (the app turned the bars off for ultrawide).
  if (!UltrawideActive() || REXCVAR_GET(present_letterbox) || g_movie_bars.exchange(true)) return;
  // The presenter takes the frame's aspect from the window's size (present_use_actual_aspect),
  // so the 16:9 frame counts as wide as the window: off for the movie, the console's 16:9 video
  // mode then gives the aspect and present_letterbox the bars.
  g_movie_actual_aspect = rex::cvar::Query<bool>("present_use_actual_aspect");
  rex::cvar::SetFlagByName("present_use_actual_aspect", "false");
  rex::cvar::SetFlagByName("present_letterbox", "true");
  REXLOG_INFO("Ultrawide: movie {}x{} with bars at the sides", width, height);
}

void MovieClosed() {
  if (!g_movie_bars.exchange(false)) return;
  rex::cvar::SetFlagByName("present_letterbox", "false");
  rex::cvar::SetFlagByName("present_use_actual_aspect", g_movie_actual_aspect ? "true" : "false");
}

}  // namespace swapforcerecomp

// Before 0x82B993A4 (bl sub_824906D8): f1 = horizontal FOV (radians; negative when the vertical
// one is given), f2 = vertical FOV, f3 = aspect.
void UltrawideFovHook(PPCRegister& f1, PPCRegister& f2, PPCRegister& f3) {
  static std::atomic<bool> logged{false};
  if (!logged.exchange(true)) {
    REXLOG_INFO("Ultrawide: camera FOV h {} v {}, aspect {}", f1.f64, f2.f64, f3.f64);
  }
  if (!swapforcerecomp::UltrawideActive() || f3.f64 <= 0) return;
  const double wide = WindowAspect();
  if (wide <= f3.f64) return;
  const double scale = wide / f3.f64;
  if (f1.f64 >= 0) {
    // Same vertical FOV: tan(h'/2) = tan(h/2) * scale.
    f1.f64 = double(float(2.0 * std::atan(std::tan(f1.f64 * 0.5) * scale)));
  }
  f3.f64 = double(float(wide));
}

// At 0x82642E5C (lfs f0,12(r11) in sub_82642E20): r11 = a Juice context, its main screen's
// expected size at +0x24 / +0x28.
void UltrawideHudHook(PPCRegister& r11) {
  if (!r11.u32) return;
  uint8_t* context = rex::system::kernel_state()->memory()->virtual_membase() + r11.u32;
  auto load = [](const uint8_t* p) {
    const uint32_t bits = rex::memory::load_and_swap<uint32_t>(p);
    float v;
    std::memcpy(&v, &bits, 4);
    return v;
  };
  auto store = [](uint8_t* p, float v) {
    uint32_t bits;
    std::memcpy(&bits, &v, 4);
    rex::memory::store_and_swap<uint32_t>(p, bits);
  };
  constexpr uint32_t kWidths[] = {0x24};  // _expectedScreenWidthMain, then its height
  // The width written here, so that any other value counts as the game's own.
  static std::atomic<uint32_t> written_bits{0};
  const bool wide = REXCVAR_GET(ultrawide_hud) && swapforcerecomp::UltrawideActive();
  for (uint32_t field : kWidths) {
    const float width = load(context + field), height = load(context + field + 4);
    if (height <= 0) continue;
    uint32_t width_bits;
    std::memcpy(&width_bits, &width, 4);
    const bool ours = width_bits == written_bits.load();
    float want = width;
    if (wide && !ours) {
      want = float(double(height) * WindowAspect());
    } else if (!wide && ours) {
      want = float(double(height) * 16.0 / 9.0);  // the game's own: 16:9 at its height
    }
    if (want == width) continue;
    store(context + field, want);
    if (wide) {
      uint32_t bits;
      std::memcpy(&bits, &want, 4);
      written_bits = bits;
      static std::atomic<bool> logged{false};
      if (!logged.exchange(true)) REXLOG_INFO("Ultrawide: UI {} x {} -> {} wide", width, height, want);
    }
  }
}

// At the start of BinkClose (sub_83084128): the movie is over (or skipped).
void BinkCloseHook() {
  swapforcerecomp::MovieClosed();
}
