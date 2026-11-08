#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace swapforcerecomp {

// The disc's archives folder: its <name>_<code>.pak files say which languages the disc has. Set it
// before ApplyLanguageSetting and before the settings menu is built.
void SetDiscArchives(const std::filesystem::path& archives);

// The languages the disc has, by the `language` setting's names (english, german, french, spanish,
// italian), in that order. English and German when the folder can't be read (the owner's disc).
const std::vector<std::string>& DiscLanguages();

// Resolves the `language` cvar (a friendly name, or a raw numeric Xbox 360 language ID for
// experimentation) and applies it to the `user_language` cvar that XGetLanguage/
// ExGetXConfigSetting actually read. Call once at startup, before the game can query either.
void ApplyLanguageSetting();

}  // namespace swapforcerecomp
