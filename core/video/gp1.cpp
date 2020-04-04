#include <stdafx.hpp>
#include "gpu_core.h"

void GPU::write_gp1(uint data) {    
    switch ((data >> 24) & 0x3f) {
    case 0x00:
        state.status.raw = 0x14802000;
        state.textured_rectangle_x_flip = 0;
        state.textured_rectangle_y_flip = 0;
        break;

    case 0x01:
        state.fifo.clear();
        break;

    case 0x02:
        state.status.interrupt_req = false;
        break;

    case 0x03:
        state.status.display_disable = (data & 1) != 0;
        break;

    case 0x04:
        state.status.dma_dir = data & 0x3;
        break;

    case 0x05:
        state.display_area_x = (ushort)(data & 0x3FE);
        state.display_area_y = (ushort)((data >> 10) & 0x1FE);
        break;

    case 0x06:
        state.display_area_x1 = (ushort)(data & 0xFFF);
        state.display_area_x2 = (ushort)((data >> 12) & 0xFFF);
        break;

    case 0x07:
        state.display_area_y1 = (ushort)(data & 0x3FF);
        state.display_area_y2 = (ushort)((data >> 10) & 0x3FF);
        break;

    case 0x08:
        state.status.hres1 = data & 0x3;
        state.status.hres2 = (data & 0x40) >> 6;
        state.status.vres = (data & 0x4) != 0;
        state.status.video_mode = (data & 0x8) != 0;
        state.status.color_depth = (data & 0x10) != 0;
        state.status.vertical_interlace = (data & 0x20) != 0;
        state.status.reverse_flag = (data & 0x80) != 0;

        state.status.field = state.status.vertical_interlace ? true : false;
        break;

    default:
        printf("unhandled gp1 command: 0x%08x\n", (data >> 24) & 0x3f);
        break;
    }
}
