#version 330 core

uniform sampler2D main_tex;
uniform vec3 sprite_color;

in vec2 tex_coords;
out vec4 frag_color;

void main() 
{
	vec4 color = texture(main_tex, tex_coords);
	frag_color = vec4(sprite_color, 1.0) * color;
}