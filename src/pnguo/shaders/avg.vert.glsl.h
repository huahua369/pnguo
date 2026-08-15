#version 450
#extension GL_EXT_scalar_block_layout : enable
layout(row_major) uniform;
layout(row_major) buffer;

layout(location = 0) in vec2 pos;
layout(location = 1) in vec2 uv;
layout(location = 2) in vec4 col;

out gl_PerVertex
{
	vec4 gl_Position;
};

layout(location = 0) out struct {
	vec4 Src;
	vec2 uv;
	int PatType;
	float Opacity;
	mat3x2 Mat;
} o;

layout(scalar, binding = 0, set = 1) uniform PushConsts
{
	mat3x2 mat;
	mat3x2 matInv;
	vec4 source;
	vec2 size;
	int    fullScreenQuad_srcType;
	float  opacity;
} pc;


const int FULLSCREEN_BIT = 0x10000000;
const int SRCTYPE_MASK = 0x000000FF;
const int SOLID = 0;
const int SURFACE = 1;
const int LINEAR = 2;
const int RADIAL = 3;
const int MESH = 4;
const int RASTER_SOURCE = 5;
const int SWEEP = 6;

void main()
{
	o.PatType = pc.fullScreenQuad_srcType & SRCTYPE_MASK;
	o.Mat = pc.matInv;
	o.Src = o.PatType == SOLID ? col : pc.source;
	o.Opacity = pc.opacity;

	if ((pc.fullScreenQuad_srcType & FULLSCREEN_BIT) == FULLSCREEN_BIT) {
		gl_Position = vec4(pos, 0.0f, 1.0f);
		o.uv = vec2(0, 0);
		return;
	}

	o.uv = uv;
	vec2 p = pc.mat * vec3(pos,1.0); 
	gl_Position = vec4(p * vec2(2) / pc.size - vec2(1), 0.0, 1.0); 
}

