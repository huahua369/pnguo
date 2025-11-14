#version 450
layout(push_constant) uniform uPushConstant
//layout(binding=0) uniform u_UniformBuffer 
{
    mat4 u_mvpMatrix;
};

layout(location = 0) in vec3 pos;
layout(location = 1) in vec2 uv;
layout(location = 2) in vec4 col; 

out gl_PerVertex
{
    vec4 gl_Position;
};

layout(location = 0) out struct{
    vec4 col;
    vec2 uv;
} o;

void main(){
    gl_Position = u_mvpMatrix * vec4(pos.xyz, 1);
    o.uv = uv;
    o.col = col;  
}
