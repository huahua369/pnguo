#version 450

layout(binding=0) uniform u_UniformBuffer {
    mat4 u_mvpMatrix;
};

layout(location = 0) in vec3 pos;
layout(location = 1) in vec2 uv;
layout(location = 2) in vec4 col;
layout(location = 3) in vec4 col1;

out gl_PerVertex
{
    vec4 gl_Position;
};

layout(location = 0) out struct{
    vec4 col[2];
    vec2 uv;
} o;

void main(){
    gl_Position = u_mvpMatrix * vec4(pos.xyz, 1);
    o.uv = uv;
    o.col[0] = col; 
    o.col[1] = col1;
}
