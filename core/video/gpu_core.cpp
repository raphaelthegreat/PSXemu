#include <stdafx.hpp>
#include "gpu_core.h"
#include <memory/bus.h>
#include <video/vram.h>
#include <glad/glad.h>

GPU::GPU(Renderer* renderer) :
    gl_renderer(renderer)
{
    status.value = 0x14802000;

    cpu_to_gpu.active = false;
    gpu_to_cpu.active = false;

    current_command = GPUCommand::None;
}

uint GPU::read(uint address) 
{
    uint offset = GPU_RANGE.offset(address);
    
    if (offset == 0)
        return get_gpuread();
    else if (offset == 4)
        return get_gpustat();
    else
        exit(0);     
}

void GPU::write(uint address, uint data) 
{
    uint offset = GPU_RANGE.offset(address);
    
    if (offset == 0)
        return write_gp0(data);
    else if (offset == 4)
        return write_gp1(data);
    else
        exit(0);
}

glm::ivec2 GPU::unpack_point(uint point) 
{
    glm::ivec2 result;
    result.x = util::sclip<11>(point);
    result.y = util::sclip<11>(point >> 16);

    return result;
}

glm::ivec2 GPU::unpack_coord(uint coord)
{
    glm::ivec2 p;
    p.s = (coord >> 0) & 0xff;
    p.t = (coord >> 8) & 0xff;

    return p;
}

GPUSync GPU::get_blanks_and_dot()
{
    GPUSync sync;
    sync.dotDiv = dotClockDiv[status.hres2 << 2 | status.hres1];
    sync.hblank = in_hblank;
    sync.vblank = in_vblank;

    return sync;
}

glm::ivec3 GPU::unpack_color(uint color) 
{
    glm::ivec3 result;
    result.r = (color >> 0) & 0xff;
    result.g = (color >> 8) & 0xff;
    result.b = (color >> 16) & 0xff;

    return result;
}

uint GPU::get_gpuread() 
{
    if (gpu_to_cpu.active) {
        auto lower = vram_transfer();
        auto upper = vram_transfer();
        return (upper << 16) | lower;
    }

    return 0;
}

uint GPU::get_gpustat() 
{
    /*GPUSTAT copy = { status.value };

    copy.reverse_flag = false;
    copy.ready_cmd = true;
    copy.ready_vram = current_command != GPUCommand::Cpu_Vram;
    copy.ready_dma = true;

    return copy.value;*/
    return (status.value & ~0x00080000) | 0x1c002000;
}

ushort GPU::hblank_timings()
{
    if (status.video_mode == VideoMode::NTSC)
        return 3412;
    else
        return 3404;
}

ushort GPU::lines_per_frame()
{
    if (status.video_mode == VideoMode::NTSC)
        return 263;
    else
        return 314;
}

bool GPU::tick(uint cycles)
{
    in_vblank = false; in_hblank = false;

    uint cycles_per_scanline = hblank_timings();
    uint scanlines_per_frame = lines_per_frame();
    bool is_480 = (status.vres == 1);

    /* Add the cycles to GPU pixel clock (this is in pixels). */
    /* NOTE: The GPU clock is the cpu clock * 11/7 */
    gpu_clock += cycles * 11 / 7;

    /* Finished a scanline. */
    if (gpu_clock > cycles_per_scanline) {
        gpu_clock -= cycles_per_scanline;
        in_hblank = true;
        scanline++;

        if (!is_480)
            status.odd_lines = scanline % 2 != 0;

        if (scanline > scanlines_per_frame) {
            scanline = 0;

            if (status.vertical_interlace && is_480)
                status.odd_lines = !status.odd_lines;

            in_vblank = true;
        }
    }

    return in_vblank;
}

ushort GPU::vram_transfer() 
{
    auto& transfer = gpu_to_cpu;
    if (!transfer.active)
        return 0;

    auto data = vram.read(transfer.start_x + transfer.pos_x,
        transfer.start_y + transfer.pos_y);

    transfer.pos_x++;
    if (transfer.pos_x == transfer.width) {
        transfer.pos_x = 0;
        transfer.pos_y++;

        if (transfer.pos_y == transfer.height) {
            transfer.pos_y = 0;
            transfer.active = false;
        }
    }

    return data;
}

void GPU::vram_transfer(ushort data) 
{
    auto& transfer = cpu_to_gpu;
    if (!transfer.active)
        return;

    vram.write(transfer.start_x + transfer.pos_x,
        transfer.start_y + transfer.pos_y, data);

    transfer.pos_x++;
    if (transfer.pos_x == transfer.width) {
        transfer.pos_x = 0;
        transfer.pos_y++;

        if (transfer.pos_y == transfer.height) {
            transfer.pos_y = 0;
            transfer.active = false;

            vram.upload_to_gpu();
        }
    }
}