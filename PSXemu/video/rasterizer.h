#pragma once
#include <video/math_util.h>
#include <cstdint>

/* A pixel on the GPU. */
struct Pixel {
    /* Vertex */
    glm::ivec2 point;
    /* Color */
    glm::ivec3 color;
    /* Texture coords */
    uint32_t u, v;
};

/* A polygon on the GPU. */
template<uint32_t size>
struct Polygon {
    Pixel point[size];

    /* Polygon attribs */
    uint32_t clut_x, clut_y;
    uint32_t base_u, base_v;
    uint32_t depth;
};

typedef Polygon<3> Triangle;
typedef Polygon<4> Quad;

class GPU;
class Rasterizer {
public:
	Rasterizer(GPU* _gpu);
	~Rasterizer() = default;

    void draw_point(int x, int y, int r, int g, int b);

    void draw_polygon_shaded(const Triangle& p);
    void draw_polygon_shaded(const Quad& q);
    
    void draw_polygon_textured(const Triangle& p);
    void draw_polygon_textured(const Quad& q);

private:
    void fill_polygon_shaded(Pixel v0, Pixel v1, Pixel v2);
    void fill_polygon_textured(const Triangle& t);

    void fill_texture_4bpp(const Triangle& p, glm::ivec3 w, glm::ivec2 pos);
    void fill_texture_15bpp(const Triangle& p, glm::ivec3 w, glm::ivec2 pos);

private:
    GPU* gpu;
};