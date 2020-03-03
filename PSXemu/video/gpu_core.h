#pragma once
#include <video/rasterizer.h>
#include <video/gpu_reg.h>
#include <cpu/utility.h>
#include <glm/glm.hpp>
#include <functional>
#include <iostream>
#include <cstdint>
#include <vector>
#include <map>

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
    uint32_t x_offset;
    uint32_t y_offset;
    uint32_t display_area_x;
    uint32_t display_area_y;
    uint32_t display_area_x1;
    uint32_t display_area_y1;
    uint32_t display_area_x2;
    uint32_t display_area_y2;
    bool textured_rectangle_x_flip;
    bool textured_rectangle_y_flip;

    std::vector<uint32_t> fifo;

    struct {
        struct {
            int x;
            int y;
            int w;
            int h;
        } reg;

        struct {
            bool active;
            int x;
            int y;
        } run;
    } cpu_to_gpu_transfer;

    struct {
        struct {
            int x;
            int y;
            int w;
            int h;
        } reg;

        struct {
            bool active;
            int x;
            int y;
        } run;
    } gpu_to_cpu_transfer;
};

typedef glm::ivec3 color_t;
typedef glm::ivec2 point_t;

typedef std::function<void()> GPUCommand;

enum GP0Command {
    Nop = 0x0,
    Clear_Cache = 0x1,
    Fill_Rect = 0x2,
    Mono_Quad = 0x28,
    Shaded_Quad_Blend = 0x2c,
    Shaded_Quad_Raw_Texture = 0x2d,
    Shaded_Triangle = 0x30,
    Shaded_Quad = 0x38,
    Textured_Rect_Opaque = 0x65,
    Mono_Quad_Dot = 0x68,
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

    uint32_t data();
    uint32_t stat();

    void gp0(uint32_t data);
    void gp1(uint32_t data);

    void vram_transfer(uint16_t data);
    uint16_t vram_transfer();

    void gp0_nop();
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
    void gp0_shaded_quad();
    void gp0_shaded_quad_blend();
    void gp0_shaded_trig();

public:
    Rasterizer raster;
    state_t state;

    std::unordered_map<GP0Command, GPUCommand> gp0_lookup;
    std::unordered_map<GP1Command, GPUCommand> gp1_lookup;
};