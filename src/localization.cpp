#include "localization.h"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include <rex/cvar.h>
#include <rex/logging.h>

#ifdef _WIN32
#include <windows.h>
#endif

// Defined in the SDK (xam_user.cpp); both XGetLanguage and ExGetXConfigSetting read this.
REXCVAR_DECLARE(uint32_t, user_language);
// Defined in the SDK (xam_locale.cpp): the console's country (XC_COUNTRY ids), which XGetLocale reads.
REXCVAR_DECLARE(uint32_t, user_country);

REXCVAR_DEFINE_STRING(language, "auto", "Game",
                      "Game text/voice language: auto (the language of Windows if the disc has "
                      "it, else English), or english, german, french, spanish, italian, those the "
                      "disc has. A raw Xbox 360 language ID (a number) also works, for experiments.");

namespace swapforcerecomp {

namespace {

std::string Lower(std::string s) {
  std::transform(s.begin(), s.end(), s.begin(),
                 [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return s;
}

std::filesystem::path g_archives;

bool DiscHas(const std::string& name) {
  const auto& languages = DiscLanguages();
  return std::find(languages.begin(), languages.end(), name) != languages.end();
}

// The Windows display language if the disc has it, else English, else the disc's first.
std::string SystemLanguage() {
#ifdef _WIN32
  const char* windows = nullptr;
  switch (PRIMARYLANGID(GetUserDefaultUILanguage())) {
    case LANG_GERMAN: windows = "german"; break;
    case LANG_FRENCH: windows = "french"; break;
    case LANG_SPANISH: windows = "spanish"; break;
    case LANG_ITALIAN: windows = "italian"; break;
    case LANG_DUTCH: windows = "dutch"; break;
    default: break;
  }
  if (windows && DiscHas(windows)) return windows;
#endif
  if (DiscHas("english")) return "english";
  return DiscLanguages().front();
}

}  // namespace

void SetDiscArchives(const std::filesystem::path& archives) { g_archives = archives; }

const std::vector<std::string>& DiscLanguages() {
  static const std::vector<std::string> languages = [] {
    static const std::pair<const char*, const char*> kCodes[] = {
        {"en", "english"}, {"de", "german"}, {"fr", "french"}, {"es", "spanish"}, {"it", "italian"},
        {"nl", "dutch"}};
    std::set<std::string> found;
    std::error_code ec;
    for (const auto& entry : std::filesystem::directory_iterator(g_archives, ec)) {
      // <name>_<code>.pak, e.g. boss_kaos_bank_lv_de.pak.
      const std::string name = Lower(entry.path().filename().string());
      if (name.size() < 8 || name.compare(name.size() - 4, 4, ".pak") != 0 ||
          name[name.size() - 7] != '_') {
        continue;
      }
      const std::string code = name.substr(name.size() - 6, 2);
      for (const auto& [c, language] : kCodes) {
        if (code == c) found.insert(language);
      }
    }
    std::vector<std::string> list;
    for (const auto& [c, language] : kCodes) {
      if (found.count(language)) list.push_back(language);
    }
    if (list.empty()) list = {"english", "german"};
    REXLOG_INFO("Language: the disc has {} language(s)", list.size());
    return list;
  }();
  return languages;
}

void ApplyLanguageSetting() {
  // A direct --user_language / user_language=<n> override (raw, for experimentation) always
  // wins: don't clobber it with whatever `language` defaults to (a real bug this had until now --
  // it unconditionally overwrote user_language, silently discarding a direct override).
  if (rex::cvar::HasNonDefaultValue("user_language")) {
    REXLOG_INFO("Language: user_language already set directly ({}); leaving it alone",
               REXCVAR_GET(user_language));
    return;
  }

  // Xbox 360 XLanguage IDs (include/rex/system/xcontent.h); only these five are confirmed to
  // match a real console. Dutch/Swedish/Norwegian/Danish/Finnish aren't in that enum at all, even
  // though this game's disc supports them on real hardware -- use a raw numeric id to try one.
  static const std::unordered_map<std::string, uint32_t> kNames = {
      {"english", 1}, {"german", 3}, {"french", 4}, {"spanish", 5}, {"italian", 6},
  };

  std::string setting = Lower(REXCVAR_GET(language));
  if ((kNames.count(setting) || setting == "dutch") && !DiscHas(setting)) {
    // A settings file from another disc: its language isn't on this one.
    REXLOG_WARN("Language: the disc has no {}; using auto", setting);
    setting = "auto";
  }
  if (setting == "auto") {
    setting = SystemLanguage();
    REXLOG_INFO("Language: auto, from the Windows display language");
  }
  // Dutch isn't an Xbox 360 language id: the Skylanders games take Dutch from the console's country
  // (language English, country the Netherlands), as Spyro's Adventure does (CoL #68, checked there
  // 2026-11-09). A country set directly (--user_country) stays.
  if (setting == "dutch") {
    REXCVAR_SET(user_language, 1u);
    if (!rex::cvar::HasNonDefaultValue("user_country")) {
      REXCVAR_SET(user_country, 74u);  // XC_COUNTRY_NL
    }
    REXLOG_INFO("Language: dutch (English with the country {})", REXCVAR_GET(user_country));
    return;
  }
  if (auto it = kNames.find(setting); it != kNames.end()) {
    REXCVAR_SET(user_language, it->second);
    REXLOG_INFO("Language: {} (id {})", setting, it->second);
    return;
  }

  uint32_t numeric_id = 0;
  auto [ptr, ec] = std::from_chars(setting.data(), setting.data() + setting.size(), numeric_id);
  if (ec == std::errc() && ptr == setting.data() + setting.size()) {
    REXCVAR_SET(user_language, numeric_id);
    REXLOG_INFO("Language: raw id {}", numeric_id);
    return;
  }

  REXLOG_WARN("Language: '{}' is not a known name or a number; using English", setting);
  REXCVAR_SET(user_language, 1u);
}

}  // namespace swapforcerecomp
