#version 450
//#extension GL_OES_standard_derivatives : enable
#extension GL_EXT_shader_texture_lod: enable 
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
// this makes the structures declared with a scalar layout match the c structures
#extension GL_EXT_scalar_block_layout : enable

precision highp float;

//--------------------------------------------------------------------------------------
//  PS Inputs
//--------------------------------------------------------------------------------------

#include "GLTF_VS2PS_IO.h"
layout (location = 0) in VS2PS Input;

//--------------------------------------------------------------------------------------
// PS Outputs
//--------------------------------------------------------------------------------------

#ifdef HAS_MOTION_VECTORS_RT
layout(location = HAS_MOTION_VECTORS_RT) out vec2 Output_motionVect;
#endif

#ifdef HAS_FORWARD_RT
layout (location = HAS_FORWARD_RT) out vec4 Output_finalColor;
#endif

#ifdef HAS_SPECULAR_ROUGHNESS_RT
layout (location = HAS_SPECULAR_ROUGHNESS_RT) out vec4 Output_specularRoughness;
#endif

#ifdef HAS_DIFFUSE_RT
layout (location = HAS_DIFFUSE_RT) out vec4 Output_diffuseColor;
#endif

#ifdef HAS_NORMALS_RT
layout (location = HAS_NORMALS_RT) out vec4 Output_normal;
#endif

#ifdef HAS_OIT_ACCUM_RT
layout (location = HAS_OIT_ACCUM_RT) out vec4 outAccum;	// 积累纹理（RGBA32F）
layout (location = HAS_OIT_WEIGHT_RT) out float outReveal;	// 权重纹理（R32F）
#endif
//--------------------------------------------------------------------------------------
//
// Constant Buffers 
//
//--------------------------------------------------------------------------------------
 
#include "pbr_px0.h"
 
//--------------------------------------------------------------------------------------
// mainPS
//--------------------------------------------------------------------------------------
void main()
{  
	discardPixelIfAlphaCutOff(Input);
	gpuMaterial m = defaultPbrMaterial();
#ifdef ID_TEXCOORD_0
	vec2 uv = Input.UV[0];
#else
	vec2 uv = vec2(0.0, 0.0);
#endif
	m.uv = uv;
	getPBRParams(Input, u_pbrParams, m);
	vec3 c3 = doPbrLighting(Input, myPerFrame, m);
	vec4 color = vec4(c3, m.baseColor.a);   
#ifdef HAS_MOTION_VECTORS_RT
	Output_motionVect = Input.CurrPosition.xy / Input.CurrPosition.w - Input.PrevPosition.xy / Input.PrevPosition.w;
#endif

#ifdef HAS_SPECULAR_ROUGHNESS_RT
	Output_specularRoughness = vec4(m.specularColor, m.alphaRoughness);
#endif

#ifdef HAS_DIFFUSE_RT
	Output_diffuseColor = vec4(m.diffuseColor, 0);
#endif

#ifdef HAS_NORMALS_RT
	Output_normal = vec4((getPixelNormal(Input) + 1) / 2, 0);
#endif 
	color = mix(color, vec4(myPerFrame.u_WireframeOptions.rgb, 1.0), myPerFrame.u_WireframeOptions.w);
#ifdef HAS_OIT_ACCUM_RT
		if (u_pbrParams.alphaMode == ALPHA_BLEND)
		{
			const float depthPower = 10.0;  // 深度衰减系数
			float z = Input.depth;
			color.rgb *= color.a; 
			const float depthZ = -z*depthPower;//0.1 < z < 500,
			const float distWeight = clamp(0.03 / (1e-5 + pow(depthZ / 200, 4.0)), 1e-2, 3e3);
			float alphaWeight = min(1.0, max(max(color.r, color.g), max(color.b, color.a)) * 40.0 + 0.01);
			alphaWeight *= alphaWeight;
			float weight = alphaWeight * distWeight;
			weight = clamp(pow(min(1.0, color.a * 10.0) + 0.01, 3.0) * 1e8 * pow(1.0 - gl_FragCoord.z * 0.9, 3.0), 1e-2, 3e3);
			weight = clamp(color.a, 0.0, 1.0);
			outAccum = vec4(color.rgb*weight, (color.a)*weight);
			outReveal = 1.0 - color.a;
			color *= 0.0;
		}else{
			outAccum =vec4(0);// 分支也要输出默认值
			outReveal = 0.0; 
		}
			
#endif
#ifdef HAS_FORWARD_RT 
	Output_finalColor = color;
#endif
}
