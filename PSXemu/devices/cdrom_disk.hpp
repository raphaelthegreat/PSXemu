#pragma once
#include <cstdint>
#include <fstream>
#include <string>
#include <vector>
#include <cassert>

constexpr uint32_t SECTORS_PER_SECOND = 75;
constexpr uint32_t SECTOR_SIZE = 2352;

using buffer = std::vector<uint8_t>;
namespace fs = std::filesystem;

namespace io {

// Represents a position in mm:ss:ff (minute-second-frame) format
struct CdromPosition {
  // track ('TT') is always 01
  uint8_t minutes{};  // 'MM'
  uint8_t seconds{};  // 'SS'
  uint8_t frames{};   // 'FF'

  CdromPosition() = default;
  constexpr CdromPosition(uint8_t minutes, uint8_t seconds, uint8_t frames)
      : minutes(minutes),
        seconds(seconds),
        frames(frames) {}

  static CdromPosition from_lba(uint32_t lba) {
    uint8_t minutes = (uint8_t)((uint32_t)lba / 60 / SECTORS_PER_SECOND);
    uint8_t seconds = (uint8_t)((uint32_t)lba % (60 * SECTORS_PER_SECOND) / SECTORS_PER_SECOND);
    uint8_t frames = (uint8_t)((uint32_t)lba % SECTORS_PER_SECOND);
    return CdromPosition(minutes, seconds, frames);
  }
  uint32_t to_lba() const {
    return (minutes * 60 * SECTORS_PER_SECOND) + (seconds * SECTORS_PER_SECOND) + frames;
  }
  std::string to_str() const { return "{" + std::to_string(minutes) + "}:{" + std::to_string(seconds) + "}{" + std::to_string(frames) + "}"; }

  CdromPosition operator+(const CdromPosition& lhs) const { return from_lba(to_lba() + lhs.to_lba()); }
  CdromPosition operator-(const CdromPosition& lhs) const { return from_lba(to_lba() - lhs.to_lba()); }
  void physical_to_logical() { *this = *this - CdromPosition(0, 2, 0); }
  void logical_to_physical() { *this = *this + CdromPosition(0, 2, 0); }
};

using CdromSize = CdromPosition;

static constexpr auto CDROM_INDEX_1_POS = CdromPosition(0, 2, 0);
static constexpr auto PREGAP_FRAME_COUNT = SECTORS_PER_SECOND * 2;

struct CdromTrack {
  enum class DataType {
    Invalid,
    Audio,
    Data,
  };

  DataType type{ DataType::Invalid };
  std::string filepath;
  uint32_t number{};

  CdromSize pregap{};
  CdromPosition start{};

  uint32_t offset{};  // File offset in sectors/frames
  uint32_t frame_count{};

  std::ifstream file;

  const char* type_to_str() const {
    switch (type) {
      case DataType::Invalid: return "Invalid";
      case DataType::Audio: return "Audio";
      case DataType::Data: return "Data";
      default: return "<unknown>";
    }
  }
};

class CdromDisk {
 public:
  buffer read(CdromPosition pos, CdromTrack::DataType& sector_type);

  void init_from_bin(const std::string& bin_path);
  void init_from_cue(const std::string& cue_path);

  const CdromTrack& track(uint32_t track_number) const { return m_tracks[track_number]; }
  uint8_t get_track_count() const {
    const auto track_count = m_tracks.size();
    assert(track_count <= 99);  // There can't be more than 99 tracks
    return static_cast<uint8_t>(track_count);
  }

  CdromSize size() const {
    uint32_t sectors{};
    for (auto& t : m_tracks)
      sectors += t.frame_count;
    return CdromPosition::from_lba(sectors) + CDROM_INDEX_1_POS;
  }

  CdromPosition get_track_start(uint32_t track_number) const {
    uint32_t start{};

    const auto track_count = get_track_count();

    if (track_count > 0) {
      // Add Pregap if necessary
      if (m_tracks[0].type == CdromTrack::DataType::Data)
        start += PREGAP_FRAME_COUNT;

      if (track_count > 1) {
        for (uint32_t t = 0; t < track_number - 1; t++)
          start += m_tracks[t].frame_count;
      }
    }

    return CdromPosition::from_lba(start);
  }

  // Returns nullptr if the position is out of bounds of laoded tracks
  CdromTrack* get_track_by_pos(CdromPosition pos) {
    for (auto i = 0; i < m_tracks.size(); ++i) {
      const auto pos_lba = pos.to_lba();
      const auto start = get_track_start(i).to_lba();
      const auto size = m_tracks[i].frame_count;

      if (pos_lba >= start && pos_lba < start + size)
        return &m_tracks[i];
    }
    return nullptr;
  }

  bool is_empty() const { return m_tracks.empty(); }

 private:
  void create_track_for_bin(const std::string& bin_path);

  std::string filepath;
  std::vector<CdromTrack> m_tracks;
};

}  // namespace io

namespace util {

inline uint8_t bcd_to_dec(uint8_t val) {
  return ((val / 16 * 10) + (val % 16));
}

inline uint8_t dec_to_bcd(uint8_t val) {
  return ((val / 10 * 16) + (val % 10));
}

}  // namespace util
