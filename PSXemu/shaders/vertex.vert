#version 430 core
layout (location = 0) in vec2 vpos;
layout (location = 1) in vec2 coords;

out vec2 tex_coords;

uniform ivec2 offset = ivec2(0);

void main()
{
	vec2 pos = vpos + offset;
	
	gl_Position = vec4(pos.x, pos.y, 0.0, 1.0);
	tex_coords = coords;
}