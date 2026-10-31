#pragma once

// The levels' names as the game shows them, for the Discord status (level_names.cpp).

#include <filesystem>

namespace swapforcerecomp {

// Reads the levels' names from the disc's archives in the background and gives them to the Discord
// status (skylanders-portal portal_rex/discord_presence.h). `archives` is rom/archives.
void InstallLevelNames(const std::filesystem::path& archives);

}  // namespace swapforcerecomp
