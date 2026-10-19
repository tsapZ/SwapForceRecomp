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
// State: with 1280x720 every pass gets the new size (64 instead of 86 FPS in the hub), but the
// four G-buffer surfaces need 4 x 720 = 2880 EDRAM tiles of 2048; the engine shrinks one (the SDK
// reports "Resolve region 0 <= x < 1280 is outside the surface pitch 880") and the 3D scene is
// black. Next: the engine's EDRAM surface allocation and the SDK fork's 4096-tile EDRAM.

#include <atomic>
#include <cstdint>
#include <cstdio>
#include <string>

#include <rex/cvar.h>
#include <rex/logging.h>
#include <rex/memory.h>
#include <rex/memory/utils.h>
#include <rex/ppc/context.h>
#include <rex/system/kernel_state.h>

REXCVAR_DEFINE_STRING(scene_size, "", "Graphics",
                      "EXPERIMENTAL, not working yet: draw the 3D scene at this size instead of "
                      "the Xbox 360's 1040x620, e.g. 1280x720. For now the scene stays black: "
                      "its surfaces no longer fit into the EDRAM (#41). Empty: the game's size")
    .lifecycle(rex::cvar::Lifecycle::kRequiresRestart);

namespace {

constexpr uint32_t kWidthOffset = 0x20;
constexpr uint32_t kHeightOffset = 0x24;
constexpr int32_t kGameWidth = 1040;
constexpr int32_t kGameHeight = 620;

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
  } else if (!text.empty()) {
    REXLOG_WARN("Scene size: '{}' is not WIDTHxHEIGHT; keeping the game's size", text);
  }
  return size;
}

}  // namespace

void SceneSizeHook(PPCRegister& r3) {
  static const Size size = ConfiguredSize();
  if (!size.width) return;
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
