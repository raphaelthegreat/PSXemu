#pragma once
#include <memory/range.h>

enum TexColors : uint {
    D4bit = 0,
    D8bit = 1,
    D15bit = 2
};

enum Field : uint {
    Top = 1,
    Bottom = 0
};

enum VerticalRes : uint {
    V240 = 0,
    V480 = 1
};

enum VideoMode :uint {
    NTSC = 0,
    PAL = 1
};

enum ColorDepth :uint {
    C15bit = 0,
    C24bit = 1
};

enum DMADirection : uint {
    Off = 0,
    Fifo = 1,
    Cpu_Gpu = 2,
    Vram_Cpu = 3
};

union GPUSTAT {
    uint raw;

    struct {
        uint page_base_x : 4;
        uint page_base_y : 1;
        uint semi_transprency : 2;
        uint texture_depth : 2;
        uint dithering : 1;
        uint draw_to_display : 1;
        uint force_set_mask_bit : 1;
        uint preserve_masked_pixels : 1;
        uint field : 1;
        uint reverse_flag : 1;
        uint texture_disable : 1;               
        union {
            uint hres : 3;

            struct { uint hres2 : 1; uint hres1 : 2; };
        };
        uint vres : 1;
        uint video_mode : 1;
        uint color_depth : 1;
        uint vertical_interlace : 1;
        uint display_disable : 1;
        uint interrupt_req : 1;
        uint dma_request : 1;
        uint ready_cmd : 1;
        uint ready_vram : 1;
        uint ready_dma : 1;
        uint dma_dir : 2;
        uint odd_lines : 1;
    };
};

struct DataMover {
    struct {
        uint x, y;
        uint w, h;
    } reg;

    struct {
        bool active;
        uint x, y;
    } run;
};

struct state_t {
    GPUSTAT status;
   
    ubyte texture_window_mask_x;
    ubyte texture_window_mask_y;
    ubyte texture_window_offset_x;
    ubyte texture_window_offset_y;
    ushort drawing_area_x1;
    ushort drawing_area_y1;
    ushort drawing_area_x2;
    ushort drawing_area_y2;
    int16_t x_offset;
    int16_t y_offset;
    ushort display_area_x;
    ushort display_area_y;
    ushort display_area_x1;
    ushort display_area_y1;
    ushort display_area_x2;
    ushort display_area_y2;
    bool textured_rectangle_x_flip;
    bool textured_rectangle_y_flip;

    std::vector<uint> fifo;

    DataMover cpu_to_gpu_transfer;
    DataMover gpu_to_cpu_transfer;
};

struct Vertex {
    glm::vec3 color;
    glm::vec2 pos;
    glm::vec2 coord;
    glm::vec2 texpage;
    glm::vec2 clut_coord;
    float textured;
    float color_depth;
};

const Range GPU_RANGE = Range(0x1f801810, 8);

class Renderer;
class GPU {
public:
    GPU(Renderer* renderer);
    ~GPU() = default;

    uint read(uint address);
    void write(uint address, uint data);

    glm::vec3 unpack_color(uint color);
    glm::vec2 unpack_point(uint point);
    glm::vec2 unpack_coord(uint coord);
    
    glm::ivec2 get_viewport();
    glm::ivec2 get_draw_offset();
    std::pair<glm::vec2, glm::vec2> get_draw_area();

    bool tick(uint cycles);
    ushort hblank_timings();
    ushort lines_per_frame();

    /* GPU memory read/write commands. */
    void write_gp0(uint data);
    void write_gp1(uint data);
    uint get_gpuread();
    uint get_gpustat();

    /* VRAM transfer commands. */
    void vram_transfer(ushort data);
    ushort vram_transfer();

    /* Drawing commands. */
    void gp0_render_polygon();
    void gp0_render_rect();

    void gp0_nop();
    void gp0_fill_rect();
    void gp0_draw_mode();
    void gp0_draw_area_top_left();
    void gp0_draw_area_bottom_right();
    void gp0_texture_window_setting();
    void gp0_drawing_offset();
    void gp0_mask_bit_setting();
    void gp0_clear_cache();
    void gp0_image_load();
    void gp0_image_store();
    void gp0_image_transfer();

public:
    Renderer* gl_renderer;
    state_t state;

    uint gpu_clock = 0, scanline = 0, frame_count = 0;
    bool in_vblank = false, in_hblank = false;

    int height[2] = { 240, 480 };
    int depth[4] = { 4, 8, 16, 0 };
    int width[7] = { 256, 368, 320, 0, 512, 0, 640 };

    static int command_size[16 * 16];

    /* Temporary data storage. */
    std::vector<Vertex> vdata;
};