// Portions Copyright 2019 Advanced Micro Devices, Inc.All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

//--------------------------------------------------------------------------------------
//
// Texture and samplers bindings
//
//--------------------------------------------------------------------------------------
//#define CONCAT(a,b) a ## b
//#define TEXCOORD_old(id) CONCAT(Input.UV, id)
#ifdef ID_TEXCOORD_1
#define TEXCOORD(id) vec2(id < 1 ? Input.UV0 : Input.UV1);
#else
#define TEXCOORD(id) vec2(Input.UV0);
#endif

//disable texcoords that are not in the VS2PS structure
#if defined(ID_TEXCOORD_0)==false
#if ID_normalTexCoord == 0
#undef ID_normalTexture
#undef ID_normalTexCoord
#endif
#if ID_emissiveTexCoord == 0
#undef ID_emissiveTexture
#undef ID_emissiveTexCoord
#endif
#if ID_baseTexCoord == 0
#undef ID_baseColorTexture
#undef ID_baseTexCoord
#endif
#if ID_metallicRoughnessTexCoord == 0
#undef ID_metallicRoughnessTexture
#undef ID_metallicRoughnessTexCoord
#endif
#endif

#ifdef ID_baseColorTexture
layout(set = 1, binding = ID_baseColorTexture) uniform sampler2D u_BaseColorSampler;
#endif

#ifdef ID_normalTexture
layout(set = 1, binding = ID_normalTexture) uniform sampler2D u_NormalSampler;
#endif

#ifdef ID_emissiveTexture
layout(set = 1, binding = ID_emissiveTexture) uniform sampler2D u_EmissiveSampler;
#endif

#ifdef ID_metallicRoughnessTexture
layout(set = 1, binding = ID_metallicRoughnessTexture) uniform sampler2D u_MetallicRoughnessSampler;
#endif

#ifdef ID_occlusionTexture
layout(set = 1, binding = ID_occlusionTexture) uniform sampler2D u_OcclusionSampler;
float u_OcclusionStrength = 1.0;
#endif

#ifdef ID_diffuseTexture
layout(set = 1, binding = ID_diffuseTexture) uniform sampler2D u_diffuseSampler;
#endif

#ifdef ID_specularGlossinessTexture
layout(set = 1, binding = ID_specularGlossinessTexture) uniform sampler2D u_specularGlossinessSampler;
#endif

#ifdef USE_IBL
#ifdef ID_diffuseCube
layout(set = 1, binding = ID_diffuseCube) uniform samplerCube u_DiffuseEnvSampler;
#endif
#ifdef ID_specularCube
layout(set = 1, binding = ID_specularCube) uniform samplerCube u_SpecularEnvSampler;
#endif
#define USE_TEX_LOD
#endif

#ifdef ID_brdfTexture
layout(set = 1, binding = ID_brdfTexture) uniform sampler2D u_brdfLUT;
#endif
#ifdef ID_CharlieTexture
layout(set = 1, binding = ID_CharlieTexture) uniform sampler2D u_CharlieLUT;
#endif
 
#ifdef ID_specularTexture
layout(set = 1, binding = ID_specularTexture) uniform sampler2D u_specularTexture;
#endif
#ifdef ID_specularColorTexture
layout(set = 1, binding = ID_specularColorTexture) uniform sampler2D u_specularColorTexture;
#endif
#ifdef ID_transmissionTexture
layout(set = 1, binding = ID_transmissionTexture) uniform sampler2D u_transmissionTexture;
#endif  
#ifdef ID_thicknessTexture
layout(set = 1, binding = ID_thicknessTexture) uniform sampler2D u_thicknessTexture;
#endif

#ifdef ID_clearcoatRoughnessTexture
layout(set = 1, binding = ID_clearcoatRoughnessTexture) uniform sampler2D u_clearcoatRoughnessTexture;
#endif
#ifdef ID_clearcoatNormalTexture
layout(set = 1, binding = ID_clearcoatNormalTexture) uniform sampler2D u_clearcoatNormalTexture;
#endif
#ifdef ID_iridescenceTexture
layout(set = 1, binding = ID_iridescenceTexture) uniform sampler2D u_iridescenceTexture;
#endif
#ifdef ID_iridescenceThicknessTexture
layout(set = 1, binding = ID_iridescenceThicknessTexture) uniform sampler2D u_iridescenceThicknessTexture;
#endif
#ifdef ID_anisotropyTexture
layout(set = 1, binding = ID_anisotropyTexture) uniform sampler2D u_anisotropyTexture;
#endif
#ifdef ID_sheenColorTexture
layout(set = 1, binding = ID_sheenColorTexture) uniform sampler2D u_sheenColorTexture;
#endif
#ifdef ID_CharlieEnvSampler
layout(set = 1, binding = ID_CharlieEnvSampler) uniform samplerCube u_CharlieEnvSampler;
#endif
#ifdef ID_sheenRoughnessTexture
layout(set = 1, binding = ID_sheenRoughnessTexture) uniform sampler2D u_sheenRoughnessTexture;
#endif


#ifdef ID_transmissionFramebufferTexture
layout(set = 1, binding = ID_transmissionFramebufferTexture) uniform sampler2D u_TransmissionFramebufferSampler;
#endif
//------------------------------------------------------------
// UV getters
//------------------------------------------------------------
vec2 getNormalUV(VS2PS Input)
{
	vec3 uv = vec3(0.0, 0.0, 1.0);
#ifdef __cplusplus
	return uv;
#else
#ifdef ID_normalTexCoord
	uv.xy = TEXCOORD(ID_normalTexCoord);
#ifdef HAS_NORMAL_UV_TRANSFORM
	uv *= u_pbrParams.uvTransform;
#endif
#endif
	return uv.xy;
#endif
}

#if 0

vec2 getEmissiveUV(VS2PS Input)
{
	vec3 uv = vec3(0.0, 0.0, 1.0);
#ifdef __cplusplus
	return uv;
#else
#ifdef ID_emissiveTexCoord
	uv.xy = TEXCOORD(ID_emissiveTexCoord);
#ifdef HAS_EMISSIVE_UV_TRANSFORM
	uv *= u_EmissiveUVTransform;
#endif
#endif
	return uv.xy;
#endif
}

vec2 getOcclusionUV(VS2PS Input)
{
	vec3 uv = vec3(0.0, 0.0, 1.0);
#ifdef __cplusplus
	return uv;
#else
#ifdef ID_occlusionTexCoord
	uv.xy = TEXCOORD(ID_occlusionTexCoord);
#ifdef HAS_OCCLSION_UV_TRANSFORM
	uv *= u_OcclusionUVTransform;
#endif
#endif
	return uv.xy;
#endif
}

vec2 getBaseColorUV(VS2PS Input)
{
	vec3 uv = vec3(0.0, 0.0, 1.0);
#ifdef __cplusplus
	return uv;
#else
#ifdef ID_baseTexCoord
	uv.xy = TEXCOORD(ID_baseTexCoord);
#ifdef HAS_BASECOLOR_UV_TRANSFORM
	uv *= u_BaseColorUVTransform;
#endif
#endif
	return uv.xy;
#endif
}

vec2 getMetallicRoughnessUV(VS2PS Input)
{
	vec3 uv = vec3(0.0, 0.0, 1.0);
#ifdef __cplusplus
	return uv;
#else
#ifdef ID_metallicRoughnessTexCoord
	uv.xy = TEXCOORD(ID_metallicRoughnessTexCoord);
#ifdef HAS_METALLICROUGHNESS_UV_TRANSFORM
	uv *= u_MetallicRoughnessUVTransform;
#endif
#endif
	return uv.xy;
#endif 
}

vec2 getSpecularGlossinessUV(VS2PS Input)
{
	vec3 uv = vec3(0.0, 0.0, 1.0);
#ifdef __cplusplus
	return uv;
#else
#ifdef ID_specularGlossinessTexture
	uv.xy = TEXCOORD(ID_specularGlossinessTexCoord);
#ifdef HAS_SPECULARGLOSSINESS_UV_TRANSFORM
	uv *= u_SpecularGlossinessUVTransform;
#endif
#endif
	return uv.xy;
#endif
}

vec2 getDiffuseUV(VS2PS Input)
{
	vec3 uv = vec3(0.0, 0.0, 1.0);
#ifdef __cplusplus
	return uv;
#else
#ifdef ID_diffuseTexture
	uv.xy = TEXCOORD(ID_diffuseTexCoord);
#ifdef HAS_DIFFUSE_UV_TRANSFORM
	uv *= u_DiffuseUVTransform;
#endif
#endif
	return uv.xy;
#endif
}
#endif
vec4 getBaseColorTexture(VS2PS Input, vec2 uv)
{
#ifdef ID_baseColorTexture
	return texture(u_BaseColorSampler, uv, myPerFrame.u_LodBias);
#else
	return vec4(1, 1, 1, 1); //OPAQUE
#endif
}

vec4 getDiffuseTexture(VS2PS Input, vec2 uv)
{
#ifdef ID_diffuseTexture
	return texture(u_diffuseSampler, uv, myPerFrame.u_LodBias);
#else
	return vec4(1, 1, 1, 1);
#endif 
}

vec4 getMetallicRoughnessTexture(VS2PS Input, vec2 uv)
{
#ifdef ID_metallicRoughnessTexture
	return texture(u_MetallicRoughnessSampler, uv, myPerFrame.u_LodBias);
#else 
	return vec4(1, 1, 1, 1);
#endif   
}

vec4 getSpecularGlossinessTexture(VS2PS Input, vec2 uv)
{
#ifdef ID_specularGlossinessTexture    
	return texture(u_specularGlossinessSampler, uv, myPerFrame.u_LodBias);
#else 
	return vec4(1, 1, 1, 1);
#endif   
}
  
 
