#pragma once
#include <video/rasterizer.h>
#include <cpu/utility.h>
#include <memory/range.h>
#include <glm/glm.hpp>
#include <functional>
#include <iostream>
#include <cstdint>
#include <vector>
#include <map>

enum TexColors : uint32_t {
    D4bit = 0,
    D8bit = 1,
    D15bit = 2
};

enum Field : uint32_t {
    Top = 1,
    Bottom = 0
};

enum VerticalRes : uint32_t {
    V240 = 0,
    V480 = 1
};

enum VideoMode :uint32_t {
    NTSC = 0,
    PAL = 1
};

enum ColorDepth :uint32_t {
    C15bit = 0,
    C24bit = 1
};

enum DMADirection : uint32_t {
    Off = 0,
    Fifo = 1,
    Cpu_Gpu = 2,
    Vram_Cpu = 3
};

enum GP0Command {
    Nop = 0x0,
    Clear_Cache = 0x1,
    Fill_Rect = 0x2,
    Mono_Trig = 0x20,
    Mono_Quad = 0x28,
    Mono_Quad_Transparent = 0x2a,
    Shaded_Quad_Blend = 0x2c,
    Shaded_Quad_Raw_Texture = 0x2d,
    Shaded_Quad_Semi_Transparent_Raw_Texture = 0x2f,
    Shaded_Triangle = 0x30,
    Shaded_Quad = 0x38,
    Shaded_Textured_Quad_Blend = 0x3c,
    Mono_Rect = 0x60,
    Textured_Rect_Opaque = 0x65,
    Textured_Rect_Semi_Transparent = 0x66,
    Mono_Quad_Dot = 0x68,
    Mono_Rect_16 = 0x78,
    Textured_Rect_16_Blending = 0x7c,
    Image_Load = 0xa0,
    Image_Store = 0xc0,
    Texture_Window_Setting = 0xe2,
    Draw_Mode_Setting = 0xe1,
    Draw_Area_Top_Left = 0xe3,
    Draw_Area_Bottom_Right = 0xe4,
    Drawing_Offset = 0xe5,
    Mask_Bit_Setting = 0xe6
};

enum GP1Command {
    Reset = 0x0,
    Reset_Cmd_Buffer = 0x01,
    Acknowledge_Irq = 0x02,
    Display_Enable = 0x03,
    DMA_Direction = 0x04,
    Start_Of_Display_Area = 0x05,
    Horizontal_Display_Range = 0x06,
    Vertical_Display_Range = 0x07,
    Display_Mode = 0x08

};

union GPUSTAT {
    uint32_t raw;

    struct {
        uint32_t page_base_x : 4;
        uint32_t page_base_y : 1;
        uint32_t semi_transprency : 2;
        uint32_t texture_depth : 2;
        uint32_t dithering : 1;
        uint32_t draw_to_display : 1;
        uint32_t force_set_mask_bit : 1;
        uint32_t preserve_masked_pixels : 1;
        uint32_t field : 1;
        uint32_t reverse_flag : 1;
        uint32_t texture_disable : 1;
        uint32_t hres : 3;
        uint32_t vres : 1;
        uint32_t video_mode : 1;
        uint32_t color_depth : 1;
        uint32_t vertical_interlace : 1;
        uint32_t display_disable : 1;
        uint32_t interrupt_req : 1;
        uint32_t dma_request : 1;
        uint32_t ready_cmd : 1;
        uint32_t ready_vram : 1;
        uint32_t ready_dma : 1;
        uint32_t dma_dir : 2;
        uint32_t odd_lines : 1;
    };
};

struct DataMover {
    struct {
        uint32_t x, y;
        uint32_t w, h;
    } reg;

    struct {
        bool active;
        uint32_t x, y;
    } run;
};

struct state_t {
    GPUSTAT status;
   
    uint32_t texture_window_mask_x;
    uint32_t texture_window_mask_y;
    uint32_t texture_window_offset_x;
    uint32_t texture_window_offset_y;
    uint32_t drawing_area_x1;
    uint32_t drawing_area_y1;
    uint32_t drawing_area_x2;
    uint32_t drawing_area_y2;
    int16_t x_offset;
    int16_t y_offset;
    uint32_t display_area_x;
    uint32_t display_area_y;
    uint32_t display_area_x1;
    uint32_t display_area_y1;
    uint32_t display_area_x2;
    uint32_t display_area_y2;
    bool textured_rectangle_x_flip;
    bool textured_rectangle_y_flip;

    std::vector<uint32_t> fifo;

    DataMover cpu_to_gpu_transfer;
    DataMover gpu_to_cpu_transfer;
};

typedef glm::ivec3 color_t;
typedef glm::ivec2 point_t;
typedef uint32_t GPURead;
typedef std::function<void()> GPUCommand;

const Range GPU_RANGE = Range(0x1f801810, 8);

class GPU {
public:
    GPU();
    ~GPU() = default;

    uint32_t read(uint32_t address);
    void write(uint32_t address, uint32_t data);

    void register_commands();

    Pixel create_pixel(uint32_t point, uint32_t color, uint32_t coord = 0);
    glm::ivec3 unpack_color(uint32_t color);
    glm::ivec2 unpack_point(uint32_t point);
    uint16_t fetch_texel(glm::ivec2 p, glm::uvec2 uv, glm::uvec2 clut);

    uint32_t data();
    uint32_t stat();

    uint16_t hblank_timings();
    uint16_t lines_per_frame();
    bool tick(uint32_t cycles);

    void gp0(uint32_t data);
    void gp1(uint32_t data);

    void vram_transfer(uint16_t data);
    uint16_t vram_transfer();

    void gp0_nop();
    void gp0_mono_trig();
    void gp0_mono_quad();
    void gp0_pixel();
    void gp0_fill_rect();
    void gp0_draw_mode();
    void gp0_draw_area_top_left();
    void gp0_draw_area_bottom_right();
    void gp0_textured_rect_opaque();
    void gp0_texture_window_setting();
    void gp0_drawing_offset();
    void gp0_mask_bit_setting();
    void gp0_clear_cache();
    void gp0_image_load();
    void gp0_image_store();
    void gp0_textured_rect_16();
    void gp0_mono_rect_16();
    void gp0_mono_rect();
    void gp0_textured_rect_transparent();
    void gp0_shaded_quad();
    void gp0_shaded_quad_blend();
    void gp0_shaded_quad_transparent();
    void gp0_shaded_trig();
    void gp0_shaded_textured_quad_blend();

public:
    Rasterizer raster;
    state_t state;

    std::unordered_map<GP0Command, GPUCommand> gp0_lookup;
    std::unordered_map<GP1Command, GPUCommand> gp1_lookup;

    uint32_t gpu_clock = 0, scanline = 0, frame_count = 0;
    bool in_vblank = false, in_hblank = false;
};