// Strength of the game's bloom and motion blur (Cemu's Wii U packs offer the same: bloom
// intensity, motion blur off).
//
// Both are set up each frame by the renderer from the level's post effect data, in two functions
// called by sub_82BA36C8:
// - sub_82BA31A0 copies CBloomData (_bloomLuminanceCutoff, _bloomInnerScale, _bloomOuterScale,
//   _bloomDiffuseScale at +0x0C..+0x18) into CBloomParametersAttr (+0x10..+0x1C, the shader's
//   ig_bloom_parameters), or 1.0 when the level has none. The hook at loc_82BA32C0 (r28 = the
//   attr, both paths meet there) scales the inner and outer scales: how much of the two blurred
//   bright-pass images BloomApply adds to the scene.
// - sub_82BA3390 takes CMotionBlurData's _motionBlurExposureTime (+0x0C, 0.35 in the hub) and stores
//   it at +0x90 of the motion blur attr; on a camera cut (flag +0x264) it stores 0 for one frame,
//   i.e. no blur. The hook at loc_82BA35CC (f0 = that value, stored next) scales it, so 0 is the
//   game's own "no blur" state; the pass still runs, so its colour grading (generatedLut) stays.
//
// Found 2026-10-19: the classes from the reflection strings and RTTI (CBloomParametersAttr vtable
// 0x82016274, CBloomData 0x8201361C, CMotionBlurData 0x8209B6B4), the live objects by scanning for
// those vtables in the hub, then a write watch on the attr's inner scale (only writer:
// sub_82BA31A0) and read watches on the motion blur data's fields (sub_82BA3390 on the render side,
// sub_82BB4090 on the game side, which blends the data and is left alone). Switching the passes
// off through igRenderPass._active instead is no good: bloomapply writes its own target
// (bloomedScene) that later passes read, and motionBlurPass also applies the colour LUT and writes
// the image FXAA reads.

#include <algorithm>
#include <cstdint>

#include <rex/cvar.h>
#include <rex/memory/utils.h>
#include <rex/ppc/context.h>
#include <rex/system/kernel_state.h>

REXCVAR_DEFINE_DOUBLE(bloom_strength, 1.0, "Graphics",
                      "Glow around bright areas: 0 = off, 1 = the game's, up to 3");
REXCVAR_DEFINE_DOUBLE(motion_blur_strength, 1.0, "Graphics",
                      "Motion blur: 0 = off, 1 = the game's, up to 2");

namespace {

constexpr uint32_t kInnerScale = 0x14;
constexpr uint32_t kOuterScale = 0x18;

void ScaleFloat(uint32_t guest_address, double scale) {
  uint8_t* p = rex::system::kernel_state()->memory()->virtual_membase() + guest_address;
  rex::memory::store_and_swap<float>(p,
                                     float(rex::memory::load_and_swap<float>(p) * scale));
}

}  // namespace

// loc_82BA32C0: r28 = CBloomParametersAttr, filled for this frame.
void BloomStrengthHook(PPCRegister& r28) {
  const double scale = std::clamp(REXCVAR_GET(bloom_strength), 0.0, 3.0);
  if (scale == 1.0) return;
  ScaleFloat(r28.u32 + kInnerScale, scale);
  ScaleFloat(r28.u32 + kOuterScale, scale);
}

// loc_82BA35CC: f0 = the exposure time stored into the motion blur attr next.
void MotionBlurStrengthHook(PPCRegister& f0) {
  const double scale = std::clamp(REXCVAR_GET(motion_blur_strength), 0.0, 2.0);
  if (scale == 1.0) return;
  f0.f64 = double(float(f0.f64 * scale));
}
