#version 430 core
in vec2 tex_coords;

out vec4 frag_color;
uniform sampler2D texel;

void main()
{
	frag_color = texture(texel, tex_coords); 
}