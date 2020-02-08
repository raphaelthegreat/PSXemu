#version 330 core
layout (location = 0) in ivec2 vpos;
layout (location = 1) in uvec3 vcolor;
out vec3 color;

uniform ivec2 offset = ivec2(0);

float map(float x, float in_min, float in_max, float out_min, float out_max)
{
	return ((x - in_min) / (in_max - in_min) * (out_max - out_min) + out_min);
}

void main()
{
	ivec2 pos = vpos + offset;
	float xpos = map(pos.x, 0, 640, -1, 1);
	float ypos = map(pos.y, 0, 480, 1, -1);

	gl_Position = vec4(xpos, ypos, 0.0, 1.0);
	color = vec3(vcolor) / 255.0f;
}