#pragma once
// The game's UI pictures (collection icons, upgrade and hat icons, element symbols) as the portal
// menu takes them.

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "alchemy/iga_archive.h"
#include "portal_rex/portal_menu.h"

namespace swapforcerecomp {

// Decodes the texture file at `index` (an igImage2 in an .igz), flipped the right way up: the
// icons were made from .tga files, which store the bottom row first, and the textures keep that
// order. nullptr if it isn't a texture the reader can decode.
std::shared_ptr<const skylanders::FigureImage> DecodeUiIcon(const alchemy::IgaArchive& pak,
                                                           size_t index);

// The textures a material instance file (.igz) uses, each with the source path it was made from
// ("textures:\ui\Hats\Hat_2011_CowboyHat.tga") and its index in `pak`: the file's texture
// dependencies ("textures:/JuiceStandard_Hat_2011_CowboyHat_10.igz" -> the archive entry
// "textures\juicestandard_hat_2011_cowboyhat_10.igz"), each texture's source path being its own
// first string.
struct UiTexture {
  std::string source;  // lower case
  size_t index = 0;
};
std::vector<UiTexture> MaterialTextures(const alchemy::IgaArchive& pak, size_t material_index);

// The archive index of a file by its stored path, compared without case; nullopt if absent.
std::optional<size_t> FindFile(const alchemy::IgaArchive& pak, const std::string& path);

std::string Lower(std::string s);

// The igx files are UTF-8 in places and Latin-1 in others; anything that isn't valid UTF-8 is
// taken as Latin-1.
std::string ToUtf8(const std::string& s);

// "textures:\ui\hats\hat_2011_cowboyhat.tga" -> "hat_2011_cowboyhat".
std::string SourceBaseName(const std::string& source);

}  // namespace swapforcerecomp
