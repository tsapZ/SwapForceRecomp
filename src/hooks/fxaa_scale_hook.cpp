// The game's own anti-aliasing at the real resolution.
//
// Swap Force runs NVIDIA FXAA 3.11 (its console / Xbox 360 variant) over the 3D scene every frame:
// CFxaaMaterial with a CFxaaParametersAttr, shader shaders:/Fxaa.fx. The sample offsets come from
// sub_82B8E340, which turns the frame size (f1 = width, f2 = height, f3 = 0.5) into the attr's
// fxaaQualityRcpFrame (1/w, 1/h), fxaaConsoleRcpFrameOpt (+-0.5/w, +-0.5/h), Opt2 and the 360 Opt2
// (8/w, 8/h, -4/w, -4/h). The game passes its own scene size, 1040x620. With resolution_scale the
// scene is drawn that many times larger, so the shader sampled that many host pixels apart:
// softer edges and only part of the smoothing. Multiplying the size by the draw resolution scale
// puts the samples one pixel apart again, as on the console (Cemu's Wii U graphic pack does the
// same with its "antialias scaling").
//
// Found 2026-10-19: a RenderDoc capture of the hub (event 9310 reads the 2080x1240 scene and binds
// c0-c5 = RcpFrameOpt, 360 RcpFrameOpt2, 8, 0.125, 0.05, (1, -1, 0.25, -0.25)); then a write watch
// (src/dev/write_watch.cpp) on the D3D constant shadow led to sub_824F8AE8 (material constants),
// and one on the attr fields to their writer, sub_82B8E340 (15 calls per second per attr copy).
// The function only writes the attr (four vectors at r3 + 16..64), so scaling its two inputs
// changes nothing else.

#include <algorithm>
#include <atomic>
#include <cstdint>

#include <rex/cvar.h>
#include <rex/logging.h>
#include <rex/ppc/context.h>

REXCVAR_DEFINE_BOOL(fxaa_follow_resolution_scale, true, "Graphics",
                    "Let the game's own FXAA work at the internal resolution: its sample offsets "
                    "assume the console's 1040x620, so without this it blurs at resolution_scale "
                    "2 and up");

namespace {

// The draw resolution scale the GPU code uses (TextureCache::GetConfigDrawResolutionScale, which
// lives in the GPU plugin): resolution_scale for both axes unless draw_resolution_scale_x/y are set.
uint32_t DrawScale(const char* axis_cvar) {
  const auto shared = uint32_t(std::max(1, rex::cvar::Query<int32_t>("resolution_scale")));
  const bool use_shared = rex::cvar::HasNonDefaultValue("resolution_scale") &&
                          !rex::cvar::HasNonDefaultValue(axis_cvar);
  const uint32_t scale =
      use_shared ? shared : uint32_t(std::max(1, rex::cvar::Query<int32_t>(axis_cvar)));
  return std::min<uint32_t>(scale, 7);
}

}  // namespace

void FxaaFrameSizeHook(PPCRegister& f1, PPCRegister& f2) {
  if (!REXCVAR_GET(fxaa_follow_resolution_scale)) return;
  static const uint32_t scale_x = DrawScale("draw_resolution_scale_x");
  static const uint32_t scale_y = DrawScale("draw_resolution_scale_y");
  static std::atomic<bool> logged{false};
  if (!logged.exchange(true)) {
    REXLOG_INFO("FXAA: frame {}x{} scaled by {}x{}", f1.f64, f2.f64, scale_x, scale_y);
  }
  f1.f64 *= scale_x;
  f2.f64 *= scale_y;
}
