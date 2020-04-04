#version 430 core
out vec4 frag_color;

in vec2 texcoord;
in vec2 texpage;
in vec3 color;
in vec2 clut;
in float textured;
in float color_depth;

uniform usampler2DRect vram;

vec4 split_colors(int data)
{
	vec4 color;
	color.r = (data << 3) & 0xf8;
    color.g = (data >> 2) & 0xf8;
    color.b = (data >> 7) & 0xf8;
	color.a = int(data != 0) * 255;

    return color / vec4(255.0f);
}

vec4 sample_texel()
{
	if (textured == 1.0f) {
		if (color_depth == 4) {
			vec2 coord = vec2(texpage.x + int(texcoord.x) / 4, texpage.y + texcoord.y);
			int value = int(texture(vram, coord).r);
			int index = value >> ((int(texcoord.x) % 4) * 4) & 0xf;
			
			vec2 clut_coord = vec2(clut.x + index + 0.5f, clut.y);
			int texel = int(texture(vram, clut_coord).r);

			return split_colors(texel);
		}
		else if (color_depth == 8) {
			vec2 coord = vec2(texpage.x + int(texcoord.x) / 2, texpage.y + texcoord.y);
			int value = int(texture(vram, coord).r);
			int index = value >> ((int(texcoord.x) % 2) * 8) & 0xff;
			
			vec2 clut_coord = vec2(clut.x + index + 0.5f, clut.y);
			int texel = int(texture(vram, clut_coord).r);

			return split_colors(texel);
		}
		else {
			vec2 coord = texpage + texcoord;
			int texel = int(texture(vram, coord).r);
			return split_colors(texel);
		}
	}
	else {
		return vec4(1.0f);
	}
}

void main()
{
	frag_color = sample_texel() * vec4(color, 1.0f);
}