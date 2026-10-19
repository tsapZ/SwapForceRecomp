// Pictures and names for the portal menu from the game's own data.
//
// The chain, all inside the owner's game files (rom/archives), nothing of it in the repository:
//   1. The game's toy type enum maps names to figure ids, and those ids are the same as the portal
//      catalog's (Character_ChopChop = 30, Shapeshifter_Body_Fortune = 2008 for Hoot Loop's top,
//      Item_Anvil = 200). It is read from the loaded executable: 159 pointers to the names at
//      0x820FD058 and their values right after at 0x820FD2D8. Found in Ghidra (2026-10-18): the
//      only pointer to "kTfbSpyroTag_ToyType_Character_Spyro" (0x820FD808) sits at 0x820FD09C in
//      that array; every value was checked against the catalog.
//   2. toydata.pak holds one .igx (XML) file per toy: its "_toyId", the collection material
//      ("Collection_ChopChop_Normal") and its variants, each identified by year, deco id and three
//      flags, which together make the catalog's variant number:
//        variant = year << 12 | wow_pow << 11 | alt_deco << 10 | light_core << 9 | deco_id
//      Next to each are _en/_de files with the names the game shows ("Ambossregen").
//   3. toycollectionimages.pak holds the material instances; each names its texture by source
//      path ("textures:\ui\Collections\ToyInventoryIcons\2011\Collections_ChopChop.tga"), and every
//      texture file there carries the same path as its first string. The textures are 192x192
//      DXT5 icons, the ones the game's collection screen shows.
// Swapper bottoms have no icon of their own; they show their top's. Variants without their own
// picture (most chase variants) show the base toy's. Sidekicks (CPetToyData) name no material; their
// icons follow the character name: Pet_Terrabite -> collection_pet_terrabite_normal, and the
// Giants sidekicks get "lil" (Pet_Treerex, Barkley -> collection_pet_liltreerex_normal).

#include "toy_images.h"

#include <algorithm>
#include <atomic>
#include <cctype>
#include <chrono>
#include <condition_variable>
#include <deque>
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
#include "alchemy/igz_file.h"
#include "guest_memory.h"
#include "portal/figure_catalog.h"
#include "portal_rex/portal_menu.h"
#include "ui_icon.h"

namespace swapforcerecomp {
namespace {

using alchemy::IgaArchive;
using alchemy::IgzFile;

constexpr uint32_t kToyTypeNames = 0x820FD058;
constexpr uint32_t kToyTypeValues = 0x820FD2D8;
constexpr uint32_t kToyTypeCount = 159;
constexpr char kToyTypePrefix[] = "kTfbSpyroTag_ToyType_";

bool EndsWith(const std::string& s, const std::string& suffix) {
  return s.size() >= suffix.size() && s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0;
}

// "Collection_ChopChop_Normal.Collection_ChopChop_Normal" -> "collection_chopchop_normal".
std::string MaterialKey(const std::string& ref) { return Lower(ref.substr(0, ref.find('.'))); }

// The part of a texture path after "textures", lower case, without leading separators:
// "U:\data\textures\ui\Collections\X.tga" and "textures:\ui\Collections\X.tga" both give
// "ui\collections\x.tga".
std::optional<std::string> TextureKey(const std::string& path) {
  const std::string lower = Lower(path);
  size_t at = lower.find("textures");
  if (at == std::string::npos) return std::nullopt;
  at += 8;
  while (at < lower.size() && (lower[at] == ':' || lower[at] == '\\' || lower[at] == '/')) ++at;
  std::string key = lower.substr(at);
  std::replace(key.begin(), key.end(), '/', '\\');
  return key.empty() ? std::nullopt : std::optional<std::string>(key);
}

uint32_t Key(uint16_t id, uint16_t variant) { return (uint32_t(id) << 16) | variant; }

class ToyImages {
 public:
  ToyImages(rex::memory::Memory* memory, std::filesystem::path archives)
      : memory_(memory), archives_(std::move(archives)) {}

  ~ToyImages() { Stop(); }

  // Indexes right away, so the first portal menu already has every picture and name.
  void Start() {
    std::lock_guard lock(mutex_);
    StartLocked();
  }

  void Stop() {
    {
      std::lock_guard lock(mutex_);
      stop_ = true;
    }
    cv_.notify_all();
    if (thread_.joinable()) thread_.join();
  }

  // UI thread, every frame: never blocks.
  std::shared_ptr<const skylanders::FigureImage> Image(uint16_t id, uint16_t variant) {
    std::lock_guard lock(mutex_);
    StartLocked();
    if (failed_) return nullptr;
    const uint32_t key = Key(id, variant);
    if (auto it = images_.find(key); it != images_.end()) return it->second;
    if (pending_.insert(key).second) {
      queue_.push_back(key);
      cv_.notify_one();
    }
    return nullptr;
  }

  std::optional<std::string> Name(uint16_t id, uint16_t variant) {
    {
      std::lock_guard lock(mutex_);
      StartLocked();
      if (!indexed_) return std::nullopt;
    }
    // The index is complete and no longer changes once indexed_ is set.
    auto toy = toys_.find(id);
    if (toy == toys_.end() || toy->second.part) return std::nullopt;
    const bool german = Lower(rex::cvar::GetFlagByName("language")) == "german";
    const auto& names = german ? names_de_ : names_en_;
    // Only a name for exactly this figure: a chase variant without its own entry keeps the
    // catalog's more specific name.
    const std::string& ref = variant ? (FindVariant(toy->second, variant)
                                            ? FindVariant(toy->second, variant)->ref
                                            : std::string())
                                     : toy->second.ref;
    if (auto it = names.find(ref); !ref.empty() && it != names.end()) return it->second;
    return std::nullopt;
  }

  // The character a toy is (its <name>.pak), lower case: "chopchop", "body_statue".
  std::optional<std::string> Character(uint16_t id) {
    {
      std::lock_guard lock(mutex_);
      StartLocked();
      if (!indexed_) return std::nullopt;
    }
    auto toy = toys_.find(id);
    if (toy == toys_.end() || toy->second.character.empty()) return std::nullopt;
    return toy->second.character;
  }

 private:
  struct Variant {
    uint16_t bits = 0;
    std::string material;
    std::string ref;
  };
  struct Toy {
    std::string ref;
    std::string material;
    std::vector<Variant> variants;
    bool part = false;     // a swapper half
    std::string pet_name;  // sidekicks: "terrabite" from _characterName Pet_Terrabite
    std::string character;  // _characterName, or for a swapper half its file: "body_statue"
  };

  void StartLocked() {
    if (started_) return;
    started_ = true;
    thread_ = std::thread([this] { Run(); });
  }

  const Variant* FindVariant(const Toy& toy, uint16_t variant) const {
    if (!variant) return nullptr;
    for (const Variant& v : toy.variants) {
      if (v.bits == variant) return &v;
    }
    return nullptr;
  }

  // The toy type table is part of the loaded executable; at start-up it may not be readable yet.
  bool WaitForToyTable() {
    for (int i = 0; i < 240; ++i) {
      const auto name_ptr = ReadGuest32(memory_, kToyTypeNames);
      const auto name = name_ptr ? ReadGuestString(memory_, *name_ptr) : std::nullopt;
      if (name && name->rfind(kToyTypePrefix, 0) == 0) return true;
      std::unique_lock lock(mutex_);
      if (cv_.wait_for(lock, std::chrono::milliseconds(500), [this] { return stop_; })) {
        return false;
      }
    }
    return false;
  }

  void Run() {
    if (!WaitForToyTable()) {
      std::lock_guard lock(mutex_);
      failed_ = true;
      return;
    }
    const auto start = std::chrono::steady_clock::now();
    const bool ok = BuildIndex();
    {
      std::lock_guard lock(mutex_);
      indexed_ = ok;
      failed_ = !ok;
    }
    if (ok) {
      REXLOG_INFO("Toy images: {} toys, {} materials, {} textures indexed in {} ms", toys_.size(),
                  materials_.size(), textures_.size(),
                  std::chrono::duration_cast<std::chrono::milliseconds>(
                      std::chrono::steady_clock::now() - start)
                      .count());
      LogWithoutPicture();
    }
    while (ok) {
      uint32_t key;
      {
        std::unique_lock lock(mutex_);
        cv_.wait(lock, [this] { return stop_ || !queue_.empty(); });
        if (stop_) return;
        key = queue_.front();
        queue_.pop_front();
      }
      auto image = Decode(uint16_t(key >> 16), uint16_t(key & 0xFFFF));
      std::lock_guard lock(mutex_);
      images_[key] = std::move(image);
      pending_.erase(key);
    }
  }

  bool BuildIndex() {
    // 1. Toy type names -> ids, from the loaded executable.
    std::unordered_map<std::string, uint16_t> ids;
    for (uint32_t i = 0; i < kToyTypeCount; ++i) {
      const auto name_ptr = ReadGuest32(memory_, kToyTypeNames + 4 * i);
      const auto value = ReadGuest32(memory_, kToyTypeValues + 4 * i);
      const auto name = name_ptr ? ReadGuestString(memory_, *name_ptr) : std::nullopt;
      if (!value || !name || name->rfind(kToyTypePrefix, 0) != 0) {
        REXLOG_WARN("Toy images: the toy type table isn't where it was expected; no pictures");
        return false;
      }
      ids.emplace(*name, uint16_t(*value));
    }

    // 2. Toy data.
    auto toydata = IgaArchive::Open(archives_ / "toydata.pak");
    if (!toydata) {
      REXLOG_WARN("Toy images: cannot open {}", (archives_ / "toydata.pak").string());
      return false;
    }
    // Object names are only unique within their file (every swapper's toy is called "ToyData"),
    // so everything is keyed "<file>.<object>", as the language files refer to them.
    static const std::unordered_map<std::string, uint32_t> kYears = {
        {"eSG_Spyro2011", 0}, {"eSG_Giants2012", 1}, {"eSG_Shapeshifters2013", 2}};
    static const std::unordered_map<std::string, uint32_t> kDecos = {
        {"Normal", 0}, {"Repose1", 1}, {"AlternateDeco", 2},
        {"Legendary", 3}, {"Event", 4}, {"Repose2", 5}};
    for (size_t i = 0; i < toydata->size(); ++i) {
      std::string name = Lower(toydata->name(i));
      if (!EndsWith(name, ".igx")) continue;
      name.erase(0, name.rfind('\\') + 1);
      name.erase(name.size() - 4);  // "chopchop_toydata", "chopchop_toydata_de"
      const bool en = EndsWith(name, "_en"), de = EndsWith(name, "_de");
      if (!en && !de && name.size() >= 3 && name[name.size() - 3] == '_') continue;  // other languages
      auto data = toydata->Read(i);
      if (!data) continue;
      std::vector<alchemy::IgxObject> parsed = alchemy::ParseIgx(
          std::string_view(reinterpret_cast<const char*>(data->data()), data->size()));
      if (en || de) {
        for (const auto& object : parsed) {
          if (object.type != "igLocalizedString" ||
              object.Var("_field").find("::_toyName") == std::string::npos) {
            continue;
          }
          (de ? names_de_ : names_en_)[Lower(object.Var("_object"))] =
              ToUtf8(object.Var("_string"));
        }
        continue;
      }
      std::unordered_map<std::string, const alchemy::IgxObject*> objects;
      for (const auto& object : parsed) objects[object.refname] = &object;
      for (const auto& object : parsed) {
        auto id = ids.find(object.Var("_toyId"));
        if (id == ids.end()) continue;
        Toy toy;
        toy.ref = name + "." + Lower(object.refname);
        toy.material = MaterialKey(object.Var("_toyCollectionMaterial"));
        toy.part = object.type == "CShapeshifterPartToyData";
        toy.character = Lower(object.Var("_characterName"));
        if (toy.character.empty() && toy.part && EndsWith(name, "_toydata")) {
          toy.character = name.substr(0, name.size() - 8);
        }
        if (object.type == "CPetToyData") {
          toy.pet_name = Lower(object.Var("_characterName"));
          if (toy.pet_name.rfind("pet_", 0) == 0) toy.pet_name.erase(0, 4);
        }
        std::string list_ref = object.Var("_variants");
        if (list_ref.rfind("this.", 0) == 0) list_ref.erase(0, 5);
        if (auto list = objects.find(list_ref); list != objects.end()) {
          for (const std::string& item : list->second->items) {
            auto v = objects.find(item);
            if (v == objects.end()) continue;
            const auto& o = *v->second;
            std::string deco = o.Var("_decoId");
            if (deco.rfind("kTfbSpyroTag_DecoID_", 0) == 0) deco.erase(0, 20);
            auto year = kYears.find(o.Var("_yearCode"));
            auto deco_id = kDecos.find(deco);
            if (year == kYears.end() || deco_id == kDecos.end()) continue;
            Variant variant;
            variant.bits = uint16_t(year->second << 12 | (o.Var("_wowPowFlag") == "true") << 11 |
                                    (o.Var("_fullAltDecoFlag") == "true") << 10 |
                                    (o.Var("_lightCoreFlag") == "true") << 9 | deco_id->second);
            variant.material = MaterialKey(o.Var("_toyCollectionMaterial"));
            variant.ref = name + "." + Lower(item);
            toy.variants.push_back(std::move(variant));
          }
        }
        toys_[id->second] = std::move(toy);
      }
    }
    // 3. Collection images: materials by name, textures by source path.
    images_pak_ = IgaArchive::Open(archives_ / "toycollectionimages.pak");
    if (!images_pak_) {
      REXLOG_WARN("Toy images: cannot open {}", (archives_ / "toycollectionimages.pak").string());
      return false;
    }
    for (size_t i = 0; i < images_pak_->size(); ++i) {
      std::string name = Lower(images_pak_->name(i));
      if (!EndsWith(name, ".igz")) continue;
      if (name.rfind("materialinstances\\", 0) == 0) {
        const size_t slash = name.rfind('\\');
        materials_[name.substr(slash + 1, name.size() - slash - 5)] = i;
      } else if (name.rfind("textures\\", 0) == 0) {
        auto data = images_pak_->Read(i);
        auto igz = data ? IgzFile::Parse(std::move(*data)) : std::nullopt;
        if (!igz) continue;
        const auto strings = igz->Strings("TSTR");
        if (strings.empty()) continue;
        if (auto key = TextureKey(strings[0])) textures_[*key] = i;
      }
    }
    for (auto& [id, toy] : toys_) {
      if (!toy.material.empty() || toy.pet_name.empty()) continue;
      for (const char* prefix : {"collection_pet_", "collection_pet_lil"}) {
        const std::string material = prefix + toy.pet_name + "_normal";
        if (materials_.contains(material)) {
          toy.material = material;
          break;
        }
      }
    }
    return !toys_.empty() && !textures_.empty();
  }

  // Lists the figures of the games the portal menu offers that the game has no collection icon
  // for (they show their element symbol), once, for the log.
  void LogWithoutPicture() const {
    const std::string games = rex::cvar::GetFlagByName("portal_menu_games");
    std::string list;
    size_t count = 0;
    for (const auto& sky : skylanders::portal::AllSkylanders()) {
      if (sky.category == "Unreleased" || games.find(sky.game) == std::string::npos) continue;
      // A swapper bottom shows its top.
      const uint16_t id = sky.id >= 1000 && sky.id < 2000 ? uint16_t(sky.id + 1000) : sky.id;
      auto toy = toys_.find(id);
      std::string material;
      if (toy != toys_.end()) {
        const Variant* v = FindVariant(toy->second, sky.variant);
        material = v && !v->material.empty() ? v->material : toy->second.material;
      }
      if (!material.empty() && materials_.contains(material)) continue;
      ++count;
      list += (list.empty() ? "" : ", ") + std::string(sky.name) +
              (toy == toys_.end() ? " (unknown toy)" : "");
    }
    if (count) REXLOG_INFO("Toy images: no picture for {} figures: {}", count, list);
  }

  std::shared_ptr<const skylanders::FigureImage> Decode(uint16_t id, uint16_t variant) {
    auto toy = toys_.find(id);
    std::string material;
    if (toy != toys_.end()) {
      const Variant* v = FindVariant(toy->second, variant);
      material = v && !v->material.empty() ? v->material : toy->second.material;
    }
    // A swapper bottom (id 1000-1015) shows its top (id + 1000).
    if (material.empty() && id >= 1000 && id < 2000) return Decode(uint16_t(id + 1000), variant);
    if (material.empty()) return nullptr;

    auto material_file = materials_.find(material);
    if (material_file == materials_.end()) return nullptr;
    auto data = images_pak_->Read(material_file->second);
    auto igz = data ? IgzFile::Parse(std::move(*data)) : std::nullopt;
    if (!igz) return nullptr;
    std::optional<size_t> texture_file;
    for (const std::string& s : igz->Strings("TSTR")) {
      if (Lower(s).rfind("textures:", 0) != 0) continue;
      auto key = TextureKey(s);
      if (auto it = key ? textures_.find(*key) : textures_.end(); it != textures_.end()) {
        texture_file = it->second;
        break;
      }
    }
    if (!texture_file) return nullptr;
    if (auto cached = by_texture_.find(*texture_file); cached != by_texture_.end()) {
      return cached->second;
    }
    auto image = DecodeUiIcon(*images_pak_, *texture_file);
    by_texture_[*texture_file] = image;
    return image;
  }

  rex::memory::Memory* memory_;
  std::filesystem::path archives_;
  std::thread thread_;

  std::mutex mutex_;
  std::condition_variable cv_;
  bool started_ = false;
  bool indexed_ = false;
  bool failed_ = false;
  bool stop_ = false;
  std::deque<uint32_t> queue_;
  std::unordered_set<uint32_t> pending_;
  std::unordered_map<uint32_t, std::shared_ptr<const skylanders::FigureImage>> images_;

  // Written by the worker before indexed_ is set, read-only afterwards.
  std::unordered_map<uint16_t, Toy> toys_;
  std::unordered_map<std::string, std::string> names_en_, names_de_;  // object ref -> name
  std::unordered_map<std::string, size_t> materials_;                 // material -> pak index
  std::unordered_map<std::string, size_t> textures_;                  // source path -> pak index
  std::optional<IgaArchive> images_pak_;
  // Worker only.
  std::unordered_map<size_t, std::shared_ptr<const skylanders::FigureImage>> by_texture_;
};

std::unique_ptr<ToyImages> g_toy_images;

}  // namespace

void InstallToyImages(rex::memory::Memory* memory, std::filesystem::path archives_dir) {
  if (!memory || g_toy_images) return;
  g_toy_images = std::make_unique<ToyImages>(memory, std::move(archives_dir));
  g_toy_images->Start();
  skylanders::SetFigureImageProvider(
      [](uint16_t id, uint16_t variant) { return g_toy_images->Image(id, variant); });
  skylanders::SetFigureNameProvider(
      [](uint16_t id, uint16_t variant) { return g_toy_images->Name(id, variant); });
}

std::optional<std::string> ToyCharacterName(uint16_t id) {
  return g_toy_images ? g_toy_images->Character(id) : std::nullopt;
}

void ShutdownToyImages() {
  skylanders::SetFigureImageProvider(nullptr);
  skylanders::SetFigureNameProvider(nullptr);
  if (g_toy_images) g_toy_images->Stop();
  g_toy_images.reset();
}

}  // namespace swapforcerecomp
