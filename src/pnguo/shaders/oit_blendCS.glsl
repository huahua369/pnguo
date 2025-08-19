#version 420

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
#extension GL_ARB_compute_shader  : enable

layout (local_size_x = 8, local_size_y = 8) in; 
layout(set = 0, binding = 0, rgba16f) uniform image2D opaqueTex;  // 不透明纹理 
layout(set = 0, binding = 1, rgba16f) uniform image2D accumTex;   // 积累纹理 
layout(set = 0, binding = 2, r16f) uniform image2D weightTex;  // 权重纹理 
 
void main()
{
	ivec2 coords = ivec2(gl_GlobalInvocationID.xy);
	vec4 opaqueColor = imageLoad(opaqueTex, coords).rgba;
	vec4 accum = imageLoad(accumTex, coords).rgba;
	float reveal = imageLoad(weightTex, coords).r;
	if (reveal > 0.0&&accum.a>0.0)
	{
		// 混合不透明与半透明结果（alpha混合）
		vec4 a = opaqueColor;
		vec4 b = vec4(accum.rgb / accum.a, reveal);
		vec4 color = a * (1 - b.a) + b * (b.a);
		imageStore(opaqueTex, coords, color);
	}
}