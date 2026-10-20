// Draws the 3D scene at another size than the Xbox 360's 1040x620 (#41, experimental).
//
// The render pipeline is data (renderer\outdoor_shading_xenon.igx; the game parses all platforms'
// copies at start-up): every render target is an igRenderTarget, and only "mainScene" has a size
// of its own, 1040x620. The others follow it through _parentTarget and _widthScale/_heightScale.
// The same pipeline uses 1024x576 on PS3 and Wii U and 1920x1080 on PS4 and Xbox One; 1040x620 is
// what fits four 32-bit surfaces into the 360's EDRAM (13 x 39 = 507 tiles each, 2048 in all).
//
// sub_8258EBE8 computes a target's size (r3 = the igRenderTarget): its own _width / _height
// (+0x20 / +0x24, -1 when unset) or else its parent's computed size or the back buffer's, times
// _widthScale / _heightScale (+0x28 / +0x2C), stored at +0x58 / +0x5C. It runs when the targets are
// (re)built, at start-up and on level loads. The hook at its start puts scene_size into a target
// that says 1040x620 (the mainScene of the 360 and PC pipelines), so the whole pipeline follows.
//
// Found 2026-10-19 with the write watch (src/dev/write_watch.cpp): the 1040 of mainScene is written
// by the IGX parser (sub_82455FD8 -> sscanf) while it parses outdoor_shading_xenon, and read only by
// this function, on the way from the title screen to the hub.
//
// With 1280x720 every pass gets the new size (64 instead of 86 FPS in the hub), but the G-buffer
// pass needs 4 x 720 = 2880 EDRAM tiles of the 360's 2048, and the 3D scene was black. Where the
// surfaces go: sub_82514448 (per render pass) puts the colour surfaces one after the other from
// tile 0 (Xenon surface _edramBase +0x80, _edramTileCount +0x84, counted by sub_82516EB8 from
// _actualWidth / _actualHeight +0x6C / +0x70) and the depth surface at the top,
// 2048 - its tiles (subfic r9,r11,0x800 at 0x82514608), with _overrideBase (+0x7D) skipping both.
// At 1280x720 the colours end at 2160 and the depth starts at 1328, so they overlap. Found
// 2026-10-19 from the reflection field table at 0x822BB340 (offsets at 0x822BB3B0), XGSurfaceSize
// (sub_82F5A0D8: tiles of 80x16 samples, 5120 bytes) and XGSetSurfaceHeader (sub_82F5B778, which
// keeps 12 bits of the base) up to their only caller with an allocation.
//
// Before that, sub_825138F8 decides whether a pass fits at all: it adds up the pass's tiles (rows x
// columns x the surfaces' 32-bit units; the G-buffer at 1280x720: 45 x 16 x 4 = 2880) and, above
// 2048 (cmpwi cr6,r5,0x800 at 0x82513A08), switches to predicated tiling: the screen is cut into
// strips that fit, 2048 / (45 x 4) = 11 tiles = 880 pixels and the remaining 400, and every
// surface of the pass gets the strip's size (_actualWidth / _actualHeight). The SDK doesn't
// emulate predicated tiling well, which is where the "surface pitch 880" and the black scene came
// from.
//
// That per-pass allocation is only the fallback. Normally the engine plans the whole frame once
// (sub_82593AD0): every render target gets an EDRAM range for its lifetime (_firstUse.._lastUse)
// from a heap of 2048 tiles at 1024 bytes per tile (lis r5,0x20 = 0x200000 before the heap's init
// call at 0x82593B14; a second heap of 3600 is for hierarchical Z), so that targets later passes
// read stay in the EDRAM. sub_82593870 allocates (sub_82472370), sub_825939E0 turns the address
// into _edramBase with _overrideBase set. At 1040x620 this gives the G-buffer depth 0 and colours
// 507 / 1014 / 1521. At 1280x720 the heap runs out, every target falls back to the per-pass
// allocation, later passes overwrite what earlier ones left in the EDRAM, and the scene goes black
// (traced 2026-10-19 with RenderDoc: the G-buffer and the lit scene were right, the scene texture
// the water pass read was not; and by logging every resolve's EDRAM base in both sizes).
//
// The SDK can emulate a 4096-tile EDRAM (edram_tile_count, D3D12, up to a resolution scale area of
// 25). With it, the hook at 0x82593B04 makes the planning heap 4096 tiles, the hook at 0x82513A08
// lets every pass that fits into 4096 tiles go untiled, and the hook at 0x8251460C moves a
// fallback depth surface to the top of that EDRAM, 4096 - its tiles. A
// scene_size larger than the game's turns edram_tile_count 4096 on by itself
// (swapforcerecomp_app.h). Passes larger than 4096 tiles (a scene above about 1440x810) are still
// tiled and stay broken.
//
// The scene must not be exactly the screen's size, 1280x720. At that size the game crashed in
// about half of the save loads (10 runs with the owner's settings, 2026-10-20), in two places: the
// per-pass resolve setup (sub_82513358 at 0x82513630) took a pooled texture record of the
// target's size whose resource pointer (+60) was 0, and the game heap's free (sub_82476C40) met a
// block header with one byte overwritten. At 1264x704 neither happened in 6 runs of the same test,
// so it is the size coinciding with the 1280x720 screen targets, not the memory a larger scene
// takes; presumably the game shares textures of one size between its scene and screen targets.
// So a configured 1280x720 draws 1264x704 (16 pixels less each way, 1.3 % stretched to the screen
// instead of 23 % from 1040x620).

#include <atomic>
#include <cstdint>
#include <cstdio>
#include <string>

#include <rex/cvar.h>
#include <rex/graphics/edram_layout.h>
#include <rex/logging.h>
#include <rex/memory.h>
#include <rex/memory/utils.h>
#include <rex/ppc/context.h>
#include <rex/system/kernel_state.h>

REXCVAR_DEFINE_STRING(scene_size, "", "Graphics",
                      "EXPERIMENTAL: draw the 3D scene at this size instead of the Xbox 360's "
                      "1040x620, e.g. 1280x720 (#41). Needs the larger emulated EDRAM, which it "
                      "turns on (edram_tile_count 4096, D3D12 and resolution_scale up to 5 only; "
                      "otherwise the game's size stays). Empty: the game's size")
    .lifecycle(rex::cvar::Lifecycle::kRequiresRestart);

namespace {

constexpr uint32_t kWidthOffset = 0x20;
constexpr uint32_t kHeightOffset = 0x24;
constexpr int32_t kGameWidth = 1040;
constexpr int32_t kGameHeight = 620;
// The game's screen (back buffer and HUD) size, which the scene must not match exactly.
constexpr int32_t kScreenWidth = 1280;
constexpr int32_t kScreenHeight = 720;
constexpr int32_t kScreenSizeStep = 16;

struct Size {
  int32_t width = 0, height = 0;
};

Size ConfiguredSize() {
  Size size;
  const std::string text = REXCVAR_GET(scene_size);
  unsigned w = 0, h = 0;
  if (!text.empty() && std::sscanf(text.c_str(), "%ux%u", &w, &h) == 2 && w >= 320 && h >= 240 &&
      w <= 7680 && h <= 4320) {
    size = {int32_t(w), int32_t(h)};
    if (size.width == kScreenWidth && size.height == kScreenHeight) {
      size = {kScreenWidth - kScreenSizeStep, kScreenHeight - kScreenSizeStep};
      REXLOG_INFO("Scene size: {}x{} is the screen's size, which crashes the game while loading "
                  "(#41); drawing {}x{} instead",
                  kScreenWidth, kScreenHeight, size.width, size.height);
    }
  } else if (!text.empty()) {
    REXLOG_WARN("Scene size: '{}' is not WIDTHxHEIGHT; keeping the game's size", text);
  }
  return size;
}

}  // namespace

// Before 0x82593B04 (li r4,0x400): r5 = 0x200000 is the size of the frame's EDRAM planning heap,
// 2048 tiles of 1024 bytes, passed to its init call two instructions on. With the larger EDRAM it
// gets that many tiles.
void EdramHeapSizeHook(PPCRegister& r5) {
  const uint32_t tiles = rex::graphics::ActiveEdramLayout().tile_count;
  if (tiles <= 2048 || r5.u32 != 2048 * 1024) return;
  r5.u64 = uint64_t(tiles) * 1024;
  static std::atomic<bool> logged{false};
  if (!logged.exchange(true)) REXLOG_INFO("Scene size: EDRAM planning heap of {} tiles", tiles);
}

// Before 0x82513A08 (cmpwi cr6,r5,0x800): r5 is the pass's tile count. Between 2048 and the larger
// EDRAM it becomes 2048, so the comparison takes the "fits" path (r5 isn't read on that path; the
// strip size it stores is then the whole surface).
void EdramTilingHook(PPCRegister& r5) {
  const uint32_t tiles = rex::graphics::ActiveEdramLayout().tile_count;
  if (tiles <= 2048 || r5.u32 <= 2048 || r5.u32 > tiles) return;
  static std::atomic<int> logged{0};
  if (logged.fetch_add(1) < 4) {
    REXLOG_INFO("Scene size: a pass of {} tiles fits into the {}-tile EDRAM, no tiling", r5.u32,
                tiles);
  }
  r5.u64 = 2048;
}

// Before 0x8251460C (cmplwi cr6,r10,0xe10): r9 = 2048 - r11 is the depth surface's EDRAM base,
// r11 its tile count; stw r9,0x80(r23) stores it next. With a 4096-tile EDRAM in use the depth
// goes to the top of that instead. Only r9 changes, and only when the EDRAM is the larger one.
void EdramDepthBaseHook(PPCRegister& r9, PPCRegister& r11) {
  const uint32_t tiles = rex::graphics::ActiveEdramLayout().tile_count;
  if (tiles <= 2048 || r11.u32 > tiles) return;
  r9.u64 = tiles - r11.u32;
  static std::atomic<int> logged{0};
  if (logged.fetch_add(1) < 4) {
    REXLOG_INFO("Scene size: depth surface of {} tiles at EDRAM tile {} of {}", r11.u32, r9.u32,
                tiles);
  }
}

void SceneSizeHook(PPCRegister& r3) {
  static const Size size = ConfiguredSize();
  if (!size.width) return;
  // Without the larger EDRAM (Vulkan, or a resolution scale area above 25) a larger scene doesn't
  // fit and would stay black; then the game keeps its own size.
  if (size.width * size.height > kGameWidth * kGameHeight &&
      !rex::graphics::ActiveEdramLayout().is_extended()) {
    static std::atomic<bool> warned{false};
    if (!warned.exchange(true)) {
      REXLOG_WARN("Scene size: {}x{} needs the 4096-tile EDRAM (D3D12, resolution scale up to 5); "
                  "keeping 1040x620",
                  size.width, size.height);
    }
    return;
  }
  uint8_t* target = rex::system::kernel_state()->memory()->virtual_membase() + r3.u32;
  if (rex::memory::load_and_swap<int32_t>(target + kWidthOffset) != kGameWidth ||
      rex::memory::load_and_swap<int32_t>(target + kHeightOffset) != kGameHeight) {
    return;
  }
  rex::memory::store_and_swap<int32_t>(target + kWidthOffset, size.width);
  rex::memory::store_and_swap<int32_t>(target + kHeightOffset, size.height);
  static std::atomic<int> logged{0};
  if (logged.fetch_add(1) < 4) {
    REXLOG_INFO("Scene size: render target {:08X} 1040x620 -> {}x{}", r3.u32, size.width,
                size.height);
  }
}
