#include <cassert>
#include "gpu_core.h"
#include <memory/bus.h>
#include "vram.h"

#define BIND(x) std::bind(&GPU::x, this)

uint32_t GPU::data() {
    if (state.gpu_to_cpu_transfer.run.active) {
        auto lower = vram_transfer();
        auto upper = vram_transfer();
        return (upper << 16) | lower;
    }

    return 0;
}

uint32_t GPU::stat() {
    return (state.status.raw & ~0x00080000) | 0x1c002000;
}

uint16_t GPU::hblank_timings()
{
    if (state.status.video_mode == VideoMode::NTSC)
        return 3412;
    else
        return 3404;
}

uint16_t GPU::lines_per_frame()
{
    if (state.status.video_mode == VideoMode::NTSC)
        return 263;
    else
        return 314;
}

bool GPU::tick(uint32_t cycles)
{
    in_vblank = false;
    in_hblank = false;

    uint32_t cycles_per_line = hblank_timings();
    uint32_t scanlines_per_frame = lines_per_frame();
    VerticalRes vres = (VerticalRes)state.status.vres;
    bool is_480 = vres == VerticalRes::V480;

    /* Add the cycles to GPU pixel clock (this is in pixels). */
    gpu_clock += cycles * 11 / 7;

    /* Finished a scanline. */
    if (gpu_clock > cycles_per_line) {
        gpu_clock -= cycles_per_line;
        in_hblank = true;
        scanline++;

        if (is_480)
            state.status.odd_lines = scanline % 2 != 0;

        if (scanline > scanlines_per_frame) {
            scanline = 0;

            if (state.status.vertical_interlace && is_480)
                state.status.odd_lines = !state.status.odd_lines;

            in_vblank = true;
        }
    }

    return in_vblank;
}

GPU::GPU() :
    raster(this)
{
    state.status.raw = 0x14802000;
    register_commands();
}

uint32_t GPU::read(uint32_t address) {
    uint32_t offset = GPU_RANGE.offset(address);
    
    if (offset == 0)
        return data();
    else if (offset == 4)
        return stat();
}

void GPU::write(uint32_t address, uint32_t data) {
    uint32_t offset = GPU_RANGE.offset(address);
    
    if (offset == 0)
        return gp0(data);
    else if (offset == 4)
        return gp1(data);
}

void GPU::register_commands()
{

}

uint16_t GPU::vram_transfer() {
    auto& transfer = state.gpu_to_cpu_transfer;
    
    if (!transfer.run.active) {
        return 0;
    }

    auto data = vram.read(transfer.reg.x + transfer.run.x,
        transfer.reg.y + transfer.run.y);

    transfer.run.x++;

    if (transfer.run.x == transfer.reg.w) {
        transfer.run.x = 0;
        transfer.run.y++;

        if (transfer.run.y == transfer.reg.h) {
            transfer.run.y = 0;
            transfer.run.active = false;
        }
    }

    return data;
}

void GPU::vram_to_vram_transfer()
{
    auto src_coord = state.fifo[1];
    auto dest_coord = state.fifo[2];
    auto dim = state.fifo[3];

    glm::ivec2 src = unpack_point(src_coord);
    glm::ivec2 dest = unpack_point(dest_coord);
    glm::ivec2 size = unpack_point(dim);
    
    for (int y = 0; y < size.y; y++) {
        for (int x = 0; x < size.x; x++) {            
            int sx = (src.x + x) % 1024;
            int sy = (src.y + y) % 512;         
            auto data = vram.read(sx, sy);

            int dx = (dest.x + x) % 1024;
            int dy = (dest.y + y) % 512;
            vram.write(dx, dy, data);
        }
    }
}

void GPU::vram_transfer(uint16_t data) {
    auto& transfer = state.cpu_to_gpu_transfer;

    vram.write(transfer.reg.x + transfer.run.x,
        transfer.reg.y + transfer.run.y, data);

    transfer.run.x++;

    if (transfer.run.x == transfer.reg.w) {
        transfer.run.x = 0;
        transfer.run.y++;

        if (transfer.run.y == transfer.reg.h) {
            transfer.run.y = 0;
            transfer.run.active = false;
        }
    }
}
