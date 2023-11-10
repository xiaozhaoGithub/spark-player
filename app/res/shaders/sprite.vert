#version 330 core
layout (location = 0) in vec4 vertex;

uniform mat4 model_mat;
uniform mat4 proj_mat;

out vec2 tex_coords;

void main()
{
	gl_Position = proj_mat * model_mat * vec4(vertex.xy, 0.0, 1.0);
	tex_coords = vec2(vertex.z, vertex.w);
}