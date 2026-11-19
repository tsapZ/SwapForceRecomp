// The levels' names as the game shows them, for the Discord status. Read 2026-10-31, as
// SuperChargers and Imaginators have them (their level_names.cpp), but in Swap Force's own form:
//
// - zoneinfos.pak has a maps\<area>\<level>\<level>_zoneinfo.mua per level and the language files
//   next to it (<level>_zoneinfo_<language>.mua), IGX text (alchemy/igx_reader.h): an
//   igLocalizedStringList of igLocalizedString, each with _string, _object and _field. The one
//   whose _field is "metafield.CZoneInfo::_displayName" is the level's name ("Mount
//   Cloudbreak" / "Wolkenbruch-Berg" for tutorial_a).
// - German from the _de files, English from the _en files (the European English / German disc);
//   a disc without English text gives its own language instead (EnglishOrDiscText, CoL #105).
// - The level as where_hook.cpp names it: its archive, the same name as the zone info's in any
//   case.

#include "game/level_names.h"

#include <algorithm>
#include <cctype>
#include <memory>
#include <string>
#include <string_view>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

#include <rex/cvar.h>
#include <rex/logging.h>

#include "alchemy/iga_archive.h"
#include "alchemy/igx_reader.h"
#include "portal_rex/discord_presence.h"
#include "portal_rex/menu_draw.h"

// Defined in the SDK (xam_user.cpp): the game's language, which localization.cpp sets from [Game]
// language.
REXCVAR_DECLARE(uint32_t, user_language);

namespace swapforcerecomp {
namespace {

namespace alchemy = skylanders::alchemy;

constexpr char kArchive[] = "zoneinfos.pak";
constexpr std::string_view kZoneInfo = "_zoneinfo";
constexpr std::string_view kExtension = ".mua";
constexpr char kDisplayNameField[] = "metafield.CZoneInfo::_displayName";

using Table = std::unordered_map<std::string, std::string>;

std::string Lower(std::string s) {
  for (char& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
  return s;
}

// The names for the English status: the _en files. A disc without them (CoL #105: the French /
// Dutch Imaginators disc has only fr and nl texts) gives its own text instead: the language the
// game runs in first (user_language 4 French, 5 Spanish, 6 Italian), then Dutch, French, Italian,
// Spanish, whichever the disc has. Picked at each lookup, as the language is set at start-up,
// maybe after these files were read.
const Table& EnglishOrDiscText(const std::unordered_map<std::string, Table>& by_language) {
  static const Table kNone;
  const auto has = [&](const char* code) -> const Table* {
    const auto it = by_language.find(code);
    return it != by_language.end() && !it->second.empty() ? &it->second : nullptr;
  };
  if (const Table* table = has("en")) return *table;
  const char* own = nullptr;
  switch (REXCVAR_GET(user_language)) {
    case 4: own = "fr"; break;
    case 5: own = "es"; break;
    case 6: own = "it"; break;
    default: break;
  }
  if (own != nullptr) {
    if (const Table* table = has(own)) return *table;
  }
  for (const char* code : {"nl", "fr", "it", "es"}) {
    if (const Table* table = has(code)) return *table;
  }
  return kNone;
}

// The zone's name from its language file, empty if it has none.
std::string DisplayName(std::string_view text) {
  for (const alchemy::IgxObject& object : alchemy::ParseIgx(text)) {
    if (object.type == "igLocalizedString" && object.Var("_field") == kDisplayNameField) {
      return object.Var("_string");
    }
  }
  return {};
}

}  // namespace

void InstallLevelNames(const std::filesystem::path& archives) {
  std::thread([archives] {
    const auto archive = alchemy::IgaArchive::Open(archives / kArchive);
    if (!archive) {
      REXLOG_WARN("Level names: no {}; the Discord status shows no level", kArchive);
      return;
    }
    // Every zone info's name by language ("de", "en"), the level in lower case.
    std::unordered_map<std::string, Table> by_language;
    for (size_t i = 0; i < archive->size(); ++i) {
      // "maps\barrens\brn2\brn2_zoneinfo_en.mua"
      const std::string name = Lower(archive->name(i));
      const size_t suffix = kZoneInfo.size() + 3 + kExtension.size();  // "_zoneinfo_en.mua"
      if (name.size() < suffix || name.compare(name.size() - kExtension.size(), kExtension.size(), kExtension) != 0) {
        continue;
      }
      const size_t base = name.find_last_of("/\\") + 1;
      const size_t zone = name.rfind(kZoneInfo);
      if (zone == std::string::npos || zone < base || zone + suffix != name.size() || name[zone + kZoneInfo.size()] != '_') {
        continue;
      }
      const std::string language = name.substr(zone + kZoneInfo.size() + 1, 2);
      const auto data = archive->Read(i);
      if (!data) continue;
      std::string shown = DisplayName(std::string_view(reinterpret_cast<const char*>(data->data()), data->size()));
      if (!shown.empty()) by_language[language][name.substr(base, zone - base)] = std::move(shown);
    }
    auto german = std::make_shared<Table>(by_language["de"]);
    auto english = std::make_shared<Table>(by_language["en"]);
    std::vector<std::string> languages;
    for (const auto& [code, table] : by_language) {
      if (!table.empty()) languages.push_back(code);  // operator[] above adds empty en / es / de
    }
    std::sort(languages.begin(), languages.end());
    std::string disc;
    for (const std::string& code : languages) disc += (disc.empty() ? "" : " ") + code;
    auto tables = std::make_shared<const std::unordered_map<std::string, Table>>(std::move(by_language));
    // Which levels have a name, so a log shows the ones the status leaves out.
    std::vector<std::string> known;
    for (const auto& [level, shown] : german->empty() ? *english : *german) known.push_back(level);
    std::sort(known.begin(), known.end());
    std::string list;
    for (const std::string& level : known) list += (list.empty() ? "" : " ") + level;
    REXLOG_INFO("Level names: {} German, {} English ({}); the disc has: {}", german->size(), english->size(),
                list, disc);
    skylanders::SetDiscordLevelNames([german, tables](const std::string& level) {
      const bool in_german = skylanders::menu_draw::GermanText();
      std::string key = Lower(level);
      // Woodburrow, the hub (town_state1-13 as the story goes on): its zone infos have no name;
      // the game calls it so in "Woodburrow Landing" / "Holzhausens Anlegeplatz" (Tutorial_B's
      // Areas_Area7).
      if (key.starts_with("town_state") || key == "woodburrow") {
        return std::string(in_german ? "Holzhausen" : "Woodburrow");
      }
      if (key == "cloudbreak") key = "tutorial_a";  // Mount Cloudbreak, the first level
      if (in_german) {
        if (const auto it = german->find(key); it != german->end()) return it->second;
      }
      const Table& table = EnglishOrDiscText(*tables);
      const auto it = table.find(key);
      return it == table.end() ? std::string() : it->second;
    });
  }).detach();
}

}  // namespace swapforcerecomp
