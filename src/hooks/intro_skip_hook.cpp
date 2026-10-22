// Skips the startup logo movies and, optionally, the opening movie of a new game.
//
// BinkOpen (sub_83081CE8) reads the 44-byte file header into r1+112 and then checks its signature
// (the KB2f/KB2g/BIKf..BIKi compares at 0x83081EB8). The midasm hook at 0x83081EB8
// (config/default.toml) runs in between, for Bink 1 and Bink 2 files alike. When the header belongs
// to one of the movies below, recognised by the file-size and frame-count fields that are unique to
// them, the frame count at +8 is set to 1. Bink then plays one frame and reports the movie finished,
// so the game moves on as if it had played out. Same approach as GiantsRecomp's intro skip.

#include <cstdint>

#include <rex/cvar.h>
#include <rex/logging.h>
#include <rex/memory.h>
#include <rex/memory/utils.h>
#include <rex/ppc/context.h>
#include <rex/system/kernel_state.h>

#include "ultrawide_hook.h"

// Off by default (the owner's choice, 2026-10-22): the game starts as on the console, with its
// logo movies; skipping them is the option.
REXCVAR_DEFINE_BOOL(skip_intro, false, "Game",
                    "Skip the Activision and Vicarious Visions logo movies at startup.");
REXCVAR_DEFINE_BOOL(skip_opening_movie, false, "Game",
                    "Skip the long opening movie when starting a new game.");

namespace {

struct Movie {
  uint32_t size_field;  // header +4: file size minus 8
  uint32_t frames;      // header +8
  bool opening;         // true: controlled by skip_opening_movie instead of skip_intro
  const char* name;
};

constexpr Movie kMovies[] = {
    {0x007E5628, 311, false, "archives/GSBP_ATVI.bik"},       // Activision logo (Bink 1, BIKi)
    {0x00812B74, 270, false, "archives/GSBP_VVisions.bk2"},   // Vicarious Visions logo (BIKi)
    {0x11F59BF8, 7897, true, "archives/FMV01.bk2"},           // new-game opening movie (KB2g)
};

}  // namespace

void BinkHeaderLoaded(PPCRegister& r1) {
  uint8_t* base = rex::system::kernel_state()->memory()->virtual_membase();
  uint8_t* header = base + r1.u32 + 112;
  // Every movie, skipped or not (ultrawide: bars at the sides while it plays).
  swapforcerecomp::MovieOpened(rex::memory::load_and_swap<uint32_t>(header + 0x14),
                               rex::memory::load_and_swap<uint32_t>(header + 0x18));
  const bool skip_logos = REXCVAR_GET(skip_intro);
  const bool skip_opening = REXCVAR_GET(skip_opening_movie);
  if (!skip_logos && !skip_opening) {
    return;
  }
  // The file stores these fields little-endian, but Bink's reader has already swapped the header
  // into native (big-endian) words by the time it reaches this point.
  const uint32_t size_field = rex::memory::load_and_swap<uint32_t>(header + 4);
  const uint32_t frames = rex::memory::load_and_swap<uint32_t>(header + 8);
  for (const Movie& movie : kMovies) {
    if (size_field != movie.size_field || frames != movie.frames) {
      continue;
    }
    if (movie.opening ? skip_opening : skip_logos) {
      rex::memory::store_and_swap<uint32_t>(header + 8, 1);
      REXLOG_INFO("Skipping movie {} ({} frames)", movie.name, frames);
    }
    return;
  }
}
