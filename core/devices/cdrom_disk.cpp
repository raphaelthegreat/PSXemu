#include <stdafx.hpp>
#include "cdrom_disk.hpp"

constexpr CDPos::CDPos(ubyte minutes, ubyte seconds, ubyte frames) :
    minutes(minutes), seconds(seconds), frames(frames) {}

CDPos CDPos::from_lba(uint lba)
{
    ubyte minutes = (ubyte)((uint)lba / 60 / SECTORS_PER_SECOND);
    ubyte seconds = (ubyte)((uint)lba % (60 * SECTORS_PER_SECOND) / SECTORS_PER_SECOND);
    ubyte frames = (ubyte)((uint)lba % SECTORS_PER_SECOND);
    return CDPos(minutes, seconds, frames);
}

uint CDPos::to_lba() const
{
    return (minutes * 60 * SECTORS_PER_SECOND) +
        (seconds * SECTORS_PER_SECOND) + frames;
}

CDPos CDPos::operator+(const CDPos& lhs) const
{
    return from_lba(to_lba() + lhs.to_lba());
}

CDPos CDPos::operator-(const CDPos& lhs) const
{
    return from_lba(to_lba() - lhs.to_lba());
}

inline void CDPos::physical_to_logical()
{
    *this = *this - CDPos(0, 2, 0);
}

inline void CDPos::logical_to_physical()
{
    *this = *this + CDPos(0, 2, 0);
}

void CDDisk::parse_bin(const std::string& bin_path) {
    filepath = bin_path;
    create_track_for_bin(bin_path);

    tracks[0].file.open(bin_path, std::ios::binary | std::ios::in);
}

std::vector<ubyte> CDDisk::read(CDPos pos, DataType& sector_type) {
    auto track = get_track_by_pos(pos);

    if (!track) {
        printf("[CDROM] Reading failed, no disk loaded\n");
        sector_type = DataType::Invalid;
        return {};
    }

    std::vector<ubyte> sector_buf(SECTOR_SIZE);

    /* Convert physical position (on real CDROMs) to logical (on .bin files). */
    if (track->number == 1 && track->type == DataType::Data)
        pos.physical_to_logical();

    const auto seek_pos = pos.to_lba() * SECTOR_SIZE;
    track->file.seekg(seek_pos);
    track->file.read((char*)sector_buf.data(), SECTOR_SIZE);

    sector_type = track->type;
    return sector_buf;
}

void CDDisk::create_track_for_bin(const std::string& bin_path) {
    CDTrack bin_track = {};

    const auto filesize = fs::file_size(bin_path);
    if (filesize == 0)
        return;

    bin_track.filepath = bin_path;
    bin_track.number = 1;  /* Track number 01. */
    bin_track.type = DataType::Data;
    bin_track.frame_count = static_cast<uint>(filesize) / SECTOR_SIZE;

    tracks.clear();
    tracks.emplace_back(std::move(bin_track));
}

inline const CDTrack& CDDisk::track(uint track_number) const
{
    return tracks[track_number];
}

ubyte CDDisk::get_track_count() const
{
    const auto track_count = tracks.size();
    assert(track_count <= 99);
    
    return (ubyte)track_count;
}

CDPos CDDisk::get_track_start(uint track_number) const
{
    uint start{};

    const auto track_count = get_track_count();

    if (track_count > 0) {
        /* Add pregap if necessary.*/
        if (tracks[0].type == DataType::Data)
            start += PREGAP_FRAME_COUNT;

        if (track_count > 1) {
            for (uint t = 0; t < track_number - 1; t++)
                start += tracks[t].frame_count;
        }
    }

    return CDPos::from_lba(start);
}

CDTrack* CDDisk::get_track_by_pos(CDPos pos)
{
    for (auto i = 0; i < tracks.size(); ++i) {
        const auto pos_lba = pos.to_lba();
        const auto start = get_track_start(i).to_lba();
        const auto size = tracks[i].frame_count;

        if (pos_lba >= start && pos_lba < start + size)
            return &tracks[i];
    }
    return nullptr;
}


CDPos CDDisk::size()
{
    uint sectors = 0;
    for (auto& t : tracks) {
        sectors += t.frame_count;
    }

    return CDPos::from_lba(sectors) + CDROM_INDEX_1_POS;
}

bool CDDisk::is_empty()
{
    return tracks.empty();
}
