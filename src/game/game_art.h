#pragma once

// The game's own art and figure data for the portal menu, read at runtime from the owner's game
// files: element and stat symbols, hats (name, icon, stats) and each character's upgrades (names,
// descriptions, prices, icons, which bit of the figure's upgrade flags each one is). See
// game_art.cpp.

#include <filesystem>

namespace rex::memory {
class Memory;
}  // namespace rex::memory

namespace swapforcerecomp {

// Hands the portal menu the art, upgrade and hat providers. A background thread indexes the hats
// right after start-up; pictures and upgrade lists are read when the menu first asks for them.
void InstallGameArt(rex::memory::Memory* memory, std::filesystem::path archives_dir);

// Stops the background thread; call before the runtime (and its guest memory) goes away.
void ShutdownGameArt();

}  // namespace swapforcerecomp
