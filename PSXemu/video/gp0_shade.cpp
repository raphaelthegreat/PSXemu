#include "gpu_core.h"
#include "vram.h"

template<int min, int max>
int clip(int value) {
    if (value <= min) return min;
    if (value >= max) return max;
    return value;
};

static int dither_lut[4][4] = {
    { -4,  0, -3,  1 },
    {  2, -2,  3, -1 },
    { -3,  1, -4,  0 },
    {  3, -1,  2, -2 }
};

void gpu::draw_point(int x, int y, int r, int g, int b) {
    auto dither = dither_lut[y & 3][x & 3];

    r = clip<0, 255>(r + dither);
    g = clip<0, 255>(g + dither);
    b = clip<0, 255>(b + dither);

    auto color =
        (((r >> 3) & 0x1f) << 0) |
        (((g >> 3) & 0x1f) << 5) |
        (((b >> 3) & 0x1f) << 10);

    vram.write(x, y, uint16_t(color));
}

void fill_texture_4bpp(const gpu::texture::polygon_t<3>& p,
    const int& w0,
    const int& w1,
    const int& w2, int x, int y) {
    int area = w0 + w1 + w2;

    int u = ((p.v[0].u * w0) + (p.v[1].u * w1) + (p.v[2].u * w2)) / area;
    int v = ((p.v[0].v * w0) + (p.v[1].v * w1) + (p.v[2].v * w2)) / area;

    auto texel = vram.read(p.base_u + (u / 4), p.base_v + v);
    int index = 0;

    switch (u & 3) {
    case 0: index = (texel >> 0) & 0xf; break;
    case 1: index = (texel >> 4) & 0xf; break;
    case 2: index = (texel >> 8) & 0xf; break;
    case 3: index = (texel >> 12) & 0xf; break;
    }

    auto color = vram.read(p.clut_x + index, p.clut_y);
    if (color == 0) {
        return;
    }

    vram.write(x, y, color);
}

void fill_texture_15bpp(const gpu::texture::polygon_t<3>& p,
    const int& w0,
    const int& w1,
    const int& w2, int x, int y) {
    int area = w0 + w1 + w2;

    int u = ((p.v[0].u * w0) + (p.v[1].u * w1) + (p.v[2].u * w2)) / area;
    int v = ((p.v[0].v * w0) + (p.v[1].v * w1) + (p.v[2].v * w2)) / area;

    auto color = vram.read(p.base_u + u, p.base_v + v);
    if (color) {
        vram.write(x, y, color);
    }
}

static int min3(int a, int b, int c) {
    if (a <= b && a <= c) return a;
    if (b <= a && b <= c) return b;
    return c;
}

static int max3(int a, int b, int c) {
    if (a >= b && a >= c) return a;
    if (b >= a && b >= c) return b;
    return c;
}

static void fill_poly3_texture(const gpu::texture::polygon_t<3>& t) {
    int min_x = min3(t.v[0].point.x, t.v[1].point.x, t.v[2].point.x);
    int min_y = min3(t.v[0].point.y, t.v[1].point.y, t.v[2].point.y);
    int max_x = max3(t.v[0].point.x, t.v[1].point.x, t.v[2].point.x);
    int max_y = max3(t.v[0].point.y, t.v[1].point.y, t.v[2].point.y);

    bool is_top_left_12 = is_top_left(t.v[1].point, t.v[2].point);
    bool is_top_left_20 = is_top_left(t.v[2].point, t.v[0].point);
    bool is_top_left_01 = is_top_left(t.v[0].point, t.v[1].point);

    int x01 = t.v[0].point.y - t.v[1].point.y, y01 = t.v[1].point.x - t.v[0].point.x;
    int x12 = t.v[1].point.y - t.v[2].point.y, y12 = t.v[2].point.x - t.v[1].point.x;
    int x20 = t.v[2].point.y - t.v[0].point.y, y20 = t.v[0].point.x - t.v[2].point.x;

    gpu::point_t p = { min_x, min_y };

    int w0_row = edge(t.v[1].point, t.v[2].point, p);
    int w1_row = edge(t.v[2].point, t.v[0].point, p);
    int w2_row = edge(t.v[0].point, t.v[1].point, p);

    for (p.y = min_y; p.y <= max_y; p.y++) {
        int w0 = w0_row;
        int w1 = w1_row;
        int w2 = w2_row;

        for (p.x = min_x; p.x <= max_x; p.x++) {
            bool draw =
                (w0 > 0 || (w0 == 0 && is_top_left_12)) &&
                (w1 > 0 || (w1 == 0 && is_top_left_20)) &&
                (w2 > 0 || (w2 == 0 && is_top_left_01));

            if (draw) {
                switch (t.depth) {
                case 0: fill_texture_4bpp(t, w0, w1, w2, p.x, p.y); break;
                case 2: fill_texture_15bpp(t, w0, w1, w2, p.x, p.y); break;
                case 3: fill_texture_15bpp(t, w0, w1, w2, p.x, p.y); break;
                }
            }

            w0 += x12;
            w1 += x20;
            w2 += x01;
        }

        w0_row += y12;
        w1_row += y20;
        w2_row += y01;
    }
}

void gpu::texture::draw_poly3(const gpu::texture::polygon_t<3>& p) {
    auto& v0 = p.v[0];
    auto& v1 = p.v[1];
    auto& v2 = p.v[2];

    if (double_area(v0.point, v1.point, v2.point) < 0) {
        fill_poly3_texture({ v0, v1, v2, p.clut_x, p.clut_y, p.base_u, p.base_v, p.depth });
    }
    else {
        fill_poly3_texture({ v0, v2, v1, p.clut_x, p.clut_y, p.base_u, p.base_v, p.depth });
    }
}

void gpu::texture::draw_poly4(const gpu::texture::polygon_t<4>& p) {
    auto& v0 = p.v[0];
    auto& v1 = p.v[1];
    auto& v2 = p.v[2];
    auto& v3 = p.v[3];

    gpu::texture::draw_poly3({ v0, v1, v2, p.clut_x, p.clut_y, p.base_u, p.base_v, p.depth });
    gpu::texture::draw_poly3({ v1, v2, v3, p.clut_x, p.clut_y, p.base_u, p.base_v, p.depth });
}

void fill_gouraud(const gpu::gouraud::pixel_t& v0, const int& w0,
    const gpu::gouraud::pixel_t& v1, const int& w1,
    const gpu::gouraud::pixel_t& v2, const int& w2, int x, int y) {
    int area = w0 + w1 + w2;

    int r = ((v0.color.r * w0) + (v1.color.r * w1) + (v2.color.r * w2)) / area;
    int g = ((v0.color.g * w0) + (v1.color.g * w1) + (v2.color.g * w2)) / area;
    int b = ((v0.color.b * w0) + (v1.color.b * w1) + (v2.color.b * w2)) / area;

    gpu::draw_point(x, y, r, g, b);
}

static void fill_poly3_gouraud(const gpu::gouraud::pixel_t& v0, const gpu::gouraud::pixel_t& v1, const gpu::gouraud::pixel_t& v2) {
    int min_x = min3(v0.point.x, v1.point.x, v2.point.x);
    int min_y = min3(v0.point.y, v1.point.y, v2.point.y);
    int max_x = max3(v0.point.x, v1.point.x, v2.point.x);
    int max_y = max3(v0.point.y, v1.point.y, v2.point.y);

    bool is_top_left_12 = is_top_left(v1.point, v2.point);
    bool is_top_left_20 = is_top_left(v2.point, v0.point);
    bool is_top_left_01 = is_top_left(v0.point, v1.point);

    int x01 = v0.point.y - v1.point.y, y01 = v1.point.x - v0.point.x;
    int x12 = v1.point.y - v2.point.y, y12 = v2.point.x - v1.point.x;
    int x20 = v2.point.y - v0.point.y, y20 = v0.point.x - v2.point.x;

    gpu::point_t p = { min_x, min_y };

    int w0_row = edge(v1.point, v2.point, p);
    int w1_row = edge(v2.point, v0.point, p);
    int w2_row = edge(v0.point, v1.point, p);

    for (p.y = min_y; p.y <= max_y; p.y++) {
        int w0 = w0_row;
        w0_row += y12;

        int w1 = w1_row;
        w1_row += y20;

        int w2 = w2_row;
        w2_row += y01;

        for (p.x = min_x; p.x <= max_x; p.x++) {
            bool draw =
                (w0 > 0 || (w0 == 0 && is_top_left_12)) &&
                (w1 > 0 || (w1 == 0 && is_top_left_20)) &&
                (w2 > 0 || (w2 == 0 && is_top_left_01));

            if (draw) {
                fill_gouraud(v0, w0, v1, w1, v2, w2, p.x, p.y);
            }

            w0 += x12;
            w1 += x20;
            w2 += x01;
        }
    }
}

void gpu::gouraud::draw_poly3(const polygon_t<3>& p) {
    const auto& p0 = p.v[0];
    const auto& p1 = p.v[1];
    const auto& p2 = p.v[2];
    
    if (double_area(p0.point, p1.point, p2.point) < 0) {
        fill_poly3_gouraud(p0, p1, p2);
    }
    else {
        fill_poly3_gouraud(p0, p2, p1);
    }
}

void gpu::gouraud::draw_poly4(const polygon_t<4>& p) {
    auto& v0 = p.v[0];
    auto& v1 = p.v[1];
    auto& v2 = p.v[2];
    auto& v3 = p.v[3];

    draw_poly3({ v0, v1, v2 });
    draw_poly3({ v1, v2, v3 });
}
