#include <stdafx.hpp>
#include "gpu_core.h"
#include <glad/glad.h>
#include <video/renderer.h>
#include <utility/utility.hpp>
#include <video/vram.h>

/* Lookup table that tells us the number of attributes in a specific command. */
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
    /* If a transfer is pending ignore command. */
    if (cpu_to_gpu.active) {
        auto lower = ushort(data >> 0);
        auto upper = ushort(data >> 16);

        vram_transfer(lower);
        vram_transfer(upper);
        return;
    }

    /* Push command word in the fifo. */
    fifo.push_back(data);
    uint command = fifo[0] >> 24;

    /* If the command is complete, execute it. */
    if (fifo.size() == command_size[command]) {
        if (command == 0x00) {
            gp0_nop();
            current_command = GPUCommand::Nop;
        }
        else if (command == 0x01) {
            gp0_clear_cache();
            current_command = GPUCommand::Render_Attrib;
        }
        else if (command == 0x02) {
            gp0_fill_rect();
            current_command = GPUCommand::Fill_Rectangle;
        }
        else if (command >= 0x20 && command <= 0x3F) {
            gp0_render_polygon();
            current_command = GPUCommand::Polygon;
        }
        else if (command >= 0x40 && command <= 0x5F) {
            current_command = GPUCommand::Line;
        }
        else if (command >= 0x60 && command <= 0x7F) {
            gp0_render_rect();
            current_command = GPUCommand::Rectangle;
        }
        else if (command >= 0x80 && command <= 0x9F) {
            gp0_image_transfer();
            current_command = GPUCommand::Vram_Vram;
        }
        else if (command >= 0xA0 && command <= 0xBF) {
            gp0_image_load();
            current_command = GPUCommand::Cpu_Vram;
        }
        else if (command >= 0xC0 && command <= 0xDF) {
            gp0_image_store();
            current_command = GPUCommand::Vram_Cpu;
        }
        else if (command == 0xE1) {
            gp0_draw_mode();
            current_command = GPUCommand::Render_Attrib;
        }
        else if (command == 0xE2) {
            gp0_texture_window_setting();
            current_command = GPUCommand::Render_Attrib;
        }
        else if (command == 0xE3) {
            gp0_draw_area_top_left();
            current_command = GPUCommand::Render_Attrib;
        }
        else if (command == 0xE4) {
            gp0_draw_area_bottom_right();
            current_command = GPUCommand::Render_Attrib;
        }
        else if (command == 0xE5) {
            gp0_drawing_offset();
            current_command = GPUCommand::Render_Attrib;
        }
        else if (command == 0xE6) {
            gp0_mask_bit_setting();
            current_command = GPUCommand::Render_Attrib;
        }
        else {
            printf("[GPU] write_gp0: unknown command: 0x%x\n", command);
            exit(1);
        }

        /* Do not forget to clear the fifo! */
        fifo.clear();
    }
}

/* Renders a polygon to the framebuffer. */
void GPU::gp0_render_polygon()
{
    auto& shader = gl_renderer->shader;
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

        glm::i16vec2 vertex = unpack_point(fifo[pointer++]);
        v.pos = vertex + draw_offset;

        if (textured)
            v.coord = unpack_coord(fifo[pointer++]);

        v.texpage = texpage;
        v.clut_coord = clut_coord;
        v.color_depth = (float)color_depth;
        v.textured = (textured ? 1.0f : 0.0f);

        vdata.push_back(v);
    }
    
    if (quad) vdata.insert(vdata.end(), { vdata[1], vdata[2] });

    gl_renderer->draw_call(vdata, Primitive::Polygon);
    vdata.clear();
}

/* Renders a rectangle to the framebuffer. */
void GPU::gp0_render_rect()
{
    auto command = fifo[0];
    auto opcode = command >> 24;
    int pointer = 0;

    bool textured = util::get_bit(command, 26);
    bool semi_transparent = util::get_bit(command, 25);
    bool raw_textured = util::get_bit(command, 24);

    auto color = unpack_color(fifo[pointer++]);
    if (raw_textured) color = glm::vec3(255.0f); /* For raw textures color is ignored. */

    auto point = unpack_point(fifo[pointer++]);
    point += draw_offset;
    
    auto texcoord = glm::ivec2(0), clut_coord = glm::ivec2(0);
    auto texpage = glm::ivec2(0);

    ClutAttrib clut = {}; TPageAttrib page = {};
    uint tx = 0, ty = 0, cx = 0, cy = 0;

    if (textured) {
        clut.raw = fifo[2] >> 16;
        page.raw = status.value;
        
        tx = page.page_x * 64; ty = page.page_y * 256;
        cx = clut.x * 16; cy = clut.y;
        
        texcoord = unpack_coord(fifo[pointer++]);
        clut_coord = glm::vec2(cx, cy);
        texpage = glm::vec2(tx, ty);
    }

    int color_depth = depth[status.texture_depth];
    int width = 0, height = 0;
    int dimentions = (opcode & 0x18) >> 3;
    
    /* Get rectangle size. */
    if (dimentions == 0) {
        uint size = fifo[pointer++];
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

    glm::ivec2 points[4] =
    {
        glm::ivec2(0), glm::ivec2(width, 0),
        glm::ivec2(0, height), glm::ivec2(width, height)
    };

    /* Generate vertex data. */
    for (auto& p : points) {
        auto vertex = point + p;
        auto coord = texcoord + p;

        Vertex v;
        v.color = color;
        v.pos = vertex;
        v.coord = coord;
        v.texpage = texpage;
        v.clut_coord = clut_coord;
        v.textured = (textured ? 1.0f : 0.0f);
        v.color_depth = (float)color_depth;

        vdata.push_back(v);
    }

    /* Complete the quad. */
    vdata.insert(vdata.end(), { vdata[1], vdata[2] });

    /* Batch the draw data/ */
    gl_renderer->draw_call(vdata, Primitive::Rectangle);
    vdata.clear();
}

/* Does nothing. */
void GPU::gp0_nop()
{
    return;
}

/*GP0(02h) - Fill Rectangle in VRAM
  1st  Color+Command     (CcBbGgRrh)  ;24bit RGB value (see note)
  2nd  Top Left Corner   (YyyyXxxxh)  ;Xpos counted in halfwords, steps of 10h
  3rd  Width+Height      (YsizXsizh)  ;Xsiz counted in halfwords, steps of 10h
Fills the area in the frame buffer with the value in RGB. Horizontally the filling is done in 16-pixel (32-bytes) units (see below masking/rounding).
The "Color" parameter is a 24bit RGB value, however, the actual fill data is 16bit: The hardware automatically converts the 24bit RGB value to 15bit RGB (with bit15=0).
Fill is NOT affected by the Mask settings (acts as if Mask.Bit0,1 are both zero).*/
void GPU::gp0_fill_rect()
{
    auto color = unpack_color(fifo[0]);
    auto top_left = unpack_point(fifo[1]);
    auto size = unpack_point(fifo[2]);

    glm::ivec2 points[4] =
    {
        glm::ivec2(0), glm::ivec2(size.x, 0),
        glm::ivec2(0, size.y), size
    };

    /* Generate vertex data. */
    for (auto& p : points) {
        auto vertex = top_left + p;

        Vertex v = {};
        v.color = color;
        v.pos = vertex;
        v.textured = false;

        vdata.push_back(v);
    }

    /* Complete the quad. */
    vdata.insert(vdata.end(), { vdata[1], vdata[2] });

    /* Force draw. */
    /* NOTE: this done as fill commands ignore all */
    /* mask settings that the batch renderer uses. */
    gl_renderer->draw(vdata);
    vdata.clear();
}

/*GP0(E1h) - Draw Mode setting (aka "Texpage")
  0-3   Texture page X Base   (N*64) (ie. in 64-halfword steps)    ;GPUSTAT.0-3
  4     Texture page Y Base   (N*256) (ie. 0 or 256)               ;GPUSTAT.4
  5-6   Semi Transparency     (0=B/2+F/2, 1=B+F, 2=B-F, 3=B+F/4)   ;GPUSTAT.5-6
  7-8   Texture page colors   (0=4bit, 1=8bit, 2=15bit, 3=Reserved);GPUSTAT.7-8
  9     Dither 24bit to 15bit (0=Off/strip LSBs, 1=Dither Enabled) ;GPUSTAT.9
  10    Drawing to display area (0=Prohibited, 1=Allowed)          ;GPUSTAT.10
  11    Texture Disable (0=Normal, 1=Disable if GP1(09h).Bit0=1)   ;GPUSTAT.15
          (Above might be chipselect for (absent) second VRAM chip?)
  12    Textured Rectangle X-Flip   (BIOS does set this bit on power-up...?)
  13    Textured Rectangle Y-Flip   (BIOS does set it equal to GPUSTAT.13...?)
  14-23 Not used (should be 0)
  24-31 Command  (E1h)
The GP0(E1h) command is required only for Lines, Rectangle, and Untextured-Polygons (for Textured-Polygons, the data is specified in form of the Texpage attribute; 
except that, Bit9-10 can be changed only via GP0(E1h), not via the Texpage attribute).
Texture page colors setting 3 (reserved) is same as setting 2 (15bit).
Note: GP0(00h) seems to be often inserted between Texpage and Rectangle commands, maybe it acts as a NOP, which may be required between that commands, for timing reasons...?*/
void GPU::gp0_draw_mode()
{
    uint val = fifo[0];

    status.page_base_x = (ubyte)(val & 0xF);
    status.page_base_y = (ubyte)((val >> 4) & 0x1);
    status.semi_transprency = (ubyte)((val >> 5) & 0x3);
    status.texture_depth = (ubyte)((val >> 7) & 0x3);
    status.dithering = ((val >> 9) & 0x1) != 0;
    status.draw_to_display = ((val >> 10) & 0x1) != 0;
    status.texture_disable = ((val >> 11) & 0x1) != 0;
    textured_rectangle_flip.x = ((val >> 12) & 0x1) != 0;
    textured_rectangle_flip.y = ((val >> 13) & 0x1) != 0;
}

/*GP0(E3h) - Set Drawing Area top left (X1,Y1)
GP0(E4h) - Set Drawing Area bottom right (X2,Y2)
  0-9    X-coordinate (0..1023)
  10-18  Y-coordinate (0..511)   ;\on Old 160pin GPU (max 1MB VRAM)
  19-23  Not used (zero)         ;/
  10-19  Y-coordinate (0..1023)  ;\on New 208pin GPU (max 2MB VRAM)
  20-23  Not used (zero)         ;/(retail consoles have only 1MB though)
  24-31  Command  (Exh)
Sets the drawing area corners. The Render commands GP0(20h..7Fh) are automatically clipping any pixels that are outside of this region.*/
void GPU::gp0_draw_area_top_left()
{
    drawing_area_top_left.x = util::uclip<10>(fifo[0] >> 0);
    drawing_area_top_left.y = util::uclip<10>(fifo[0] >> 10);
}

void GPU::gp0_draw_area_bottom_right()
{
    drawing_area_bottom_right.x = util::uclip<10>(fifo[0] >> 0);
    drawing_area_bottom_right.y = util::uclip<10>(fifo[0] >> 10);
}

/*GP0(E2h) - Texture Window setting
  0-4    Texture window Mask X   (in 8 pixel steps)
  5-9    Texture window Mask Y   (in 8 pixel steps)
  10-14  Texture window Offset X (in 8 pixel steps)
  15-19  Texture window Offset Y (in 8 pixel steps)
  20-23  Not used (zero)
  24-31  Command  (E2h)
Mask specifies the bits that are to be manipulated, and Offset contains the new values for these bits, ie. texture X/Y coordinates are adjusted as so:
  Texcoord = (Texcoord AND (NOT (Mask*8))) OR ((Offset AND Mask)*8)
The area within a texture window is repeated throughout the texture page. */
void GPU::gp0_texture_window_setting()
{
    texture_window_mask.x = fifo[0] & 0x1F;
    texture_window_mask.y = (fifo[0] >> 5) & 0x1F;
    texture_window_offset.x = (fifo[0] >> 10) & 0x1F;
    texture_window_offset.y = (fifo[0] >> 15) & 0x1F;
}

/*GP0(E5h) - Set Drawing Offset (X,Y)
  0-10   X-offset (-1024..+1023) (usually within X1,X2 of Drawing Area)
  11-21  Y-offset (-1024..+1023) (usually within Y1,Y2 of Drawing Area)
  22-23  Not used (zero)
  24-31  Command  (E5h)
If you have configured the GTE to produce vertices with coordinate "0,0" being located in the center of the drawing area, 
then the Drawing Offset must be "X1+(X2-X1)/2, Y1+(Y2-Y1)/2". 
Or, if coordinate "0,0" shall be the upper-left of the Drawing Area, then Drawing Offset should be "X1,Y1". 
Where X1,Y1,X2,Y2 are the values defined with GP0(E3h-E4h).*/
void GPU::gp0_drawing_offset()
{
    draw_offset.x = util::sclip<11>(fifo[0] >> 0);
    draw_offset.y = util::sclip<11>(fifo[0] >> 11);
}

void GPU::gp0_mask_bit_setting()
{
    status.force_set_mask_bit = (fifo[0] & 1) != 0;
    status.preserve_masked_pixels = (fifo[0] & 2) != 0;
}

void GPU::gp0_clear_cache()
{
    return;
}

/*GP0(A0h) - Copy Rectangle (CPU to VRAM)
  1st  Command           (Cc000000h)
  2nd  Destination Coord (YyyyXxxxh)  ;Xpos counted in halfwords
  3rd  Width+Height      (YsizXsizh)  ;Xsiz counted in halfwords
  ...  Data              (...)      <--- usually transferred via DMA
Transfers data from CPU to frame buffer. 
The transfer is affected by Mask setting.*/
void GPU::gp0_image_load()
{
    auto& transfer = cpu_to_gpu;
    transfer.start_x = fifo[1] & 0xffff;
    transfer.start_y = fifo[1] >> 16;
    transfer.width = fifo[2] & 0xffff;
    transfer.height = fifo[2] >> 16;

    transfer.pos_x = 0;
    transfer.pos_y = 0;
    transfer.active = true;
}

/*GP0(80h) - Copy Rectangle (VRAM to VRAM)
  1st  Command           (Cc000000h)
  2nd  Source Coord      (YyyyXxxxh)  ;Xpos counted in halfwords
  3rd  Destination Coord (YyyyXxxxh)  ;Xpos counted in halfwords
  4th  Width+Height      (YsizXsizh)  ;Xsiz counted in halfwords
Copys data within framebuffer. The transfer is affected by Mask setting.*/
void GPU::gp0_image_store()
{
    auto& transfer = gpu_to_cpu;
    transfer.start_x = fifo[1] & 0xffff;
    transfer.start_y = fifo[1] >> 16;
    transfer.width = fifo[2] & 0xffff;
    transfer.height = fifo[2] >> 16;

    transfer.pos_x = 0;
    transfer.pos_y = 0;
    transfer.active = true;
}

/*GP0(80h) - Copy Rectangle (VRAM to VRAM)
  1st  Command           (Cc000000h)
  2nd  Source Coord      (YyyyXxxxh)  ;Xpos counted in halfwords
  3rd  Destination Coord (YyyyXxxxh)  ;Xpos counted in halfwords
  4th  Width+Height      (YsizXsizh)  ;Xsiz counted in halfwords
Copys data within framebuffer. The transfer is affected by Mask setting.*/
void GPU::gp0_image_transfer()
{
    auto src = unpack_point(fifo[1]);
    auto dest = unpack_point(fifo[2]);
    auto size = unpack_point(fifo[3]);

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