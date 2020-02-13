#version 330 core
layout (location = 0) in vec2 vpos;
layout (location = 1) in vec3 vcolor;
layout (location = 2) in vec2 coords;

out vec3 color;
out vec2 tex_coords;

uniform ivec2 offset = ivec2(0);

float map(float x, float in_min, float in_max, float out_min, float out_max)
{
	return ((x - in_min) / (in_max - in_min) * (out_max - out_min) + out_min);
}

void main()
{
	vec2 pos = vpos + offset;
	gl_Position = vec4(pos.x, pos.y, 0.0, 1.0);
	
	color = vcolor;
	tex_coords = coords;
}