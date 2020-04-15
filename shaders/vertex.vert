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

void main()
{
	/* Transform from 0-640 to 0-1 range. */
	float pos_x = vpos.x / 1024.0f * 2 - 1;
	float pos_y = vpos.y / 512.0f * (-2) + 1;

	/* Emit vertex. */
	gl_Position = vec4(pos_x, pos_y, 0.0, 1.0);
	
	/* Send data to the fragment shader. */ 
	color = vcolor / vec3(255.0f);
	texcoord = vcoord;
	texpage = vtexpage;
	clut = vclut;
	textured = vtextured;
	color_depth = vdepth;
}