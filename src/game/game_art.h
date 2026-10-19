#pragma once

// The game's own art and figure data for the portal menu, read at runtime from the owner's game
// files: element and stat symbols, hats (name, icon, stats) and each character's upgrades (names,
// descriptions, prices, icons, which bit of the figure's upgrade flags each one is). See
// game_art.cpp.

#include <filesystem>
#include <memory>
#include <string>

namespace rex::memory {
class Memory;
}  // namespace rex::memory

namespace skylanders {
struct FigureImage;
}  // namespace skylanders

namespace swapforcerecomp {

// Hands the portal menu the art, upgrade and hat providers. A background thread indexes the hats
// right after start-up; pictures and upgrade lists are read when the menu first asks for them.
void InstallGameArt(rex::memory::Memory* memory, std::filesystem::path archives_dir);

// The icon of the hat a collectible tracker stands for ("Hat_UFOHat", as an item's data names
// it), or nullptr while loading or for no such hat. UI thread, never blocks.
std::shared_ptr<const skylanders::FigureImage> GameArtHatIcon(const std::string& tracker);

// Stops the background thread; call before the runtime (and its guest memory) goes away.
void ShutdownGameArt();

}  // namespace swapforcerecomp
