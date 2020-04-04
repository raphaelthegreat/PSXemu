#version 430 core
layout (location = 0) in vec3 vcolor;
layout (location = 1) in vec2 vpos;
layout (location = 2) in vec2 vcoord;
layout (location = 3) in vec2 vtexpage;
layout (location = 4) in vec2 vclut;
layout (location = 5) in float vtextured;
layout (location = 6) in float vdepth;

out vec3 color;
out vec2 texcoord;
out vec2 texpage;
out vec2 clut;
out float textured;
out float color_depth;

/* Viewport and draw area. */ 
uniform vec2 draw_area1, draw_area2;
uniform vec2 tex_window_offset = vec2(0.0f);
uniform vec2 tex_window_mask = vec2(0.0f);
uniform vec2 draw_offset = vec2(0.0f);
uniform vec2 viewport = vec2(800.0f, 600.0f);

vec2 mask_coord(vec2 input_coord)
{
	ivec2 coord = ivec2(input_coord);
	coord.x &= 255;
	coord.y &= 255;

	ivec2 mask = ivec2(tex_window_mask);
	ivec2 offset = ivec2(tex_window_offset);

    coord.x = (coord.x & ~(mask.x * 8)) | ((offset.x & mask.x) * 8);
    coord.y = (coord.y & ~(mask.y * 8)) | ((offset.y & mask.y) * 8);

    return coord;
}

void main()
{
	/* Transform from 0-640 to 0-1 range. */
	float pos_x = vpos.x / viewport.x * 2 - 1;
	float pos_y = vpos.y / viewport.y * (-2) + 1;

	/* Emit vertex. */
	gl_Position = vec4(pos_x, pos_y, 0.0, 1.0);
	
	/* Send data to the fragment shader. */ 
	color = vcolor / vec3(255.0f);
	texcoord = mask_coord(vcoord);
	texpage = vtexpage;
	clut = vclut;
	textured = vtextured;
	color_depth = vdepth;
}