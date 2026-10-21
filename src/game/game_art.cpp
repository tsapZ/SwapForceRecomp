// The game's own art and figure data for the portal menu, all from the owner's game files
// (rom/archives); nothing of it is in the repository. Found 2026-10-19:
//
// Upgrades: every character has <name>.pak (ChopChop.pak, Body_Statue.pak for a swapper half) with
//   characters\...\<name>_characterdata.igx. Its CSkillsUpgradeComponentData lists the starting
//   attacks (_initialAbilities) and the upgrade paths (_upgrades: basic, the two paths, soul gem,
//   Wow Pow), each CSkillUpgrade with _cost and _previewImage ("ChopChop_AbilityUpgrades.
//   ChopChop_Upgrade1"); _en/_de next to it hold the names and descriptions. Which bit of the
//   figure's upgrade flags an upgrade is: the game's EToyAbilityFlagBits (PathChosen 0, PathChoice
//   1, FirstUpgrade 2 ... LastUpgrade 10, AlternateUpgradePath 11-13) and, read from the loaded
//   CSkillUpgrade objects (_toyAbilityFlagBit at +0x34), counted on from 2: the basic upgrades,
//   then the two paths sharing the same bits (bit 1 says which path; Chop Chop 2-5 and 6-8, a
//   swapper half with 3 basic and 2 per path 2-4 and 5-6), then the soul gem (Chop Chop 9); the
//   starting attacks have none (-2). The Wow Pow comes last (10, LastUpgrade), one per figure year
//   (_yearCode) in the list, so a figure gets its own year's. The path not chosen keeps its
//   upgrades in the alternate bits from 11.
// Upgrade icons: <name>_abilityupgrades.pak, a material instance file with one material per
//   upgrade (ChopChop_Upgrade1) and their textures (made from ...\ChopChop_Up1.tga).
// Hats: permanent.pak data\collectibletrackers.igx, a CHatCollectibleTracker per hat with _hatId
//   (the enum name), _iconJuiceMaterial (collectibleicons.pak) and _attributes (CCharacterAttribute:
//   eCA_CriticalHitChance, 2.0); names in _en/_de (::_hatName). The hat enum's values come from the
//   executable: 155 names at 0x820FF2F0, values at 0x820FF560.
// Stat names: loosefiles.pak data\systemdata.mua, SystemData_characterAttributeNames (eCA_* ->
//   English text); _de.mua translates by FNV-1a of the English text (igLocalizedHashTable). Stat
//   symbols: permanent.pak materialinstances\characterattributeicons.igz.
// Element symbols: permanent.pak materialinstances\elementicons.igz, made from
//   textures:\ui\ElementalIcons\DN_<Element>.tga.
// Controller picture: controller_schemexenon.pak (the game's Controls screen on the Xbox 360),
//   materialinstances\juice_controller_schemexenon_materials.igz, made from
//   textures:\ui\Controllers\Xbox_Controller.tga (1280x720). The callout lines to the screen's
//   labels and a second set of face buttons for them are drawn into it; CleanController takes
//   them out for the settings menu.
// Gold symbol ("ui:gold"): the HUD's coin, juicedomain_permanent.pak
//   textures\juicestandard_coin_icon_10.igz (next to juice_player1hud_materials.igz; found by
//   the archive's file names, 2026-10-21).

#include "game_art.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <condition_variable>
#include <cstdlib>
#include <deque>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <rex/cvar.h>
#include <rex/logging.h>

#include "alchemy/iga_archive.h"
#include "alchemy/igx_reader.h"
#include "guest_memory.h"
#include "portal_rex/portal_menu.h"
#include "portal_rex/settings_menu.h"
#include "toy_images.h"
#include "ui_icon.h"

namespace swapforcerecomp {
namespace {

using alchemy::IgaArchive;
using alchemy::IgxObject;
using skylanders::FigureImage;
using skylanders::FigureUpgrade;
using skylanders::FigureUpgradePath;
using skylanders::FigureUpgrades;
using skylanders::HatInfo;

constexpr uint32_t kHatNames = 0x820FF2F0;
constexpr uint32_t kHatValues = 0x820FF560;
constexpr uint32_t kHatCount = 155;
constexpr char kHatPrefix[] = "kTfbSpyroTag_Hat_";

// Upgrade flag bits (see the top of the file).
constexpr int kFirstUpgradeBit = 2;
constexpr int kFirstAlternateBit = 11;

// The stat symbol materials (SystemData_characterAttributeIcons) and the texture each is made from.
constexpr std::pair<const char*, const char*> kStatIcons[] = {
    {"criticalhiticon", "stats_icon_critical"}, {"elementalboosticon", "stats_icon_elemental"},
    {"healthicon", "stats_icon_health"},         {"luckicon", "stats_icon_luck"},
    {"resistanceicon", "stats_icon_armor"},      {"speedicon", "stats_icon_speed"},
    {"strengthicon", "stats_icon_strength"},
};

bool German() { return Lower(rex::cvar::GetFlagByName("language")) == "german"; }

// Xbox_Controller.tga (see the top): where the controller is in it, and the callout lines over it.
// The lines are white, 3 to 4 pixels wide, with a soft shadow to the right of and below them.
constexpr int kControllerWidth = 1280, kControllerHeight = 720;
constexpr int kControllerX = 324, kControllerY = 213, kControllerW = 503, kControllerH = 323;
struct CalloutLine {
  int at, from, to;  // vertical: x and its rows; horizontal: y and its columns
  int mirror = 0, mirror_from = 0, mirror_to = 0;  // vertical: the symmetry axis to copy across
};
constexpr CalloutLine kVerticalLines[] = {
    {512, 392, 600, 497, 395, 540},  // from Back down through the d-pad
    {638, 392, 600, 654, 395, 545},  // from Start down through the right stick
    {440, 195, 245},                 // the triggers
    {708, 195, 245},
    {375, 220, 280},                 // the bumpers
    {776, 212, 270},
};
constexpr CalloutLine kHorizontalLines[] = {
    {387, 300, 364},  // the left stick
    {335, 782, 860},  // Y and A, to the label column
    {429, 782, 860},
};

// Paints the callout lines over (across each line and its shadow, from the pixels on both sides;
// over the d-pad and the right stick, which are symmetric, from their other half), drops all that
// isn't joined to the controller (the label column's face buttons, the lines outside it: erode the
// opaque pixels, flood fill from the middle, dilate back) and cuts the controller out. Checked by
// eye against the game's picture.
std::shared_ptr<FigureImage> CleanController(const FigureImage& in) {
  if (in.width != kControllerWidth || in.height != kControllerHeight ||
      in.rgba.size() < size_t(in.width) * in.height * 4) {
    return nullptr;
  }
  const int w = in.width, h = in.height;
  std::vector<uint8_t> px = in.rgba;
  auto at = [&](int x, int y) { return &px[(size_t(y) * w + x) * 4]; };
  auto blend = [](const uint8_t* a, const uint8_t* b, float t, uint8_t* out) {
    for (int c = 0; c < 4; ++c) {
      out[c] = uint8_t(float(a[c]) + (float(b[c]) - float(a[c])) * t + 0.5f);
    }
  };
  for (const CalloutLine& line : kVerticalLines) {
    const int x0 = line.at - 6, x1 = line.at + 12;
    for (int y = line.from; y <= line.to; ++y) {
      if (line.mirror && y >= line.mirror_from && y <= line.mirror_to) {
        for (int x = x0 + 1; x < x1; ++x) std::copy_n(at(2 * line.mirror - x, y), 4, at(x, y));
        continue;
      }
      uint8_t a[4], b[4];
      std::copy_n(at(x0, y), 4, a);
      std::copy_n(at(x1, y), 4, b);
      for (int x = x0 + 1; x < x1; ++x) blend(a, b, float(x - x0) / float(x1 - x0), at(x, y));
    }
  }
  for (const CalloutLine& line : kHorizontalLines) {
    const int y0 = line.at - 4, y1 = line.at + 11;
    for (int x = line.from; x <= line.to; ++x) {
      uint8_t a[4], b[4];
      std::copy_n(at(x, y0), 4, a);
      std::copy_n(at(x, y1), 4, b);
      for (int y = y0 + 1; y < y1; ++y) blend(a, b, float(y - y0) / float(y1 - y0), at(x, y));
    }
  }
  // The opaque pixels, eroded by 4 (the lines are thinner), the part joined to the middle, dilated
  // by 5.
  std::vector<uint8_t> mask(size_t(w) * h), tmp(size_t(w) * h);
  for (size_t i = 0; i < mask.size(); ++i) mask[i] = px[i * 4 + 3] > 100;
  auto box = [&](int r, bool grow) {
    for (int y = 0; y < h; ++y) {
      for (int x = 0; x < w; ++x) {
        bool v = !grow;
        for (int k = std::max(0, x - r); k <= std::min(w - 1, x + r); ++k) {
          v = grow ? (v || mask[size_t(y) * w + k]) : (v && mask[size_t(y) * w + k]);
        }
        tmp[size_t(y) * w + x] = v;
      }
    }
    for (int y = 0; y < h; ++y) {
      for (int x = 0; x < w; ++x) {
        bool v = !grow;
        for (int k = std::max(0, y - r); k <= std::min(h - 1, y + r); ++k) {
          v = grow ? (v || tmp[size_t(k) * w + x]) : (v && tmp[size_t(k) * w + x]);
        }
        mask[size_t(y) * w + x] = v;
      }
    }
  };
  box(4, false);
  std::vector<uint8_t> body(size_t(w) * h);
  std::vector<std::pair<int, int>> stack = {{575, 420}};
  while (!stack.empty()) {
    const auto [x, y] = stack.back();
    stack.pop_back();
    if (x < 0 || y < 0 || x >= w || y >= h) continue;
    const size_t i = size_t(y) * w + x;
    if (body[i] || !mask[i]) continue;
    body[i] = 1;
    stack.insert(stack.end(), {{x + 1, y}, {x - 1, y}, {x, y + 1}, {x, y - 1}});
  }
  mask = std::move(body);
  box(5, true);
  auto out = std::make_shared<FigureImage>();
  out->width = kControllerW;
  out->height = kControllerH;
  out->rgba.resize(size_t(kControllerW) * kControllerH * 4);
  for (int y = 0; y < kControllerH; ++y) {
    for (int x = 0; x < kControllerW; ++x) {
      const int sx = kControllerX + x, sy = kControllerY + y;
      uint8_t* o = &out->rgba[(size_t(y) * kControllerW + x) * 4];
      std::copy_n(at(sx, sy), 4, o);
      if (!mask[size_t(sy) * w + sx]) o[3] = 0;
    }
  }
  return out;
}

uint32_t Fnv1a(const std::string& s) {
  uint32_t h = 0x811C9DC5;
  for (unsigned char c : s) {
    h ^= c;
    h *= 0x01000193;
  }
  return h;
}

bool EndsWith(const std::string& s, const std::string& suffix) {
  return s.size() >= suffix.size() && s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0;
}

std::vector<IgxObject> ReadIgx(const IgaArchive& pak, size_t index) {
  auto data = pak.Read(index);
  if (!data) return {};
  return alchemy::ParseIgx(
      std::string_view(reinterpret_cast<const char*>(data->data()), data->size()));
}

// The game's button glyphs in its texts (Chop Chop: "Drücke ~" is his first attack, X).
std::string ButtonNames(std::string s) {
  std::string out;
  for (char c : s) {
    switch (c) {
      case '~': out += "X"; break;
      case '^': out += "Y"; break;
      case '`': out += "B"; break;
      default: out.push_back(c);
    }
  }
  return out;
}

// "[Beschreibung fehlt]" and the like: a placeholder, not a text to show.
std::string Text(const std::string& s) {
  if (s.empty() || s.front() == '[') return {};
  return ButtonNames(ToUtf8(s));
}

struct Hat {
  std::string name_en, name_de;
  std::string icon_material;  // "haticon_cowboy_materials"
  std::vector<std::pair<std::string, float>> stats;  // eCA_* and value
};

class GameArt {
 public:
  GameArt(rex::memory::Memory* memory, std::filesystem::path archives)
      : memory_(memory), archives_(std::move(archives)) {}
  ~GameArt() { Stop(); }

  void Start() {
    std::lock_guard lock(mutex_);
    if (started_) return;
    started_ = true;
    thread_ = std::thread([this] { Run(); });
  }

  void Stop() {
    {
      std::lock_guard lock(mutex_);
      stop_ = true;
    }
    cv_.notify_all();
    if (thread_.joinable()) thread_.join();
  }

  // UI thread: never blocks; nullptr until the worker has it.
  std::shared_ptr<const FigureImage> Image(const std::string& key) {
    std::lock_guard lock(mutex_);
    if (auto it = images_.find(key); it != images_.end()) return it->second;
    Queue("img:" + key);
    return nullptr;
  }

  // The icon of the hat behind a collectible tracker ("Hat_UFOHat").
  std::shared_ptr<const FigureImage> HatIcon(const std::string& tracker) {
    uint16_t hat = 0;
    {
      std::lock_guard lock(mutex_);
      if (!indexed_) return nullptr;
      auto it = hat_trackers_.find(Lower(tracker));
      if (it == hat_trackers_.end()) return nullptr;
      hat = it->second;
    }
    return Image("hat:" + std::to_string(hat));
  }

  std::shared_ptr<const FigureUpgrades> Upgrades(uint16_t id, uint16_t variant) {
    auto character = ToyCharacterName(id);
    if (!character) return nullptr;
    const std::string key = Lower(*character) + "|" + std::to_string((variant >> 12) & 0xF) +
                            (German() ? "|de" : "|en");
    std::lock_guard lock(mutex_);
    if (auto it = upgrades_.find(key); it != upgrades_.end()) return it->second;
    Queue("upg:" + key);
    return nullptr;
  }

  std::shared_ptr<const HatInfo> HatInfoFor(uint16_t hat) {
    const bool german = German();
    std::lock_guard lock(mutex_);
    if (!indexed_) return nullptr;
    const uint32_t key = (uint32_t(hat) << 1) | (german ? 1 : 0);
    if (auto it = hat_infos_.find(key); it != hat_infos_.end()) return it->second;
    auto h = hats_.find(hat);
    if (h == hats_.end()) return nullptr;
    auto info = std::make_shared<HatInfo>();
    info->name = german && !h->second.name_de.empty() ? h->second.name_de : h->second.name_en;
    info->image = "hat:" + std::to_string(hat);
    for (const auto& [attribute, value] : h->second.stats) {
      skylanders::HatStat stat;
      stat.value = value;
      auto english = stat_names_en_.find(attribute);
      stat.name = english != stat_names_en_.end() ? english->second : attribute;
      if (german && english != stat_names_en_.end()) {
        if (auto de = stat_names_de_.find(Fnv1a(english->second)); de != stat_names_de_.end()) {
          stat.name = de->second;
        }
      }
      if (stat_icons_.contains(attribute)) stat.image = "stat:" + attribute;
      info->stats.push_back(std::move(stat));
    }
    hat_infos_[key] = info;
    return info;
  }

 private:
  // mutex_ held.
  void Queue(const std::string& job) {
    if (failed_ || !pending_.insert(job).second) return;
    queue_.push_back(job);
    cv_.notify_one();
  }

  void Run() {
    if (!WaitForHatTable()) return;
    const auto start = std::chrono::steady_clock::now();
    Index();
    {
      std::lock_guard lock(mutex_);
      indexed_ = true;
    }
    REXLOG_INFO("Game art: {} hats, {} stat names indexed in {} ms", hats_.size(),
                stat_names_en_.size(),
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now() - start)
                    .count());
    while (true) {
      std::string job;
      {
        std::unique_lock lock(mutex_);
        cv_.wait(lock, [this] { return stop_ || !queue_.empty(); });
        if (stop_) return;
        job = queue_.front();
        queue_.pop_front();
      }
      if (job.rfind("img:", 0) == 0) {
        auto image = LoadImage(job.substr(4));
        std::lock_guard lock(mutex_);
        images_[job.substr(4)] = std::move(image);
        pending_.erase(job);
      } else if (job.rfind("upg:", 0) == 0) {
        auto upgrades = LoadUpgrades(job.substr(4));
        std::lock_guard lock(mutex_);
        upgrades_[job.substr(4)] = std::move(upgrades);
        pending_.erase(job);
      }
    }
  }

  bool WaitForHatTable() {
    for (int i = 0; i < 240; ++i) {
      const auto name_ptr = ReadGuest32(memory_, kHatNames);
      const auto name = name_ptr ? ReadGuestString(memory_, *name_ptr) : std::nullopt;
      if (name && name->rfind(kHatPrefix, 0) == 0) return true;
      std::unique_lock lock(mutex_);
      if (cv_.wait_for(lock, std::chrono::milliseconds(500), [this] { return stop_; })) {
        return false;
      }
    }
    std::lock_guard lock(mutex_);
    failed_ = true;
    return false;
  }

  // Worker, before indexed_ is set: hats and stat names.
  void Index() {
    std::unordered_map<std::string, uint16_t> values;  // "kTfbSpyroTag_Hat_Cowboy" -> 22
    for (uint32_t i = 0; i < kHatCount; ++i) {
      const auto name_ptr = ReadGuest32(memory_, kHatNames + 4 * i);
      const auto value = ReadGuest32(memory_, kHatValues + 4 * i);
      const auto name = name_ptr ? ReadGuestString(memory_, *name_ptr) : std::nullopt;
      if (!value || !name || name->rfind(kHatPrefix, 0) != 0) {
        REXLOG_WARN("Game art: the hat table isn't where it was expected; no hats");
        values.clear();
        break;
      }
      values.emplace(*name, uint16_t(*value));
    }
    if (auto permanent = IgaArchive::Open(archives_ / "permanent.pak")) {
      std::unordered_map<std::string, uint16_t> trackers;  // "hat_cowboyhat" -> 22
      std::unordered_map<std::string, const IgxObject*> objects;
      auto base = FindFile(*permanent, "data\\collectibletrackers.igx");
      std::vector<IgxObject> parsed = base ? ReadIgx(*permanent, *base) : std::vector<IgxObject>{};
      for (const auto& object : parsed) objects[object.refname] = &object;
      for (const auto& object : parsed) {
        if (object.type != "CHatCollectibleTracker") continue;
        auto v = values.find(object.Var("_hatId"));
        if (v == values.end()) continue;
        trackers[Lower(object.refname)] = v->second;
        hat_trackers_[Lower(object.refname)] = v->second;
        Hat& hat = hats_[v->second];
        const std::string& material = object.Var("_iconJuiceMaterial");
        hat.icon_material = Lower(material.substr(0, material.find('.')));
        if (auto list = objects.find(object.Var("_attributes").substr(
                object.Var("_attributes").rfind("this.", 0) == 0 ? 5 : 0));
            list != objects.end()) {
          for (const std::string& item : list->second->items) {
            auto a = objects.find(item);
            if (a == objects.end()) continue;
            // The file leaves out an attribute that has the field's default value: 34 hats'
            // bonuses have none ("The Outsider": one without, one eCA_BaseElementalBoost), and no
            // hat names eCA_MaximumHealth, the one hat stat that never shows up. So none is
            // health (the owner saw Rattle Shake's hat with a nameless "+8", 2026-10-21).
            std::string attribute = a->second->Var("_attribute");
            if (attribute.empty()) attribute = "eCA_MaximumHealth";
            hat.stats.emplace_back(std::move(attribute),
                                   std::strtof(a->second->Var("_value").c_str(), nullptr));
          }
        }
      }
      for (const char* language : {"en", "de"}) {
        auto file = FindFile(*permanent, std::string("data\\collectibletrackers_") + language + ".igx");
        if (!file) continue;
        for (const auto& object : ReadIgx(*permanent, *file)) {
          if (object.type != "igLocalizedString" ||
              object.Var("_field").find("::_hatName") == std::string::npos) {
            continue;
          }
          std::string ref = Lower(object.Var("_object"));
          ref.erase(0, ref.find('.') + 1);
          if (auto t = trackers.find(ref); t != trackers.end()) {
            (language[0] == 'd' ? hats_[t->second].name_de : hats_[t->second].name_en) =
                ToUtf8(object.Var("_string"));
          }
        }
      }
    }
    if (auto loose = IgaArchive::Open(archives_ / "loosefiles.pak")) {
      if (auto base = FindFile(*loose, "data\\systemdata.mua")) {
        for (const auto& object : ReadIgx(*loose, *base)) {
          if (object.refname == "SystemData_characterAttributeNames") {
            for (const auto& [key, value] : object.dict) stat_names_en_[key] = ToUtf8(value);
          } else if (object.refname == "SystemData_characterAttributeIcons") {
            for (const auto& [key, value] : object.dict) {
              stat_icons_[key] = Lower(value.substr(value.find('.') + 1));
            }
          }
        }
      }
      if (auto de = FindFile(*loose, "data\\systemdata_de.mua")) {
        for (const auto& object : ReadIgx(*loose, *de)) {
          if (object.type != "igLocalizedHashTable") continue;
          for (const auto& [key, value] : object.dict) {
            stat_names_de_[uint32_t(std::strtoul(key.c_str(), nullptr, 10))] = ToUtf8(value);
          }
        }
      }
    }
  }

  std::shared_ptr<const FigureImage> LoadImage(const std::string& key) {
    const size_t colon = key.find(':');
    if (colon == std::string::npos) return nullptr;
    const std::string kind = key.substr(0, colon), what = key.substr(colon + 1);
    if (kind == "element") {
      return FromMaterial("permanent", "materialinstances\\elementicons.igz", "dn_" + what);
    }
    if (kind == "ui" && what == "gold") {
      // The HUD's coin next to the gold amount: a texture file of its own in the HUD's archive.
      const IgaArchive* pak = Pak("juicedomain_permanent");
      if (!pak) return nullptr;
      auto index = FindFile(*pak, "textures\\juicestandard_coin_icon_10.igz");
      return index ? DecodeUiIcon(*pak, *index) : nullptr;
    }
    if (kind == "controller") {
      auto picture = FromMaterial("controller_schemexenon",
                                  "materialinstances\\juice_controller_schemexenon_materials.igz",
                                  "xbox_controller");
      return picture ? CleanController(*picture) : nullptr;
    }
    if (kind == "stat") {
      auto icon = stat_icons_.find(what);
      if (icon == stat_icons_.end()) return nullptr;
      for (const auto& [material, texture] : kStatIcons) {
        if (icon->second == material) {
          return FromMaterial("permanent", "materialinstances\\characterattributeicons.igz",
                              texture);
        }
      }
      return nullptr;
    }
    if (kind == "hat") {
      auto hat = hats_.find(uint16_t(std::strtoul(what.c_str(), nullptr, 10)));
      if (hat == hats_.end() || hat->second.icon_material.empty()) return nullptr;
      return FromMaterial("collectibleicons",
                          "materialinstances\\" + hat->second.icon_material + ".igz", {});
    }
    if (kind == "upgrade") {
      // "ChopChop_AbilityUpgrades.ChopChop_Upgrade1": the archive and file, then the material,
      // whose texture is ChopChop_Up1.
      const size_t dot = what.find('.');
      if (dot == std::string::npos) return nullptr;
      const std::string file = Lower(what.substr(0, dot));
      std::string texture = Lower(what.substr(dot + 1));
      if (size_t at = texture.find("_upgrade"); at != std::string::npos) {
        texture.replace(at, 8, "_up");
      }
      return FromMaterial(file, "materialinstances\\abilityupgrades\\" + file + ".igz", texture);
    }
    return nullptr;
  }

  // The picture of a material instance file: the texture made from `<texture>.tga`, or with an
  // empty name the file's first one.
  std::shared_ptr<const FigureImage> FromMaterial(const std::string& pak_name,
                                                  const std::string& material_file,
                                                  const std::string& texture) {
    const IgaArchive* pak = Pak(pak_name);
    if (!pak) return nullptr;
    const std::string cache_key = pak_name + "|" + Lower(material_file);
    auto cached = material_textures_.find(cache_key);
    if (cached == material_textures_.end()) {
      auto index = FindFile(*pak, material_file);
      cached = material_textures_
                   .emplace(cache_key, index ? MaterialTextures(*pak, *index)
                                             : std::vector<UiTexture>{})
                   .first;
    }
    for (const UiTexture& t : cached->second) {
      if (texture.empty() || SourceBaseName(t.source) == texture) return DecodeUiIcon(*pak, t.index);
    }
    return nullptr;
  }

  // Worker only: archives stay open once used (a few MB each).
  const IgaArchive* Pak(const std::string& name) {
    auto it = paks_.find(name);
    if (it == paks_.end()) {
      auto pak = IgaArchive::Open(archives_ / (name + ".pak"));
      it = paks_.emplace(name, pak ? std::make_unique<IgaArchive>(std::move(*pak)) : nullptr).first;
    }
    return it->second.get();
  }

  // "chopchop|2|de": the character's upgrades for a figure of that year, in that language.
  std::shared_ptr<const FigureUpgrades> LoadUpgrades(const std::string& key) {
    const size_t a = key.find('|'), b = key.rfind('|');
    const std::string character = key.substr(0, a);
    const std::string year = key.substr(a + 1, b - a - 1);
    const bool german = key.substr(b + 1) == "de";
    auto pak = IgaArchive::Open(archives_ / (character + ".pak"));
    if (!pak) return nullptr;
    std::optional<size_t> base, language;
    for (size_t i = 0; i < pak->size(); ++i) {
      const std::string name = Lower(pak->name(i));
      if (EndsWith(name, "\\" + character + "_characterdata.igx")) base = i;
      if (EndsWith(name, "\\" + character + "_characterdata_" + (german ? "de" : "en") + ".igx")) {
        language = i;
      }
    }
    if (!base) return nullptr;
    const std::vector<IgxObject> parsed = ReadIgx(*pak, *base);
    std::unordered_map<std::string, const IgxObject*> objects;
    for (const auto& object : parsed) objects[object.refname] = &object;
    std::unordered_map<std::string, std::string> texts;  // "<object>|<field>" -> text
    if (language) {
      for (const auto& object : ReadIgx(*pak, *language)) {
        if (object.type != "igLocalizedString") continue;
        std::string ref = object.Var("_object");
        ref.erase(0, ref.find('.') + 1);
        std::string field = object.Var("_field");
        field.erase(0, field.rfind(':') + 1);
        texts[ref + "|" + field] = object.Var("_string");
      }
    }
    auto text = [&](const std::string& object, const char* field) {
      auto it = texts.find(object + "|" + field);
      return it == texts.end() ? std::string() : Text(it->second);
    };
    auto list_of = [&](const std::string& ref) -> const IgxObject* {
      auto it = objects.find(ref.rfind("this.", 0) == 0 ? ref.substr(5) : ref);
      return it == objects.end() ? nullptr : it->second;
    };
    const IgxObject* component = nullptr;
    for (const auto& object : parsed) {
      if (object.type == "CSkillsUpgradeComponentData") {
        component = &object;
        break;
      }
    }
    if (!component) return nullptr;

    auto result = std::make_shared<FigureUpgrades>();
    auto make_upgrade = [&](const std::string& ref, int bit) {
      FigureUpgrade u;
      const IgxObject* o = list_of(ref);
      u.name = text(ref, "_skillName");
      u.description = text(ref, "_upgradeDescription");
      u.bit = bit;
      if (o) {
        u.cost = uint32_t(std::strtoul(o->Var("_cost").c_str(), nullptr, 10));
        if (!o->Var("_previewImage").empty()) u.image = "upgrade:" + o->Var("_previewImage");
      }
      return u;
    };
    if (const IgxObject* start = list_of(component->Var("_initialAbilities"))) {
      FigureUpgradePath path;
      path.kind = FigureUpgradePath::Kind::kStart;
      for (const std::string& item : start->items) path.upgrades.push_back(make_upgrade(item, -1));
      if (!path.upgrades.empty()) result->paths.push_back(std::move(path));
    }
    // The paths in the list's order; the bits are counted afterwards: basic, paths, soul gem, Wow
    // Pow.
    const IgxObject* paths = list_of(component->Var("_upgrades"));
    std::vector<std::pair<FigureUpgradePath, std::vector<std::string>>> found;
    int regular = 0;
    for (const std::string& path_ref : paths ? paths->items : std::vector<std::string>{}) {
      const IgxObject* p = list_of(path_ref);
      const IgxObject* skills = p ? list_of(p->Var("_skills")) : nullptr;
      if (!skills || skills->items.empty()) continue;
      FigureUpgradePath path;
      path.name = text(path_ref, "_name");
      path.description = text(path_ref, "_description");
      bool wowpow = true, soul_gem = false;
      std::vector<std::string> items;
      for (const std::string& item : skills->items) {
        const IgxObject* o = list_of(item);
        if (!o) continue;
        wowpow &= o->type == "CSkillUpgradeWowpow";
        soul_gem |= !o->Var("_soulGemData").empty();
        items.push_back(item);
      }
      if (wowpow) {
        path.kind = FigureUpgradePath::Kind::kWowPow;
        std::erase_if(items, [&](const std::string& item) { return list_of(item)->Var("_yearCode") != year; });
      } else if (soul_gem) {
        path.kind = FigureUpgradePath::Kind::kSoulGem;
      } else {
        path.kind = regular++ == 0 ? FigureUpgradePath::Kind::kBasic : FigureUpgradePath::Kind::kPath;
      }
      if (!items.empty()) found.emplace_back(std::move(path), std::move(items));
    }
    int bit = kFirstUpgradeBit;
    for (auto kind : {FigureUpgradePath::Kind::kBasic, FigureUpgradePath::Kind::kPath,
                      FigureUpgradePath::Kind::kSoulGem, FigureUpgradePath::Kind::kWowPow}) {
      int longest = 0;
      for (auto& [path, items] : found) {
        if (path.kind != kind) continue;
        for (size_t j = 0; j < items.size(); ++j) {
          FigureUpgrade u = make_upgrade(items[j], bit + int(j));
          if (kind == FigureUpgradePath::Kind::kPath) u.alternate_bit = kFirstAlternateBit + int(j);
          path.upgrades.push_back(std::move(u));
        }
        longest = std::max(longest, int(items.size()));
        // The paths share their bits; every other kind is one list.
        if (kind != FigureUpgradePath::Kind::kPath) bit += int(items.size());
      }
      if (kind == FigureUpgradePath::Kind::kPath) bit += longest;
    }
    for (auto& [path, items] : found) result->paths.push_back(std::move(path));
    return result;
  }

  rex::memory::Memory* memory_;
  std::filesystem::path archives_;
  std::thread thread_;
  std::mutex mutex_;
  std::condition_variable cv_;
  bool started_ = false, indexed_ = false, failed_ = false, stop_ = false;
  std::deque<std::string> queue_;
  std::unordered_set<std::string> pending_;
  std::unordered_map<std::string, std::shared_ptr<const FigureImage>> images_;
  std::unordered_map<std::string, std::shared_ptr<const FigureUpgrades>> upgrades_;
  std::unordered_map<uint32_t, std::shared_ptr<const HatInfo>> hat_infos_;

  // Written by the worker before indexed_ is set, read-only afterwards.
  std::unordered_map<uint16_t, Hat> hats_;
  std::unordered_map<std::string, uint16_t> hat_trackers_;     // "hat_ufohat" -> hat number
  std::unordered_map<std::string, std::string> stat_names_en_;  // eCA_* -> English
  std::unordered_map<uint32_t, std::string> stat_names_de_;     // FNV-1a(English) -> German
  std::unordered_map<std::string, std::string> stat_icons_;     // eCA_* -> material, lower case
  // Worker only.
  std::map<std::string, std::unique_ptr<IgaArchive>> paks_;
  std::unordered_map<std::string, std::vector<UiTexture>> material_textures_;
};

std::unique_ptr<GameArt> g_game_art;

}  // namespace

void InstallGameArt(rex::memory::Memory* memory, std::filesystem::path archives_dir) {
  if (!memory || g_game_art) return;
  g_game_art = std::make_unique<GameArt>(memory, std::move(archives_dir));
  g_game_art->Start();
  skylanders::SetArtImageProvider(
      [](const std::string& key) { return g_game_art->Image(key); });
  skylanders::SetFigureUpgradesProvider(
      [](uint16_t id, uint16_t variant) { return g_game_art->Upgrades(id, variant); });
  skylanders::SetHatInfoProvider([](uint16_t hat) { return g_game_art->HatInfoFor(hat); });
}

skylanders::ControllerPicture GameArtControllerPicture() {
  // The buttons in Xbox_Controller.tga, measured on the picture, moved into the cut-out.
  static const skylanders::ControllerSpot kSpots[] = {
      {"guide", 575, 377, 33}, {"back", 511, 380, 15}, {"start", 638, 380, 15},
      {"y", 750, 337, 20},     {"x", 712, 379, 20},    {"b", 789, 378, 20},
      {"a", 750, 422, 20},     {"ls", 403, 386, 42},   {"rs", 652, 470, 40},
      {"up", 497, 433, 13},    {"down", 497, 489, 13}, {"left", 469, 461, 13},
      {"right", 525, 461, 13}, {"lb", 402, 262, 22},   {"rb", 748, 258, 22},
  };
  skylanders::ControllerPicture picture;
  picture.art_key = "controller:xbox360";
  picture.width = kControllerW;
  picture.height = kControllerH;
  for (skylanders::ControllerSpot spot : kSpots) {
    spot.x -= kControllerX;
    spot.y -= kControllerY;
    picture.spots.push_back(spot);
  }
  return picture;
}

std::shared_ptr<const skylanders::FigureImage> GameArtHatIcon(const std::string& tracker) {
  return g_game_art ? g_game_art->HatIcon(tracker) : nullptr;
}

void ShutdownGameArt() {
  skylanders::SetArtImageProvider(nullptr);
  skylanders::SetFigureUpgradesProvider(nullptr);
  skylanders::SetHatInfoProvider(nullptr);
  if (g_game_art) g_game_art->Stop();
  g_game_art.reset();
}

}  // namespace swapforcerecomp
