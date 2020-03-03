#include <cassert>
#include "gpu_core.h"
#include <memory/bus.h>
#include "vram.h"

#define BIND(x) std::bind(&GPU::x, this)

uint32_t GPU::data() {
    if (state.gpu_to_cpu_transfer.run.active) {
        auto lower = vram_transfer();
        auto upper = vram_transfer();
        return (upper << 16) | lower;
    }

    return 0;
}

uint32_t GPU::stat() {
    return (state.status.raw & ~0x00080000) | 0x1c002000;
}

GPU::GPU()
{
    state.status.raw = 0x14802000;
    register_commands();
}

uint32_t GPU::read(uint32_t address) {
    if (address == /*0x1f801810*/0) 
        return data();
   
    if (address == /*0x1f801814*/4) 
        return stat();
}

void GPU::write(uint32_t address, uint32_t data) {
    if (address == 0x1f801810)
        return gp0(data);

    if (address == 0x1f801814)
        return gp1(data);
}

void GPU::register_commands()
{
    /* Create a lookup table with all registered commands. */
    gp0_lookup[Nop] = BIND(gp0_nop);
    gp0_lookup[Clear_Cache] = BIND(gp0_clear_cache);
    gp0_lookup[Fill_Rect] = BIND(gp0_fill_rect);
    gp0_lookup[Mono_Quad] = BIND(gp0_mono_quad);
    gp0_lookup[Shaded_Quad_Blend] = BIND(gp0_shaded_quad_blend);
    gp0_lookup[Shaded_Quad] = BIND(gp0_shaded_quad);
    gp0_lookup[Shaded_Quad_Raw_Texture] = BIND(gp0_shaded_quad_blend);
    gp0_lookup[Shaded_Triangle] = BIND(gp0_shaded_trig);
    gp0_lookup[Mono_Quad_Dot] = BIND(gp0_pixel);
    gp0_lookup[Textured_Rect_Opaque] = BIND(gp0_textured_rect_opaque);
    gp0_lookup[Image_Load] = BIND(gp0_image_load);
    gp0_lookup[Image_Store] = BIND(gp0_image_store);
    gp0_lookup[Texture_Window_Setting] = BIND(gp0_texture_window_setting);
    gp0_lookup[Drawing_Offset] = BIND(gp0_drawing_offset);
    gp0_lookup[Draw_Area_Bottom_Right] = BIND(gp0_draw_area_bottom_right);
    gp0_lookup[Draw_Area_Top_Left] = BIND(gp0_draw_area_top_left);
    gp0_lookup[Draw_Mode_Setting] = BIND(gp0_draw_mode);
    gp0_lookup[Mask_Bit_Setting] = BIND(gp0_mask_bit_setting);
}

uint16_t GPU::vram_transfer() {
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

void GPU::vram_transfer(uint16_t data) {
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
        }
    }
}
