#version 330 core

uniform int format = -1;

uniform sampler2D y_tex;
uniform sampler2D u_tex;
uniform sampler2D v_tex;

uniform sampler2D uv_tex;

in vec2 tex_coords;
out vec4 frag_color;

const mat4 YUV_TO_RGB_MATRIX =
    mat4(
        1.1643835616, 0, 1.7927410714, -0.9729450750,
        1.1643835616, -0.2132486143, -0.5329093286, 0.3014826655,
        1.1643835616, 2.1124017857, 0, -1.1334022179,
        0, 0, 0, 1);

void main() 
{
    float y = texture2D(y_tex, tex_coords).r;

    float u = 0.0;
    float v = 0.0;

    if(format == 0 ||
       format == 13 ) {
        u = texture2D(u_tex, tex_coords).r;
        v = texture2D(v_tex, tex_coords).r;
    } else if(format == 23) {
        u = texture2D(uv_tex, tex_coords).r;
        v = texture2D(uv_tex, tex_coords).g;
    }

    frag_color = vec4(y, u, v, 1.0) * YUV_TO_RGB_MATRIX;
}
