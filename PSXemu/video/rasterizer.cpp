#include "rasterizer.h"
#include <algorithm>
#include <video/vram.h>
#include <cpu/util.h>

/* Dithering table. */
static int dither_lut[4][4] = {
	{ -4,  0, -3,  1 },
	{  2, -2,  3, -1 },
	{ -3,  1, -4,  0 },
	{  3, -1,  2, -2 }
};

void Rasterizer::draw_point(int x, int y, int r, int g, int b)
{
	/* Apply dithering to the pixel. */
	auto dither = dither_lut[y & 3][x & 3];

	/* Compute color components. */
	r = std::clamp(r + dither, 0, 255);
	g = std::clamp(g + dither, 0, 255);
	b = std::clamp(b + dither, 0, 255);

	/* Calculate the final color. */
	auto color =
		(((r >> 3) & 0x1f) << 0) |
		(((g >> 3) & 0x1f) << 5) |
		(((b >> 3) & 0x1f) << 10);

	/* Write it to the vram framebuffer. */
	vram.write(x, y, uint16_t(color));
}
void Rasterizer::draw_polygon_shaded(const Triangle& p)
{
	const auto& p0 = p.point[0];
	const auto& p1 = p.point[1];
	const auto& p2 = p.point[2];

	int area = double_area(p0.point, p1.point, p2.point);

	if (area < 0)
		fill_polygon_shaded(p0, p1, p2);
	else
		fill_polygon_shaded(p0, p2, p1);
}

void Rasterizer::draw_polygon_shaded(const Quad& q)
{
	/* Split the quad to triangles. */
	Triangle tr1 = { q.point[0], q.point[1], q.point[2] };
	Triangle tr2 = { q.point[1], q.point[2], q.point[3] };

	/* Draw the triangles. */
	draw_polygon_shaded(tr1);
	draw_polygon_shaded(tr2);
}

void Rasterizer::draw_polygon_textured(const Triangle& p)
{
	auto& v0 = p.point[0];
	auto& v1 = p.point[1];
	auto& v2 = p.point[2];

	if (double_area(v0.point, v1.point, v2.point) < 0) {
		fill_polygon_textured({ v0, v1, v2, p.clut_x, p.clut_y, p.base_u, p.base_v, p.depth });
	}
	else {
		fill_polygon_textured({ v0, v2, v1, p.clut_x, p.clut_y, p.base_u, p.base_v, p.depth });
	}
}

void Rasterizer::draw_polygon_textured(const Quad& q)
{
	auto& v0 = q.point[0];
	auto& v1 = q.point[1];
	auto& v2 = q.point[2];
	auto& v3 = q.point[3];

	Triangle tr1 = { v0, v1, v2, q.clut_x, q.clut_y, q.base_u, q.base_v, q.depth };
	Triangle tr2 = { v1, v2, v3, q.clut_x, q.clut_y, q.base_u, q.base_v, q.depth };

	draw_polygon_textured(tr1);
	draw_polygon_textured(tr2);
}

void Rasterizer::fill_polygon_shaded(Pixel v0, Pixel v1, Pixel v2)
{
	int min_x = min(v0.point.x, v1.point.x, v2.point.x);
	int min_y = min(v0.point.y, v1.point.y, v2.point.y);
	int max_x = max(v0.point.x, v1.point.x, v2.point.x);
	int max_y = max(v0.point.y, v1.point.y, v2.point.y);

	bool is_top_left_12 = is_top_left(v1.point, v2.point);
	bool is_top_left_20 = is_top_left(v2.point, v0.point);
	bool is_top_left_01 = is_top_left(v0.point, v1.point);

	int x01 = v0.point.y - v1.point.y, y01 = v1.point.x - v0.point.x;
	int x12 = v1.point.y - v2.point.y, y12 = v2.point.x - v1.point.x;
	int x20 = v2.point.y - v0.point.y, y20 = v0.point.x - v2.point.x;

	glm::ivec2 p = { min_x, min_y };

	int w0_row = edge(v1.point, v2.point, p);
	int w1_row = edge(v2.point, v0.point, p);
	int w2_row = edge(v0.point, v1.point, p);

	for (p.y = min_y; p.y <= max_y; p.y++) {
		int w0 = w0_row; w0_row += y12;
		int w1 = w1_row; w1_row += y20;
		int w2 = w2_row; w2_row += y01;

		for (p.x = min_x; p.x <= max_x; p.x++) {
			bool should_draw =
				(w0 > 0 || (w0 == 0 && is_top_left_12)) &&
				(w1 > 0 || (w1 == 0 && is_top_left_20)) &&
				(w2 > 0 || (w2 == 0 && is_top_left_01));

			if (should_draw) {
				int area = w0 + w1 + w2;

				int r = ((v0.color.r * w0) + (v1.color.r * w1) + (v2.color.r * w2)) / area;
				int g = ((v0.color.g * w0) + (v1.color.g * w1) + (v2.color.g * w2)) / area;
				int b = ((v0.color.b * w0) + (v1.color.b * w1) + (v2.color.b * w2)) / area;

				draw_point(p.x, p.y, r, g, b);
			}

			w0 += x12;
			w1 += x20;
			w2 += x01;
		}
	}
}

void Rasterizer::fill_polygon_textured(const Triangle& t)
{
	int min_x = min(t.point[0].point.x, t.point[1].point.x, t.point[2].point.x);
	int min_y = min(t.point[0].point.y, t.point[1].point.y, t.point[2].point.y);
	int max_x = max(t.point[0].point.x, t.point[1].point.x, t.point[2].point.x);
	int max_y = max(t.point[0].point.y, t.point[1].point.y, t.point[2].point.y);

	bool is_top_left_12 = is_top_left(t.point[1].point, t.point[2].point);
	bool is_top_left_20 = is_top_left(t.point[2].point, t.point[0].point);
	bool is_top_left_01 = is_top_left(t.point[0].point, t.point[1].point);

	int x01 = t.point[0].point.y - t.point[1].point.y, y01 = t.point[1].point.x - t.point[0].point.x;
	int x12 = t.point[1].point.y - t.point[2].point.y, y12 = t.point[2].point.x - t.point[1].point.x;
	int x20 = t.point[2].point.y - t.point[0].point.y, y20 = t.point[0].point.x - t.point[2].point.x;

	glm::ivec2 p = { min_x, min_y };

	int w0_row = edge(t.point[1].point, t.point[2].point, p);
	int w1_row = edge(t.point[2].point, t.point[0].point, p);
	int w2_row = edge(t.point[0].point, t.point[1].point, p);

	for (p.y = min_y; p.y <= max_y; p.y++) {
		int w0 = w0_row;
		int w1 = w1_row;
		int w2 = w2_row;

		for (p.x = min_x; p.x <= max_x; p.x++) {
			bool should_draw =
				(w0 > 0 || (w0 == 0 && is_top_left_12)) &&
				(w1 > 0 || (w1 == 0 && is_top_left_20)) &&
				(w2 > 0 || (w2 == 0 && is_top_left_01));

			if (should_draw) {
				glm::ivec3 w = glm::ivec3(w0, w1, w2);
				switch (t.depth) {
				case 0:
					fill_texture_4bpp(t, w, p);
					break;
				case 2:
					fill_texture_15bpp(t, w, p);
					break;
				case 3:
					fill_texture_15bpp(t, w, p);
					break;
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

void Rasterizer::fill_texture_4bpp(const Triangle& p, glm::ivec3 w, glm::ivec2 pos)
{
	int area = w.x + w.y + w.z;

	int u = ((p.point[0].u * w.x) + (p.point[1].u * w.y) + (p.point[2].u * w.z)) / area;
	int v = ((p.point[0].v * w.x) + (p.point[1].v * w.y) + (p.point[2].v * w.z)) / area;

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

	vram.write(pos.x, pos.y, color);
}

void Rasterizer::fill_texture_15bpp(const Triangle& p, glm::ivec3 w, glm::ivec2 pos)
{
	int area = w.x + w.y + w.z;

	int u = ((p.point[0].u * w.x) + (p.point[1].u * w.y) + (p.point[2].u * w.z)) / area;
	int v = ((p.point[0].v * w.x) + (p.point[1].v * w.y) + (p.point[2].v * w.z)) / area;

	auto color = vram.read(p.base_u + u, p.base_v + v);
	if (color) {
		vram.write(pos.x, pos.y, color);
	}
}