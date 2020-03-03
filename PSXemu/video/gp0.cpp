#include <cassert>
#include "gpu_core.h"
#include "vram.h"

static int command_size[256] = {
    1, 1, 3, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,
    1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,
    1, 1, 1, 1,  1, 1, 1, 1,  5, 1, 1, 1,  9, 9, 1, 1,
    6, 1, 1, 1,  1, 1, 1, 1,  8, 1, 1, 1,  1, 1, 1, 1,

    1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,
    1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,
    1, 1, 1, 1,  1, 4, 1, 1,  2, 1, 1, 1,  1, 1, 1, 1,
    1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,

    1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,
    1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,
    3, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,
    1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,

    3, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,
    1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,
    1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,
    1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,
};

glm::ivec2 GPU::unpack_point(uint32_t point) {
    glm::ivec2 result;
    result.x = utility::sclip<11>(point);
    result.y = utility::sclip<11>(point >> 16);

    return result;
}

glm::ivec3 GPU::unpack_color(uint32_t color) {
    glm::ivec3 result;
    result.r = (color >> 0) & 0xff;
    result.g = (color >> 8) & 0xff;
    result.b = (color >> 16) & 0xff;

    return result;
}

Pixel GPU::create_pixel(uint32_t point, uint32_t color, uint32_t coord) {
    Pixel p;
    p.point = unpack_point(point);
    p.color = unpack_color(color);

    p.u = (coord >> 0) & 0xff;
    p.v = (coord >> 8) & 0xff;

    return p;
}

void GPU::gp0(uint32_t data) {
    if (state.cpu_to_gpu_transfer.run.active) {
        auto lower = uint16_t(data >> 0);
        auto upper = uint16_t(data >> 16);

        vram_transfer(lower);
        vram_transfer(upper);
        return;
    }

    state.fifo.push_back(data);
    GP0Command command = (GP0Command)(state.fifo[0] >> 24);

    if (state.fifo.size() == command_size[command]) {
        state.fifo.clear();

        auto& handler = gp0_lookup[command];
        handler();        
    }
}

void GPU::gp0_nop()
{
    std::cout << "GPU Nop.\n";
    return;
}

void GPU::gp0_mono_quad()
{
    std::cout << "GPU Monochrome quad.\n";
    auto color = state.fifo[0];
    auto point1 = state.fifo[1];
    auto point2 = state.fifo[2];
    auto point3 = state.fifo[3];
    auto point4 = state.fifo[4];

    auto v0 = create_pixel(point1, color);
    auto v1 = create_pixel(point2, color);
    auto v2 = create_pixel(point3, color);
    auto v3 = create_pixel(point4, color);

    Quad q = { v0, v1, v2, v3 };

    raster.draw_polygon_shaded(q);
}

void GPU::gp0_pixel()
{
    std::cout << "GPU Draw pixel\n";
    auto color = unpack_color(state.fifo[0]);
    auto point = unpack_point(state.fifo[1]);

    raster.draw_point(point, color);
}

void GPU::gp0_fill_rect()
{
    std::cout << "GPU Fill rect\n";
    auto color = unpack_color(state.fifo[0]);
    auto point1 = unpack_point(state.fifo[1]);
    auto point2 = unpack_point(state.fifo[2]);

    point1.x = (point1.x + 0x0) & ~0xf;
    point2.x = (point2.x + 0xf) & ~0xf;

    for (int y = 0; y < point2.y; y++) {
        for (int x = 0; x < point2.x; x++) {
            auto offset = glm::ivec2(x, y);
            raster.draw_point(point1 + offset, color);
        }
    }
}

void GPU::gp0_textured_rect_opaque()
{
    std::cout << "GPU Textured rect\n";
    auto color = unpack_color(state.fifo[0]);
    auto point1 = unpack_point(state.fifo[1]);
    auto coord = state.fifo[2];
    auto point2 = unpack_point(state.fifo[3]);

    auto base_u = ((state.status.raw >> 0) & 0xf) * 64;
    auto base_v = ((state.status.raw >> 4) & 0x1) * 256;

    auto clut_x = ((coord >> 16) & 0x03f) * 16;
    auto clut_y = ((coord >> 22) & 0x1ff);

    for (int y = 0; y < point2.y; y++) {
        for (int x = 0; x < point2.x; x++) {
            auto texel = vram.read(base_u + (x / 4),
                base_v + y);

            int index = 0;

            switch (x & 3) {
            case 0: index = (texel >> 0) & 0xf; break;
            case 1: index = (texel >> 4) & 0xf; break;
            case 2: index = (texel >> 8) & 0xf; break;
            case 3: index = (texel >> 12) & 0xf; break;
            }

            auto color = vram.read(clut_x + index,
                clut_y);

            vram.write(point1.x + x,
                point1.y + y,
                color);
        }
    }
}

void GPU::gp0_draw_mode()
{
    std::cout << "GPU Draw mode\n";
    state.status.raw &= ~0x87ff;
    state.status.raw |= (state.fifo[0] << 0) & 0x7ff;
    state.status.raw |= (state.fifo[0] << 4) & 0x8000;

    state.textured_rectangle_x_flip = ((state.fifo[0] >> 12) & 1) != 0;
    state.textured_rectangle_y_flip = ((state.fifo[0] >> 13) & 1) != 0;
}

void GPU::gp0_draw_area_top_left()
{
    std::cout << "GPU Draw area top left\n";
    state.drawing_area_x1 = utility::uclip<10>(state.fifo[0] >> 0);
    state.drawing_area_y1 = utility::uclip<10>(state.fifo[0] >> 10);
}

void GPU::gp0_draw_area_bottom_right()
{
    std::cout << "GPU Draw area bottom right\n";
    state.drawing_area_x2 = utility::uclip<10>(state.fifo[0] >> 0);
    state.drawing_area_y2 = utility::uclip<10>(state.fifo[0] >> 10);
}

void GPU::gp0_texture_window_setting()
{
    std::cout << "GPU Texture window setting\n";
    state.texture_window_mask_x = utility::uclip<5>(state.fifo[0] >> 0);
    state.texture_window_mask_y = utility::uclip<5>(state.fifo[0] >> 5);
    state.texture_window_offset_x = utility::uclip<5>(state.fifo[0] >> 10);
    state.texture_window_offset_y = utility::uclip<5>(state.fifo[0] >> 15);
}

void GPU::gp0_drawing_offset()
{
    std::cout << "GPU Drawing offset\n";
    state.x_offset = utility::sclip<11>(state.fifo[0] >> 0);
    state.y_offset = utility::sclip<11>(state.fifo[0] >> 11);
}

void GPU::gp0_mask_bit_setting()
{
    std::cout << "GPU Mask bit setting\n";
    state.status.raw &= ~0x1800;
    state.status.raw |= (state.fifo[0] << 11) & 0x1800;
}

void GPU::gp0_clear_cache()
{
    std::cout << "GPU Clear cache\n";
}

void GPU::gp0_image_load()
{
    std::cout << "GPU Image load\n";
    auto& transfer = state.cpu_to_gpu_transfer;
    transfer.reg.x = state.fifo[1] & 0xffff;
    transfer.reg.y = state.fifo[1] >> 16;
    transfer.reg.w = state.fifo[2] & 0xffff;
    transfer.reg.h = state.fifo[2] >> 16;

    transfer.run.x = 0;
    transfer.run.y = 0;
    transfer.run.active = true;
}

void GPU::gp0_image_store()
{
    std::cout << "GPU Image store\n";
    auto& transfer = state.gpu_to_cpu_transfer;
    transfer.reg.x = state.fifo[1] & 0xffff;
    transfer.reg.y = state.fifo[1] >> 16;
    transfer.reg.w = state.fifo[2] & 0xffff;
    transfer.reg.h = state.fifo[2] >> 16;

    transfer.run.x = 0;
    transfer.run.y = 0;
    transfer.run.active = true;
}

void GPU::gp0_shaded_quad()
{
    std::cout << "GPU Shaded quad\n";
    auto color1 = state.fifo[0];
    auto point1 = state.fifo[1];
    auto color2 = state.fifo[2];
    auto point2 = state.fifo[3];
    auto color3 = state.fifo[4];
    auto point3 = state.fifo[5];
    auto color4 = state.fifo[6];
    auto point4 = state.fifo[7];

    auto v0 = create_pixel(point1, color1);
    auto v1 = create_pixel(point2, color2);
    auto v2 = create_pixel(point3, color3);
    auto v3 = create_pixel(point4, color4);

    Quad q = { v0, v1, v2, v3 };

    raster.draw_polygon_shaded(q);
}

void GPU::gp0_shaded_quad_blend()
{
    std::cout << "GPU Shaded quad blend\n";
    auto color = state.fifo[0];
    auto point1 = state.fifo[1];
    auto coord1 = state.fifo[2];
    auto point2 = state.fifo[3];
    auto coord2 = state.fifo[4];
    auto point3 = state.fifo[5];
    auto coord3 = state.fifo[6];
    auto point4 = state.fifo[7];
    auto coord4 = state.fifo[8];

    Quad q;

    q.point[0] = create_pixel(point1, color, coord1);
    q.point[1] = create_pixel(point2, color, coord2);
    q.point[2] = create_pixel(point3, color, coord3);
    q.point[3] = create_pixel(point4, color, coord4);
    q.clut_x = ((coord1 >> 16) & 0x03f) * 16;
    q.clut_y = ((coord1 >> 22) & 0x1ff) * 1;
    q.base_u = ((coord2 >> 16) & 0x00f) * 64;
    q.base_v = ((coord2 >> 20) & 0x001) * 256;
    q.depth = ((coord2 >> 23) & 0x003);

    raster.draw_polygon_textured(q);
}

void GPU::gp0_shaded_trig()
{
    std::cout << "GPU Shaded triangle\n";
    auto color1 = state.fifo[0];
    auto point1 = state.fifo[1];
    auto color2 = state.fifo[2];
    auto point2 = state.fifo[3];
    auto color3 = state.fifo[4];
    auto point3 = state.fifo[5];

    auto v0 = create_pixel(point1, color1);
    auto v1 = create_pixel(point2, color2);
    auto v2 = create_pixel(point3, color3);

    Triangle trig = { v0, v1, v2 };

    raster.draw_polygon_shaded(trig);
}