#pragma once

namespace fs = std::filesystem;

constexpr uint SECTOR_SIZE = 2352;
constexpr uint SECTORS_PER_SECOND = 75;
constexpr uint PREGAP_FRAME_COUNT = SECTORS_PER_SECOND * 2;

enum class DataType {
    Invalid,
    Audio,
    Data,
};

class CDPos {
public:
    CDPos() = default;
    constexpr CDPos(ubyte minutes, ubyte seconds, ubyte frames);

    static CDPos from_lba(uint lba);
    uint to_lba() const;
   
    void physical_to_logical();
    void logical_to_physical();

    CDPos operator+(const CDPos& lhs) const;
    CDPos operator-(const CDPos& lhs) const;

public:
    /* Track ('TT') is always 1. */
    ubyte minutes = 0;
    ubyte seconds = 0;
    ubyte frames = 0;
};

static CDPos CDROM_INDEX_1_POS = CDPos(0, 2, 0);

struct CDTrack {
    DataType type = DataType::Invalid;
    uint number = 0;

    CDPos pregap = {};
    CDPos start = {};

    /* File offset in sectors/frames. */
    uint offset = 0;
    uint frame_count = 0;

    std::string filepath;
    std::ifstream file;
};

class CDDisk {
public:
    CDDisk() = default;
    ~CDDisk() = default;

    std::vector<ubyte> read(CDPos pos, DataType& sector_type);

    void parse_bin(const std::string& bin_path);

    ubyte get_track_count() const;
    CDTrack* get_track_by_pos(CDPos pos);
    CDPos get_track_start(uint track_number) const;
    inline const CDTrack& track(uint track_number) const;

    CDPos size();
    bool is_empty();

private:
    void create_track_for_bin(const std::string& bin_path);

    std::string filepath;
    std::vector<CDTrack> tracks;
};
