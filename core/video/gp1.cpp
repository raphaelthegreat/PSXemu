#include <stdafx.hpp>
#include "gpu_core.h"
#include <utility/utility.hpp>

void GPU::write_gp1(uint data) 
{    
    uint opcode = (data >> 24) & 0x3f;
    if (opcode == 0) {
        status.value = 0x14802000;
        textured_rectangle_flip = glm::bvec2(false);
    }
    else if (opcode == 1) {
        fifo.clear();
    }
    else if (opcode == 2) {
        status.interrupt_req = false;
    }
    else if (opcode == 3) {
        status.display_disable = (data & 1) != 0;
    }
    else if (opcode == 4) {
        status.dma_dir = data & 0x3;
    }
    else if (opcode == 5) {
        display_area.x = data & 0x3FF;
        display_area.y = (data >> 10) & 0x1FF;
    }
    else if (opcode == 6) {
        display_area_top_left.x = data & 0xFFF;
        display_area_bottom_right.x = (data >> 12) & 0xFFF;
    }
    else if (opcode == 7) {
        display_area_top_left.y = data & 0x3FF;
        display_area_bottom_right.y = (data >> 10) & 0x3FF;
    }
    else if (opcode == 8) {
        status.hres1 = data & 0x3;
        status.vres = util::get_bit(data, 2);
        status.video_mode = util::get_bit(data, 3);
        status.color_depth = util::get_bit(data, 4);
        status.vertical_interlace = (data >> 5) & 1;
        status.hres2 = (data >> 6) & 1;
        status.reverse_flag = (data >> 7) & 1;
        status.field = status.vertical_interlace ? true : false;
    }
    else {
        printf("[GP1] GPU::write_gp1: unhandled command: 0x%0x\n", opcode);
    }
}
