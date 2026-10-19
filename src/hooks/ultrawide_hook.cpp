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
// one too, keeping the vertical FOV. The HUD is drawn into the same 16:9 frame and is stretched.
//
// Found 2026-10-19: the camera fields from their reflection tables (_fov, _aspect, ...,
// _projectionMatrix, _updateProjectionOnSizeChange at 0x822CE280 / offsets 0x822CE330), the live
// camera by its 16:9 value in the hub, and a read watch on its _aspect.

#include <atomic>
#include <cmath>
#include <cstdint>
#include <string>

#include "ultrawide_hook.h"

#include <rex/cvar.h>
#include <rex/graphics/video_mode_util.h>
#include <rex/logging.h>
#include <rex/ppc/context.h>

REXCVAR_DECLARE(std::string, resolution);

REXCVAR_DEFINE_BOOL(ultrawide_fov, true, "Graphics",
                    "On a window wider than 16:9 (resolution), show more to the sides instead of "
                    "black bars: a wider field of view and the picture stretched across the "
                    "window. The HUD is stretched with it");

namespace {

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
