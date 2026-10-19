#pragma once

// Pictures and names for the portal menu, taken from the game's own data on the owner's PC: the
// toy collection icons (the ones the game's collection screen shows) for every Skylander, variant
// and magic item, the toy names in English and German, and how long each magic item lasts. See
// toy_images.cpp.

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>

namespace rex::memory {
class Memory;
}  // namespace rex::memory

namespace swapforcerecomp {

// Hands the portal menu an image and a name provider. Nothing is read until the menu first asks;
// then a background thread indexes the archives under `archives_dir` (rom/archives) and decodes
// pictures as they are needed.
void InstallToyImages(rex::memory::Memory* memory, std::filesystem::path archives_dir);

// The character a toy is, as the game's archives name it ("chopchop" for ChopChop.pak,
// "body_statue" for a swapper half), once the index is built; any thread.
std::optional<std::string> ToyCharacterName(uint16_t id);

// Stops the background thread; call before the runtime (and its guest memory) goes away.
void ShutdownToyImages();

}  // namespace swapforcerecomp
