#include <stdafx.hpp>
#include "ram.h"

namespace fs = std::filesystem;

std::vector<ubyte> load_file(fs::path const& filepath) {
    std::ifstream ifs(filepath, std::ios::binary | std::ios::ate);

    if (!ifs)
        throw std::runtime_error(filepath.string() + ": " + std::strerror(errno));

    const auto end = ifs.tellg();
    ifs.seekg(0, std::ios::beg);

    const auto size = std::size_t(end - ifs.tellg());

    if (size == 0)  // avoid undefined behavior
        return {};

    const std::vector<ubyte> buf(size);

    if (!ifs.read((char*)buf.data(), buf.size()))
        throw std::runtime_error(filepath.string() + ": " + std::strerror(errno));

    return buf;
}

PSEXELoadInfo Ram::executable()
{
    auto m_psxexe_path = "C:\\Users\\Alex\\Desktop\\PSXemu\\PSXemu\\mahjongg.exe";
    const std::vector<ubyte>& psx_exe_buf = load_file(m_psxexe_path);

    if (psx_exe_buf.empty())
        exit(0);

    struct PSXEXEHeader {
        char magic[8];  // "PS-X EXE"
        ubyte pad0[8];
        uint pc;         // initial PC
        uint r28;        // initial R28
        uint load_addr;  // destination address in RAM
        uint filesize;   // excluding header & must be N*0x800
        uint unk0[2];
        uint memfill_start;
        uint memfill_size;
        uint r29_r30;         // initial r29 and r30 base
        uint r29_r30_offset;  // initial r29 and r30 offset, added to above
        // etc, we don't care about anything else
    };

    const auto psx_exe = (PSXEXEHeader*)psx_exe_buf.data();

    if (std::strcmp(&psx_exe->magic[0], "PS-X EXE")) {
        printf("Not a valid PS-X EXE file!\n");
        exit(0);
    }

    // We don't support memfill
    assert(psx_exe->memfill_start == 0);
    assert(psx_exe->memfill_size == 0);

    PSEXELoadInfo info;
    info.pc = psx_exe->pc;
    info.r28 = psx_exe->r28;
    info.r29_r30 = psx_exe->r29_r30 + psx_exe->r29_r30_offset;

    constexpr auto PSXEXE_HEADER_SIZE = 0x800;

    const auto copy_src_begin = psx_exe_buf.data() + PSXEXE_HEADER_SIZE;
    const auto copy_src_end = copy_src_begin + psx_exe->filesize;
    const auto copy_dest_begin = buffer + (psx_exe->load_addr & 0x7FFFFFFF);

    std::copy(copy_src_begin, copy_src_end, copy_dest_begin);
    return info;
}
