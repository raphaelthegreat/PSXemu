#include <cassert>
#include <string>
#include "gpu_core.h"
#include <algorithm>
#include "vram.h"

static short signed11bit(uint32_t n) {
    return (short)(((int)n << 21) >> 21);
}

namespace math {
    template <typename _L, typename _R>
    auto max(const _L& l, const _R& r) { return (l > r ? l : r); }

    template <typename _L, typename _R>
    auto min(const _L& l, const _R& r) { return (l < r ? l : r); }
}

static int command_size[] = {
    //0  1   2   3   4   5   6   7   8   9   A   B   C   D   E   F
     1,  1,  3,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1, //0
     1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1, //1
     4,  4,  4,  4,  7,  7,  7,  7,  5,  5,  5,  5,  9,  9,  9,  9, //2
     6,  6,  6,  6,  9,  9,  9,  9,  8,  8,  8,  8, 12, 12, 12, 12, //3
     3,  3,  3,  3,  3,  3,  3,  3, 16, 16, 16, 16, 16, 16, 16, 16, //4
     4,  4,  4,  4,  4,  4,  4,  4, 16, 16, 16, 16, 16, 16, 16, 16, //5
     3,  3,  3,  1,  4,  4,  4,  4,  2,  1,  2,  1,  3,  3,  3,  3, //6
     2,  1,  2,  1,  3,  3,  3,  3,  2,  1,  2,  2,  3,  3,  3,  3, //7
     4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4, //8
     4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4, //9
     3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, //A
     3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, //B
     3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, //C
     3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, //D
     1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1, //E
     1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1  //F
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
        if (command == 0x00)
            gp0_nop();
        else if (command == 0x01)
            gp0_clear_cache();
        else if (command == 0x02)
            gp0_fill_rect();
        else if (command == 0xE1)
            gp0_draw_mode();
        else if (command == 0xE2)
            gp0_texture_window_setting();
        else if (command == 0xE3)
            gp0_draw_area_top_left();
        else if (command == 0xE4)
            gp0_draw_area_bottom_right();
        else if (command == 0xE5)
            gp0_drawing_offset();
        else if (command == 0xE6)
            gp0_mask_bit_setting();
        else if (command >= 0x20 && command <= 0x3F)
            gp0_shaded_textured_quad_blend();
        else if (command >= 0x60 && command <= 0x7F)
            gp0_textured_rect_16();
        else if (command >= 0x80 && command <= 0x9F)
            vram_to_vram_transfer();
        else if (command >= 0xA0 && command <= 0xBF)
            gp0_image_load();
        else if (command >= 0xC0 && command <= 0xDF)
            gp0_image_store();
        else {
            std::cout << "Unhandled GP0 command: 0x" << std::hex << command << '\n';
            exit(0);
        }

        state.fifo.clear();
    }
}

void GPU::gp0_nop()
{
    //std::cout << "GPU Nop.\n";
    return;
}

void GPU::gp0_mono_trig()
{
    auto color = state.fifo[0];
    auto point1 = state.fifo[1];
    auto point2 = state.fifo[2];
    auto point3 = state.fifo[3];

    auto v0 = create_pixel(point1, color);
    auto v1 = create_pixel(point2, color);
    auto v2 = create_pixel(point3, color);

    Triangle t = { v0, v1, v2 };
    raster.draw_polygon_shaded(t);
}

void GPU::gp0_mono_quad()
{
    //std::cout << "GPU Monochrome quad.\n";
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
    //std::cout << "GPU Draw pixel\n";
    auto color = unpack_color(state.fifo[0]);
    auto point = unpack_point(state.fifo[1]);

    raster.draw_point(point, color);
}

void GPU::gp0_fill_rect()
{
    //std::cout << "GPU Fill rect\n";
    /*auto color = unpack_color(state.fifo[0]);
    auto point1 = unpack_point(state.fifo[1]);
    auto point2 = unpack_point(state.fifo[2]);

    point1.x = (point1.x + 0x0) & ~0xf;
    point2.x = (point2.x + 0xf) & ~0xf;

    for (int y = 0; y < point2.y; y++) {
        for (int x = 0; x < point2.x; x++) {
            auto offset = glm::ivec2(x, y);
            raster.draw_point(point1 + offset, color);
        }
    }*/

    Color8 color0;
    Point2D v[4];
    color0.val = state.fifo[0];
    v[0].val = state.fifo[1];
    v[1].val = state.fifo[2];

    int color = (color0.r << 16 | color0.g << 8 | color0.b);

    for (int yPos = v[0].y; yPos < v[1].y + v[0].y; yPos++) {
        for (int xPos = v[0].x; xPos < v[1].x + v[0].x; xPos++) {
            vram.write((xPos & 0x3FF), (yPos & 0x1FF), color);
        }
    }
}

void GPU::gp0_textured_rect_opaque()
{
    //std::cout << "GPU Textured rect\n";
    auto color = unpack_color(state.fifo[0]);
    auto point1 = unpack_point(state.fifo[1]);
    auto coord = state.fifo[2];
    auto point2 = unpack_point(state.fifo[3]);

    auto base_u = ((state.status.raw >> 0) & 0xf) * 64;
    auto base_v = ((state.status.raw >> 4) & 0x1) * 256;

    auto clut_x = ((coord >> 16) & 0x3f) * 16;
    auto clut_y = ((coord >> 22) & 0x1ff);
    
    for (int y = 0; y < point2.y; y++) {
        for (int x = 0; x < point2.x; x++) {
            auto texel = vram.read(base_u + (x / 4), base_v + y);
            int index = (texel >> 4 * (x & 3)) & 0xf;
            auto color = vram.read(clut_x + index, clut_y);
            vram.write(point1.x + x,
                point1.y + y,
                color);
        }
    }
}

void GPU::gp0_draw_mode()
{
    //std::cout << "GPU Draw mode\n";
    uint32_t val = state.fifo[0];

    state.status.page_base_x = (uint8_t)(val & 0xF);
    state.status.page_base_y = (uint8_t)((val >> 4) & 0x1);
    state.status.semi_transprency = (uint8_t)((val >> 5) & 0x3);
    state.status.texture_depth = (uint8_t)((val >> 7) & 0x3);
    state.status.dithering = ((val >> 9) & 0x1) != 0;
    state.status.draw_to_display = ((val >> 10) & 0x1) != 0;
    state.status.texture_disable = ((val >> 11) & 0x1) != 0;
    state.textured_rectangle_x_flip = ((val >> 12) & 0x1) != 0;
    state.textured_rectangle_y_flip = ((val >> 13) & 0x1) != 0;
}

void GPU::gp0_draw_area_top_left()
{
    //std::cout << "GPU Draw area top left\n";
    uint32_t val = state.fifo[0];

    state.drawing_area_x1 = utility::uclip<10>(state.fifo[0] >> 0);
    state.drawing_area_y1 = utility::uclip<10>(state.fifo[0] >> 10);
}

void GPU::gp0_draw_area_bottom_right()
{
    //std::cout << "GPU Draw area bottom right\n";
    state.drawing_area_x2 = utility::uclip<10>(state.fifo[0] >> 0);
    state.drawing_area_y2 = utility::uclip<10>(state.fifo[0] >> 10);
}

void GPU::gp0_texture_window_setting()
{
    //std::cout << "GPU Texture window setting\n";
    uint32_t val = state.fifo[0];

    state.texture_window_mask_x = (uint8_t)(val & 0x1F);
    state.texture_window_mask_y = (uint8_t)((val >> 5) & 0x1F);
    state.texture_window_offset_x = (uint8_t)((val >> 10) & 0x1F);
    state.texture_window_offset_y = (uint8_t)((val >> 15) & 0x1F);
}

void GPU::gp0_drawing_offset()
{
    //std::cout << "GPU Drawing offset\n";
    uint32_t val = state.fifo[0];

    state.x_offset = signed11bit(val & 0x7FF);
    state.y_offset = signed11bit((val >> 11) & 0x7FF);
}

void GPU::gp0_mask_bit_setting()
{
    //std::cout << "GPU Mask bit setting\n";
    uint32_t val = state.fifo[0];

    state.status.force_set_mask_bit = (val & 1) != 0; ;
    state.status.preserve_masked_pixels = (val & 2) != 0;
}

void GPU::gp0_clear_cache()
{
    //std::cout << "GPU Clear cache\n";
}

void GPU::gp0_image_load()
{
    //std::cout << "GPU Image load\n";
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
    //std::cout << "GPU Image store\n";
    auto& transfer = state.gpu_to_cpu_transfer;
    transfer.reg.x = state.fifo[1] & 0xffff;
    transfer.reg.y = state.fifo[1] >> 16;
    transfer.reg.w = state.fifo[2] & 0xffff;
    transfer.reg.h = state.fifo[2] >> 16;

    transfer.run.x = 0;
    transfer.run.y = 0;
    transfer.run.active = true;
}

void GPU::gp0_textured_rect_16()
{
    uint32_t command = state.fifo[0];
    uint32_t color = command & 0xFFFFFF;
    uint32_t opcode = command >> 24;

    bool isTextured = (command & (1 << 26)) != 0;
    bool isSemiTransparent = (command & (1 << 25)) != 0;
    bool isRawTextured = (command & (1 << 24)) != 0;

    Primitive primitive;
    primitive.isTextured = isTextured;
    primitive.isSemiTransparent = isSemiTransparent;
    primitive.isRawTextured = isRawTextured;

    uint32_t vertex = state.fifo[1];
    short xo = signed11bit(vertex & 0xFFFF);
    short yo = signed11bit(vertex >> 16);

    uint16_t palette = 0;
    uint8_t textureX = 0;
    uint8_t textureY = 0;
    
    if (isTextured) {
        uint32_t texture = state.fifo[2];
        palette = (uint16_t)((texture >> 16) & 0xFFFF);
        textureX = (uint8_t)(texture & 0xFF);
        textureY = (uint8_t)((texture >> 8) & 0xFF);
    }

    short width = 0;
    short heigth = 0;

    switch ((opcode & 0x18) >> 3) {
    case 0x0: {
        uint32_t hw = state.fifo[3];
        width = (short)(hw & 0xFFFF);
        heigth = (short)(hw >> 16);
        break;
    }
    case 0x1: {
        width = 1; heigth = 1;
        break;
    }
    case 0x2: {
        width = 8; heigth = 8;
        break;
    }
    case 0x3: {
        width = 16; heigth = 16;
        break;
    }
    }

    int y = yo + state.y_offset;
    int x = xo + state.x_offset;

    TextureData t[4];
    Point2D v[4];

    v[0].x = (short)x;
    v[0].y = (short)y;

    v[3].x = (short)(x + width);
    v[3].y = (short)(y + heigth);

    t[0].x = textureX;
    t[0].y = textureY;

    uint32_t texpage = 0;

    texpage |= (state.textured_rectangle_y_flip ? 1u : 0) << 13;
    texpage |= (state.textured_rectangle_x_flip ? 1u : 0) << 12;
    texpage |= (state.status.texture_disable ? 1u : 0) << 11;
    texpage |= (state.status.draw_to_display ? 1u : 0) << 10;
    texpage |= (state.status.dithering ? 1u : 0) << 9;
    texpage |= (uint32_t)(state.status.texture_depth << 7);
    texpage |= (uint32_t)(state.status.semi_transprency << 5);
    texpage |= (uint32_t)(state.status.page_base_y << 4);
    texpage |= state.status.page_base_x;

    rasterizeRect(v, t[0], color, palette, texpage, primitive);
}

uint16_t GetPixelBGR555(int x, int y) {
    int color = vram.read(x, y);

    uint8_t m = (uint8_t)((color & 0xFF000000) >> 24);
    uint8_t r = (uint8_t)((color & 0x00FF0000) >> 16 + 3);
    uint8_t g = (uint8_t)((color & 0x0000FF00) >> 8 + 3);
    uint8_t b = (uint8_t)((color & 0x000000FF) >> 3);

    return (uint16_t)(m << 15 | b << 10 | g << 5 | r);
}

int get4bppTexel(int x, int y, Point2D clut, Point2D textureBase) {
    uint16_t index = vram.read(x / 4 + textureBase.x, y + textureBase.y);
    int p = (index >> (x & 3) * 4) & 0xF;
    return vram.read(clut.x + p, clut.y);
}

int get8bppTexel(int x, int y, Point2D clut, Point2D textureBase) {
    uint16_t index = vram.read(x / 2 + textureBase.x, y + textureBase.y);
    int p = (index >> (x & 1) * 8) & 0xFF;
    return vram.read(clut.x + p, clut.y);
}

int get16bppTexel(int x, int y, Point2D textureBase) {
    return vram.read(x + textureBase.x, y + textureBase.y);
}

int GPU::getTexel(int x, int y, Point2D clut, Point2D textureBase, int depth) {
    x &= 255;
    y &= 255;

    auto mask_x = state.texture_window_mask_x;
    auto mask_y = state.texture_window_mask_y;
    auto offset_x = state.texture_window_offset_x;
    auto offset_y = state.texture_window_offset_y;

    x = (x & ~(mask_x * 8)) | ((offset_x & mask_x) * 8);
    y = (y & ~(mask_y * 8)) | ((offset_y & mask_y) * 8);

    if (depth == 0) {
        return get4bppTexel(x, y, clut, textureBase);
    }
    else if (depth == 1) {
        return get8bppTexel(x, y, clut, textureBase);
    }
    else {
        return get16bppTexel(x, y, textureBase);
    }
}

static uint8_t clampToFF(int v) {
    if (v > 0xFF) return 0xFF;
    else return (uint8_t)v;
}

static uint8_t clampToZero(int v) {
    if (v < 0) return 0;
    else return (uint8_t)v;
}

int handleSemiTransp(int x, int y, int color, int semiTranspMode) {
    Color8 color0, color1, color2;
    color0.val = (uint32_t)vram.read(x & 0x3FF, y & 0x1FF); //back
    color1.val = (uint32_t)color; //front
    switch (semiTranspMode) {
    case 0: //0.5 x B + 0.5 x F    ;aka B/2+F/2
        color2.r = (uint8_t)((color0.r + color1.r) >> 1);
        color2.g = (uint8_t)((color0.g + color1.g) >> 1);
        color2.b = (uint8_t)((color0.b + color1.b) >> 1);
        break;
    case 1://1.0 x B + 1.0 x F    ;aka B+F
        color2.r = clampToFF(color0.r + color1.r);
        color2.g = clampToFF(color0.g + color1.g);
        color2.b = clampToFF(color0.b + color1.b);
        break;
    case 2: //1.0 x B - 1.0 x F    ;aka B-F
        color2.r = clampToZero(color0.r - color1.r);
        color2.g = clampToZero(color0.g - color1.g);
        color2.b = clampToZero(color0.b - color1.b);
        break;
    case 3: //1.0 x B +0.25 x F    ;aka B+F/4
        color2.r = clampToFF(color0.r + (color1.r >> 2));
        color2.g = clampToFF(color0.g + (color1.g >> 2));
        color2.b = clampToFF(color0.b + (color1.b >> 2));
        break;
    }//actually doing RGB calcs on BGR struct...
    return (int)color2.val;
}

void GPU::rasterizeRect(Point2D vec[], TextureData t, uint32_t c, uint16_t palette, uint32_t texpage, Primitive primitive) {
    int xOrigin = math::max(vec[0].x, state.drawing_area_x1);
    int yOrigin = math::max(vec[0].y, state.drawing_area_y1);
    int width = math::min(vec[3].x, state.drawing_area_x2);
    int height = math::min(vec[3].y, state.drawing_area_y2);

    int depth = (int)(texpage >> 7) & 0x3;
    int semiTransp = (int)((texpage >> 5) & 0x3);

    Point2D clut;
    clut.x = (short)((palette & 0x3f) << 4);
    clut.y = (short)((palette >> 6) & 0x1FF);

    Point2D textureBase;
    textureBase.x = (short)((texpage & 0xF) << 6);
    textureBase.y = (short)(((texpage >> 4) & 0x1) << 8);

    int uOrigin = t.x + (xOrigin - vec[0].x);
    int vOrigin = t.y + (yOrigin - vec[0].y);

    Color8 color0, color1;
    color0.val = c;
    int baseColor = (color0.m << 24 | color0.r << 16 | color0.g << 8 | color0.b);

    for (int y = yOrigin, v = vOrigin; y < height; y++, v++) {
        for (int x = xOrigin, u = uOrigin; x < width; x++, u++) {
            int color = baseColor;

            if (primitive.isTextured) {
                int texel = getTexel(u, v, clut, textureBase, depth);
                if (texel == 0) {
                    continue;
                }

                if (!primitive.isRawTextured) {
                    color0.val = (uint32_t)color;
                    color1.val = (uint32_t)texel;
                    color1.r = clampToFF(color0.r * color1.r >> 7);
                    color1.g = clampToFF(color0.g * color1.g >> 7);
                    color1.b = clampToFF(color0.b * color1.b >> 7);

                    texel = (int)color1.val;
                }

                color = texel;
            }

            if (primitive.isSemiTransparent && (!primitive.isTextured || (color & 0xFF000000) != 0)) {
                color = handleSemiTransp(x, y, color, semiTransp);
            }

            vram.write(x, y, color);
        }

    }
}

uint16_t GPU::fetch_texel(glm::ivec2 p, glm::uvec2 uv, glm::uvec2 clut)
{
    TexColors depth = (TexColors)state.status.texture_depth;
    
    uint32_t x_mask = state.texture_window_mask_x;
    uint32_t y_mask = state.texture_window_mask_y;
    uint32_t x_offset = state.texture_window_offset_x;
    uint32_t y_offset = state.texture_window_offset_y;

    p.x &= 255;
    p.y &= 255;

    int tx = (p.x & ~(x_mask * 8)) | ((x_offset & x_mask) * 8);
    int ty = (p.y & ~(y_mask * 8)) | ((y_offset & y_mask) * 8);
    
    uint16_t texel = 0;
    if (depth == TexColors::D4bit) {
        auto color = vram.read(uv.x + (tx / 4), uv.y + ty);
        int index = (color >> 4 * (tx & 3)) & 0xf;
        texel = vram.read(clut.x + index, clut.y);
    }
    else if (depth == TexColors::D8bit) {
        auto color = vram.read(uv.x + (tx / 2), uv.y + ty);
        int index = (color >> (tx & 1) * 8) & 0xFF;
        texel = vram.read(clut.x + index, clut.y);
    }
    else {
        texel = vram.read(tx + uv.x, ty + uv.y);
    }

    return texel;
}

void GPU::gp0_mono_rect_16()
{
    //std::cout << "GPU 16x16 rectangle\n";
    auto color = state.fifo[0];
    auto point = state.fifo[1];

    glm::ivec2 vertex = unpack_point(point);
    for (int y = vertex.y; y < vertex.y + 16; y++) {
        for (int x = vertex.x; x < vertex.x + 16; x++) {
            vram.write(x, y, (uint16_t)color);
        }
    }
}

void GPU::gp0_mono_rect()
{
    auto color = state.fifo[0];
    auto point1 = state.fifo[1];
    auto width_height = state.fifo[2];

    auto wh = unpack_point(width_height);
    auto v0 = create_pixel(point1, color);
    Pixel v1 = { v0.point + glm::ivec2(wh.x, 0), v0.color };
    Pixel v2 = { v0.point + glm::ivec2(0, wh.y), v0.color };
    Pixel v3 = { v0.point + glm::ivec2(wh.x, wh.y), v0.color };

    Quad q = { v0, v1, v2, v3 };
    raster.draw_polygon_shaded(q);
}

void GPU::gp0_textured_rect_transparent()
{
    auto color = unpack_color(state.fifo[0]);
    auto point1 = unpack_point(state.fifo[1]);
    auto coord = state.fifo[2];
    auto point2 = unpack_point(state.fifo[3]);

    auto base_u = ((state.status.raw >> 0) & 0xf) * 64;
    auto base_v = ((state.status.raw >> 4) & 0x1) * 256;

    auto clut_x = ((coord >> 16) & 0x3f) * 16;
    auto clut_y = ((coord >> 22) & 0x1ff);

    for (int y = 0; y < point2.y; y++) {
        for (int x = 0; x < point2.x; x++) {
            auto texel = vram.read(base_u + (x / 4), base_v + y);
            int index = (texel >> 4 * (x & 3)) & 0xf;
       
            auto color = vram.read(clut_x + index, clut_y);
            
            if (color != 0) {
                vram.write(point1.x + x,
                    point1.y + y,
                    color);
            }
        }
    }
}

void GPU::gp0_shaded_quad()
{
    //std::cout << "GPU Shaded quad\n";
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
    //std::cout << "GPU Shaded quad blend\n";
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

void GPU::gp0_shaded_quad_transparent()
{
    /*auto color = state.fifo[0];
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

    raster.draw_polygon_textured(q);*/
}

void GPU::gp0_shaded_trig()
{
    //std::cout << "GPU Shaded triangle\n";
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

void GPU::gp0_shaded_textured_quad_blend()
{
    int pointer = 0;
    uint32_t command = state.fifo[pointer];
    //Consoe.WriteLine(command.ToString("x8") +  " "  + state.fifo.Length + " " + pointer);
    bool isQuad = (command & (1 << 27)) != 0;

    bool isShaded = (command & (1 << 28)) != 0;
    bool isTextured = (command & (1 << 26)) != 0;
    bool isSemiTransparent = (command & (1 << 25)) != 0;
    bool isRawTextured = (command & (1 << 24)) != 0;

    Primitive primitive;
    primitive.isShaded = isShaded;
    primitive.isTextured = isTextured;
    primitive.isSemiTransparent = isSemiTransparent;
    primitive.isRawTextured = isRawTextured;

    int vertexN = isQuad ? 4 : 3;

    Point2D* v = new Point2D[vertexN];
    TextureData* t = new TextureData[vertexN];
    uint32_t* c = new uint32_t[vertexN];

    if (!isShaded) {
        uint32_t color = state.fifo[pointer++];
        c[0] = color; //triangle 1 opaque color
        c[1] = color; //triangle 2 opaque color
    }

    uint32_t palette = 0;
    uint32_t texpage = (uint32_t)state.status.semi_transprency << 5;

    for (int i = 0; i < vertexN; i++) {
        if (isShaded) c[i] = state.fifo[pointer++];

        v[i].val = state.fifo[pointer++];
        v[i].x += state.x_offset;
        v[i].y += state.y_offset;

        if (isTextured) {
            uint32_t textureData = state.fifo[pointer++];
            t[i].val = (uint16_t)textureData;
            if (i == 0) {
                palette = textureData >> 16;
            }
            else if (i == 1) {
                texpage = textureData >> 16;
            }
        }
    }    

    rasterizeTri(v[0], v[1], v[2], t[0], t[1], t[2], c[0], c[1], c[2], palette, texpage, primitive);
    if (isQuad) rasterizeTri(v[1], v[2], v[3], t[1], t[2], t[3], c[1], c[2], c[3], palette, texpage, primitive);
}

static bool isTopLeft(Point2D a, Point2D b) {
    return a.y == b.y && b.x > a.x || b.y < a.y;
}

static int orient2d(Point2D a, Point2D b, Point2D c) {
    return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
}

static int getShadedColor(int w0, int w1, int w2, uint32_t c0, uint32_t c1, uint32_t c2, int area) {
    Color8 color0;
    color0.val = c0;
    Color8 color1;
    color1.val = c1;
    Color8 color2;
    color2.val = c2;

    area = w0 + w1 + w2;

    int r = (color0.r * w0 + color1.r * w1 + color2.r * w2) / area;
    int g = (color0.g * w0 + color1.g * w1 + color2.g * w2) / area;
    int b = (color0.b * w0 + color1.b * w1 + color2.b * w2) / area;

    auto color_value =
        (((r >> 3) & 0x1f) << 0) |
        (((g >> 3) & 0x1f) << 5) |
        (((b >> 3) & 0x1f) << 10);

    return color_value;
    //return (r << 16 | g << 8 | b);
}

static int interpolateCoords(int w0, int w1, int w2, int t0, int t1, int t2, int area) {
    //https://codeplea.com/triangular-interpolation
    return (t0 * w0 + t1 * w1 + t2 * w2) / area;
}

void GPU::rasterizeTri(Point2D v0, Point2D v1, Point2D v2, TextureData t0, TextureData t1, TextureData t2, uint32_t c0, uint32_t c1, uint32_t c2, uint32_t palette, uint32_t texpage, Primitive primitive) {

    int area = orient2d(v0, v1, v2);

    if (area == 0) {
        return;
    }

    if (area < 0) {
        //(v1, v2) = (v2, v1);
        //(t1, t2) = (t2, t1);
        //(c1, c2) = (c2, c1);
        std::swap(v1, v2);
        std::swap(t1, t2);
        std::swap(c1, c2);
        area *= -1;
    }

    /*boundingBox*/
    int minX = math::min(v0.x, math::min(v1.x, v2.x));
    int minY = math::min(v0.y, math::min(v1.y, v2.y));
    int maxX = math::max(v0.x, math::max(v1.x, v2.x));
    int maxY = math::max(v0.y, math::max(v1.y, v2.y));

    if ((maxX - minX) > 1024 || (maxY - maxY) > 512) return;

    Point2D min, max;

    /*clip*/
    min.x = (short)math::max(minX, state.drawing_area_x1);
    min.y = (short)math::max(minY, state.drawing_area_y1);
    max.x = (short)math::min(maxX, state.drawing_area_x2);
    max.y = (short)math::min(maxY, state.drawing_area_y2);

    int A01 = v0.y - v1.y, B01 = v1.x - v0.x;
    int A12 = v1.y - v2.y, B12 = v2.x - v1.x;
    int A20 = v2.y - v0.y, B20 = v0.x - v2.x;

    int bias0 = isTopLeft(v1, v2) ? 0 : -1;
    int bias1 = isTopLeft(v2, v0) ? 0 : -1;
    int bias2 = isTopLeft(v0, v1) ? 0 : -1;

    int w0_row = orient2d(v1, v2, min);
    int w1_row = orient2d(v2, v0, min);
    int w2_row = orient2d(v0, v1, min);

    int depth = 0;
    int semiTransp = (int)((texpage >> 5) & 0x3);
    Point2D clut;
    Point2D textureBase;

    if (primitive.isTextured) {
        depth = (int)(texpage >> 7) & 0x3;

        clut.x = (short)((palette & 0x3f) << 4);
        clut.y = (short)((palette >> 6) & 0x1FF);

        textureBase.x = (short)((texpage & 0xF) << 6);
        textureBase.y = (short)(((texpage >> 4) & 0x1) << 8);

        //forceSetE1(texpage);
    }

    auto color = unpack_color(c0);

    auto baseColor =
        (((color.r >> 3) & 0x1f) << 0) |
        (((color.g >> 3) & 0x1f) << 5) |
        (((color.b >> 3) & 0x1f) << 10);

    //TESTING END

    Color8 color0, color1;

    // Rasterize
    for (int y = min.y; y < max.y; y++) {
        // Barycentric coordinates at start of row
        int w0 = w0_row;
        int w1 = w1_row;
        int w2 = w2_row;

        for (int x = min.x; x < max.x; x++) {
            // If p is on or inside all edges, render pixel.
            if ((w0 + bias0 | w1 + bias1 | w2 + bias2) >= 0) {
                //Adjustements per triangle instead of per pixel can be done at area level
                //but it still does some little by 1 error apreciable on some textured quads
                //I assume it could be handled recalculating AXX and BXX offsets but those maths are beyond my scope

                // reset default color of the triangle calculated outside the for as it gets overwriten as follows...
                int color = baseColor;

                if (primitive.isShaded) color = getShadedColor(w0, w1, w2, c0, c1, c2, area);

                if (primitive.isTextured) {
                    int texelX = interpolateCoords(w0, w1, w2, t0.x, t1.x, t2.x, area);
                    int texelY = interpolateCoords(w0, w1, w2, t0.y, t1.y, t2.y, area);
                    int texel = getTexel(texelX, texelY, clut, textureBase, depth);
                    
                    if (texel == 0) {
                        w0 += A12;
                        w1 += A20;
                        w2 += A01;
                        continue;
                    }

                    if (!primitive.isRawTextured) {
                        color0.val = (uint32_t)color;
                        color1.val = (uint32_t)texel;

                        texel = (int)color1.val;
                    }

                    color = texel;
                }

                if (primitive.isSemiTransparent && (!primitive.isTextured || (color & 0xFF000000) != 0)) {
                    color = handleSemiTransp(x, y, color, semiTransp);
                }

                vram.write((x & 0x3FF), (y & 0x1FF), color);
            }
            // One step to the right
            w0 += A12;
            w1 += A20;
            w2 += A01;
        }
        // One row step
        w0_row += B12;
        w1_row += B20;
        w2_row += B01;
    }
    //if (debug) {
    //    //window.update(VRAM.Bits);
    //    Console.ReadLine();
    //}
}
