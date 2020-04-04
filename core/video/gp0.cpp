#include <stdafx.hpp>
#include "gpu_core.h"
#include "renderer.h"
#include "vram.h"

int GPU::command_size[16 * 16] =
{
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

void GPU::write_gp0(uint data) {
    if (state.cpu_to_gpu_transfer.run.active) {
        auto lower = ushort(data >> 0);
        auto upper = ushort(data >> 16);

        vram_transfer(lower);
        vram_transfer(upper);
        return;
    }

    state.fifo.push_back(data);
    uint command = state.fifo[0] >> 24;

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
            gp0_render_polygon();
        else if (command >= 0x40 && command <= 0x5F)
            ; /* Line rendering. */
        else if (command >= 0x60 && command <= 0x7F)
            gp0_render_rect();
        else if (command >= 0x80 && command <= 0x9F)
            gp0_image_transfer();
        else if (command >= 0xA0 && command <= 0xBF)
            gp0_image_load();
        else if (command >= 0xC0 && command <= 0xDF)
            gp0_image_store();
        else {
            printf("[GPU] write_gp0: unknown command: 0x%x\n", command);
            exit(0);
        }

        state.fifo.clear();
    }
}

void GPU::gp0_render_polygon()
{
    auto& shader = gl_renderer->shader;
    auto& fifo = state.fifo;
    auto command = fifo[0];
    auto opcode = command >> 24;

    /* Get command info. */
    bool quad = util::get_bit(command, 27);
    bool shaded = util::get_bit(command, 28);
    bool textured = util::get_bit(command, 26);
    bool mono = (opcode == 0x20 || opcode == 0x22 || opcode == 0x28 || opcode == 0x2A);
    bool semi_transparent = util::get_bit(command, 25);
    bool raw_textured = util::get_bit(command, 24);
    int num_vertices = quad ? 4 : 3;
    int pointer = (mono || !shaded ? 1 : 0);

    auto clut_coord = glm::vec2(0.0f);
    auto texpage = glm::vec2(0.0f);
    ClutAttrib clut = {}; TPageAttrib page = {};
    uint tx = 0, ty = 0, cx = 0, cy = 0;

    if (textured) {
        uint clut_loc = 2;
        uint texpage_loc = (shaded && textured ? 5 : 4);
        
        clut.raw = fifo[clut_loc] >> 16;
        page.raw = fifo[texpage_loc] >> 16;
        
        tx = page.page_x * 64; ty = page.page_y * 256;
        cx = clut.x * 16; cy = clut.y;

        clut_coord = glm::vec2(cx, cy);
        texpage = glm::vec2(tx, ty);
    }

    int color_depth = depth[page.page_colors];
    glm::vec3 base_color = unpack_color(fifo[0]);
    /* Build vertex data array. */
    for (int i = 0; i < num_vertices; i++) {
        Vertex v = {};
        
        if (shaded)
            v.color = unpack_color(fifo[pointer++]);
        else if (mono)
            v.color = base_color;
        else
            v.color = glm::vec3(255.0f);

        auto vertex = fifo[pointer++];
        glm::ivec2 vertex_val = unpack_point(vertex);

        auto vertex_x = std::clamp(vertex_val.x + state.x_offset,
            (int)state.drawing_area_x1,
            (int)state.drawing_area_x2);

        auto vertex_y = std::clamp(vertex_val.y + state.y_offset,
            (int)state.drawing_area_y1,
            (int)state.drawing_area_y2);

        v.pos = glm::vec2(vertex_x, vertex_y);

        if (textured)
            v.coord = unpack_coord(fifo[pointer++]);

        v.texpage = texpage;
        v.clut_coord = clut_coord;
        v.color_depth = color_depth;
        v.textured = (textured ? 1.0f : 0.0f);

        vdata.push_back(v);
    }
    
    if (quad) vdata.insert(vdata.end(), { vdata[1], vdata[2] });

    gl_renderer->draw_call(vdata, Primitive::Polygon);
    vdata.clear();
}

void GPU::gp0_render_rect()
{
    auto& fifo = state.fifo;

    auto command = fifo[0];
    auto opcode = command >> 24;
    int pointer = 0;

    bool textured = util::get_bit(command, 26);
    bool semi_transparent = util::get_bit(command, 25);
    bool raw_textured = util::get_bit(command, 24);

    auto color = unpack_color(fifo[pointer++]);
    if (raw_textured) color = glm::vec3(255.0f); /* For raw textures color is ignored. */

    auto vertex = unpack_point(fifo[pointer++]);
    vertex += get_draw_offset();
    
    auto coord = glm::vec2(0.0f), clut_coord = glm::vec2(0.0f);
    auto texpage = glm::vec2(0.0f);

    ClutAttrib clut = {}; TPageAttrib page = {};
    uint tx = 0, ty = 0, cx = 0, cy = 0;

    if (textured) {
        clut.raw = fifo[2] >> 16;
        page.raw = state.status.raw;
        
        tx = page.page_x * 64; ty = page.page_y * 256;
        cx = clut.x * 16; cy = clut.y;
        
        coord = unpack_coord(fifo[pointer++]);
        clut_coord = glm::vec2(cx, cy);
        texpage = glm::vec2(tx, ty);
    }

    int color_depth = depth[state.status.texture_depth];
    int width = 0, height = 0;
    int dimentions = (opcode & 0x18) >> 3;
    
    /* Get rectangle size. */
    if (dimentions == 0) {
        uint size = state.fifo[pointer++];
        width = size & 0xFFFF;
        height = size >> 16;
    }
    else if (dimentions == 1) {
        width = 1; 
        height = 1;
    }
    else if (dimentions == 2) {
        width = 8;
        height = 8;
    }
    else if (dimentions == 3) {
        width = 16; 
        height = 16;
    }

    glm::vec2 coord1, coord2, coord3, coord4;
    if (textured) {
        coord1 = coord;
        coord2 = coord1 + glm::vec2(width, 0);
        coord3 = coord1 + glm::vec2(0, height);
        coord4 = coord1 + glm::vec2(width, height);
    }
    else {
        coord1 = coord2 = coord3 = coord4 = glm::vec2(0.0f);
    }

    auto vertex1 = vertex;
    auto vertex2 = vertex + glm::vec2(width, 0);
    auto vertex3 = vertex + glm::vec2(0, height);
    auto vertex4 = vertex + glm::vec2(width, height);

    float vtextured = (textured ? 1.0f : 0.0f);
    float vdepth = color_depth;
    vdata = 
    {
       { color, vertex1, coord1, texpage, clut_coord, vtextured, vdepth },
       { color, vertex2, coord2, texpage, clut_coord, vtextured, vdepth },
       { color, vertex3, coord3, texpage, clut_coord, vtextured, vdepth },

       { color, vertex2, coord2, texpage, clut_coord, vtextured, vdepth },
       { color, vertex3, coord3, texpage, clut_coord, vtextured, vdepth },
       { color, vertex4, coord4, texpage, clut_coord, vtextured, vdepth },
    };

    gl_renderer->draw_call(vdata, Primitive::Rectangle);
    vdata.clear();
}

void GPU::gp0_nop()
{
    return;
}

void GPU::gp0_fill_rect()
{
    auto& shader = gl_renderer->shader;
    
    auto color = unpack_color(state.fifo[0]);
    glm::ivec2 point1 = unpack_point(state.fifo[1]);
    glm::ivec2 point2 = unpack_point(state.fifo[2]);

    point1.x = (point1.x + 0x0) & ~0xf;
    point2.x = (point2.x + 0xf) & ~0xf;
    
    auto vertex1 = point1;
    auto vertex2 = vertex1 + glm::ivec2(point2.x, 0);
    auto vertex3 = vertex1 + glm::ivec2(0, point2.y);
    auto vertex4 = vertex1 + point2;

    auto coord = glm::vec2(0.0f);

    vdata =
    {
        { color, vertex1, coord, coord, coord, false, 0.0f },
        { color, vertex2, coord, coord, coord, false, 0.0f },
        { color, vertex3, coord, coord, coord, false, 0.0f },

        { color, vertex2, coord, coord, coord, false, 0.0f },
        { color, vertex3, coord, coord, coord, false, 0.0f },
        { color, vertex4, coord, coord, coord, false, 0.0f },
    };

    gl_renderer->draw_call(vdata, Primitive::Rectangle);
    vdata.clear();
}

void GPU::gp0_draw_mode()
{
    uint val = state.fifo[0];

    state.status.page_base_x = (ubyte)(val & 0xF);
    state.status.page_base_y = (ubyte)((val >> 4) & 0x1);
    state.status.semi_transprency = (ubyte)((val >> 5) & 0x3);
    state.status.texture_depth = (ubyte)((val >> 7) & 0x3);
    state.status.dithering = ((val >> 9) & 0x1) != 0;
    state.status.draw_to_display = ((val >> 10) & 0x1) != 0;
    state.status.texture_disable = ((val >> 11) & 0x1) != 0;
    state.textured_rectangle_x_flip = ((val >> 12) & 0x1) != 0;
    state.textured_rectangle_y_flip = ((val >> 13) & 0x1) != 0;
}

void GPU::gp0_draw_area_top_left()
{
    uint val = state.fifo[0];

    state.drawing_area_x1 = util::uclip<10>(state.fifo[0] >> 0);
    state.drawing_area_y1 = util::uclip<10>(state.fifo[0] >> 10);
}

void GPU::gp0_draw_area_bottom_right()
{
    state.drawing_area_x2 = util::uclip<10>(state.fifo[0] >> 0);
    state.drawing_area_y2 = util::uclip<10>(state.fifo[0] >> 10);
}

void GPU::gp0_texture_window_setting()
{
    auto& shader = gl_renderer->shader;
    uint val = state.fifo[0];

    state.texture_window_mask_x = (ubyte)(val & 0x1F);
    state.texture_window_mask_y = (ubyte)((val >> 5) & 0x1F);
    state.texture_window_offset_x = (ubyte)((val >> 10) & 0x1F);
    state.texture_window_offset_y = (ubyte)((val >> 15) & 0x1F);

    auto tex_window_mask = glm::vec2(state.texture_window_mask_x, 
                                     state.texture_window_mask_y);
    auto tex_window_offset = glm::vec2(state.texture_window_offset_x,
                                       state.texture_window_offset_y);

    shader->bind();
    shader->set_vec2("tex_window_mask", tex_window_mask);
    shader->set_vec2("tex_window_offset", tex_window_offset);
}

void GPU::gp0_drawing_offset()
{
    auto& shader = gl_renderer->shader;
    uint val = state.fifo[0];

    state.x_offset = util::sclip<11>(val & 0x7FF);
    state.y_offset = util::sclip<11>((val >> 11) & 0x7FF);

    auto offset = glm::vec2(state.x_offset, state.y_offset);

    shader->bind();
    shader->set_vec2("draw_offset", offset);
}

void GPU::gp0_mask_bit_setting()
{
    uint val = state.fifo[0];

    state.status.force_set_mask_bit = (val & 1) != 0;
    state.status.preserve_masked_pixels = (val & 2) != 0;
}

void GPU::gp0_clear_cache()
{
    return;
}

void GPU::gp0_image_load()
{
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
    auto& transfer = state.gpu_to_cpu_transfer;
    transfer.reg.x = state.fifo[1] & 0xffff;
    transfer.reg.y = state.fifo[1] >> 16;
    transfer.reg.w = state.fifo[2] & 0xffff;
    transfer.reg.h = state.fifo[2] >> 16;

    transfer.run.x = 0;
    transfer.run.y = 0;
    transfer.run.active = true;
}

void GPU::gp0_image_transfer()
{
    auto src_coord = state.fifo[1];
    auto dest_coord = state.fifo[2];
    auto dim = state.fifo[3];

    glm::ivec2 src = unpack_point(src_coord);
    glm::ivec2 dest = unpack_point(dest_coord);
    glm::ivec2 size = unpack_point(dim);

    for (int y = 0; y < size.y; y++) {
        for (int x = 0; x < size.x; x++) {
            int sx = (src.x + x) % 1024;
            int sy = (src.y + y) % 512;
            auto data = vram.read(sx, sy);

            int dx = (dest.x + x) % 1024;
            int dy = (dest.y + y) % 512;
            vram.write(dx, dy, data);
        }
    }

    vram.upload_to_gpu();
}