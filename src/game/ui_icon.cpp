#include "ui_icon.h"

#include <algorithm>
#include <cctype>

#include "alchemy/igz_file.h"

namespace swapforcerecomp {

// The games' shared Alchemy readers (skylanders-portal src/alchemy).
namespace alchemy = skylanders::alchemy;

using alchemy::IgzFile;

std::string Lower(std::string s) {
  std::transform(s.begin(), s.end(), s.begin(),
                 [](unsigned char c) { return char(std::tolower(c)); });
  return s;
}

namespace {

bool IsUtf8(const std::string& s) {
  for (size_t i = 0; i < s.size();) {
    const unsigned char c = static_cast<unsigned char>(s[i]);
    const size_t extra =
        c < 0x80 ? 0 : (c >> 5) == 6 ? 1 : (c >> 4) == 14 ? 2 : (c >> 3) == 30 ? 3 : 4;
    if (extra == 4 || i + extra >= s.size()) return extra == 0;
    for (size_t k = 1; k <= extra; ++k) {
      if ((static_cast<unsigned char>(s[i + k]) >> 6) != 2) return false;
    }
    i += extra + 1;
  }
  return true;
}

}  // namespace

std::string ToUtf8(const std::string& s) {
  if (IsUtf8(s)) return s;
  std::string out;
  for (unsigned char c : s) {
    if (c < 0x80) {
      out.push_back(char(c));
    } else {
      out.push_back(char(0xC0 | (c >> 6)));
      out.push_back(char(0x80 | (c & 0x3F)));
    }
  }
  return out;
}

std::string SourceBaseName(const std::string& source) {
  std::string name = Lower(source);
  const size_t slash = name.find_last_of("\\/:");
  if (slash != std::string::npos) name.erase(0, slash + 1);
  const size_t dot = name.rfind('.');
  if (dot != std::string::npos) name.erase(dot);
  return name;
}

std::optional<size_t> FindFile(const alchemy::IgaArchive& pak, const std::string& path) {
  const std::string want = Lower(path);
  for (size_t i = 0; i < pak.size(); ++i) {
    if (Lower(pak.name(i)) == want) return i;
  }
  return std::nullopt;
}

std::shared_ptr<const skylanders::FigureImage> DecodeUiIcon(const alchemy::IgaArchive& pak,
                                                           size_t index) {
  auto data = pak.Read(index);
  auto texture = data ? IgzFile::Parse(std::move(*data)) : std::nullopt;
  auto rgba = texture ? texture->DecodeFirstImage() : std::nullopt;
  if (!rgba) return nullptr;
  auto image = std::make_shared<skylanders::FigureImage>();
  image->width = rgba->width;
  image->height = rgba->height;
  image->rgba = std::move(rgba->pixels);
  const size_t row = size_t(image->width) * 4;
  for (int y = 0; y < image->height / 2; ++y) {
    std::swap_ranges(image->rgba.begin() + ptrdiff_t(y * row),
                     image->rgba.begin() + ptrdiff_t((y + 1) * row),
                     image->rgba.begin() + ptrdiff_t((image->height - 1 - y) * row));
  }
  return image;
}

std::vector<UiTexture> MaterialTextures(const alchemy::IgaArchive& pak, size_t material_index) {
  std::vector<UiTexture> out;
  auto data = pak.Read(material_index);
  auto igz = data ? IgzFile::Parse(std::move(*data)) : std::nullopt;
  if (!igz) return out;
  for (const std::string& dep : igz->Strings("TDEP", 2)) {
    const std::string lower = Lower(dep);
    if (lower.rfind("textures:/", 0) != 0) continue;
    auto index = FindFile(pak, "textures\\" + lower.substr(10));
    if (!index) continue;
    auto texture_data = pak.Read(*index);
    auto texture = texture_data ? IgzFile::Parse(std::move(*texture_data)) : std::nullopt;
    if (!texture) continue;
    const auto strings = texture->Strings("TSTR");
    if (strings.empty()) continue;
    out.push_back({Lower(strings[0]), *index});
  }
  return out;
}

}  // namespace swapforcerecomp
