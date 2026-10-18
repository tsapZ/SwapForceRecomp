#pragma once

// Reader for Alchemy IGZ files (version 7, big-endian, 32-bit pointers) as far as the game's UI
// textures and material instances need it.
//
//   0x00 'IGZ\x01'  0x04 version  0x10 fixup count
//   0x14 section descriptors, 16 bytes each until offset 0: (pool name, offset, length, align);
//        section 0 holds the fixup records, the others are the memory pools objects live in.
//   Fixup record: fourcc (stored byte-reversed), count, length, data start, data. Used here:
//     TSTR strings, TMET type names, EXID external handles (name hash, namespace hash),
//     TMHN memory handles (u32 size | flags, u32 pointer), RVTB packed offsets of every object.
//   A serialized pointer is pool = value >> 27, offset = value & 0x07FFFFFF.
//   igImage2 on Xenon: +0x30 u16 width, height, depth, level count; +0x3C format (0x80000000 |
//   EXID index, whose name hash is FNV-1a of the format name, e.g. "dxt5_big_xenon"); +0x48 index
//   into TMHN for the pixel data.
// Worked out from the public igLibrary / igArchiveExtractor (NefariousTechSupport) and checked
// against the game's files. UI textures are DXT1/DXT5, one mip level, linear rows padded to 32
// blocks, with 16-bit words byte-swapped.

#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace swapforcerecomp::alchemy {

struct RgbaImage {
  int width = 0;
  int height = 0;
  std::vector<uint8_t> pixels;  // width * height * 4, RGBA
};

class IgzFile {
 public:
  static std::optional<IgzFile> Parse(std::vector<uint8_t> data);

  // The strings of a fixup record ("TSTR", "TMET", "TDEP"), empty if there is none.
  std::vector<std::string> Strings(const char* tag) const;

  // The first igImage2 in the file, decoded; nullopt if there is none or its format isn't DXT1/3/5.
  std::optional<RgbaImage> DecodeFirstImage() const;

 private:
  struct Fixup {
    uint32_t count = 0;
    size_t start = 0;
    size_t end = 0;
  };

  uint32_t Be32(size_t at) const;
  uint16_t Be16(size_t at) const;
  std::optional<size_t> Deserialize(uint32_t value) const;
  std::vector<size_t> ObjectOffsets() const;

  std::vector<uint8_t> data_;
  uint32_t version_ = 0;
  std::vector<size_t> pools_;
  std::unordered_map<std::string, Fixup> fixups_;
};

}  // namespace swapforcerecomp::alchemy
