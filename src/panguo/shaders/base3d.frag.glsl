#version 450

layout(binding=1) uniform sampler2D u_Texture;
layout(location = 0) in struct{
    vec4 col;
    vec2 uv;
} d;
layout(location = 0) out vec4 o_Color;
void main()
{
  vec4 c = texture(u_Texture, d.uv.st);
  o_Color = d.col; 
}
