#include <stdafx.hpp>
#include "gpu_core.h"
#include <memory/bus.h>
#include <video/vram.h>
#include <glad/glad.h>

GPU::GPU(Renderer* renderer) :
    gl_renderer(renderer)
{
    state.status.raw = 0x14802000;
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

glm::vec2 GPU::unpack_point(uint point) 
{
    glm::ivec2 result;
    result.x = util::sclip<11>(point);
    result.y = util::sclip<11>(point >> 16);

    return result;
}

glm::vec2 GPU::unpack_coord(uint coord)
{
    glm::vec2 p;
    p.s = (coord >> 0) & 0xff;
    p.t = (coord >> 8) & 0xff;

    return p;
}

glm::vec3 GPU::unpack_color(uint color) 
{
    glm::vec3 result;
    result.r = (color >> 0) & 0xff;
    result.g = (color >> 8) & 0xff;
    result.b = (color >> 16) & 0xff;

    return result;
}

uint GPU::get_gpuread() 
{
    if (state.gpu_to_cpu_transfer.run.active) {
        auto lower = vram_transfer();
        auto upper = vram_transfer();
        return (upper << 16) | lower;
    }

    return 0;
}

uint GPU::get_gpustat() 
{
    return (state.status.raw & ~0x00080000) | 0x1c002000;
}

ushort GPU::hblank_timings()
{
    if (state.status.video_mode == VideoMode::NTSC)
        return 3412;
    else
        return 3404;
}

ushort GPU::lines_per_frame()
{
    if (state.status.video_mode == VideoMode::NTSC)
        return 263;
    else
        return 314;
}

bool GPU::tick(uint cycles)
{
    in_vblank = false;
    in_hblank = false;

    uint cycles_per_line = hblank_timings();
    uint scanlines_per_frame = lines_per_frame();
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

ushort GPU::vram_transfer() 
{
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

void GPU::vram_transfer(ushort data) 
{
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

            vram.upload_to_gpu();
        }
    }
}

glm::ivec2 GPU::get_viewport()
{
    int w = width[state.status.hres];
    int h = height[state.status.vres];

    return glm::ivec2(w, h);
}

glm::ivec2 GPU::get_draw_offset()
{
    auto offset = glm::ivec2(state.x_offset, state.y_offset);
    return offset;
}

std::pair<glm::vec2, glm::vec2> GPU::get_draw_area()
{
    auto draw_area1 = glm::vec2(state.drawing_area_x1, 
                                state.drawing_area_y1);
    auto draw_area2 = glm::vec2(state.drawing_area_x2, 
                                state.drawing_area_y2);
    
    return std::make_pair(draw_area1, draw_area2);
}
