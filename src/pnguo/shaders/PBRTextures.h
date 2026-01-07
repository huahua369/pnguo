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
#define TEXCOORD(id) Input.UV[id];
#else
#define TEXCOORD(id) vec2(Input.UV[0]);
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
  
 


vec4 getPixelColor(VS2PS Input)
{
	vec4 color = vec4(1.0, 1.0, 1.0, 1.0);

#ifdef ID_COLOR_0
	color.xyz = Input.Color0;
#endif

	return color;
}

// Find the normal for this fragment, pulling either from a predefined normal map
// or from the interpolated mesh normal and tangent attributes.
vec3 getPixelNormal(VS2PS Input, vec2 UV)
{
	// Retrieve the tangent space matrix
#ifndef ID_TANGENT
	vec3 pos_dx = dFdx(Input.WorldPos);
	vec3 pos_dy = dFdy(Input.WorldPos);
	vec3 tex_dx = dFdx(vec3(UV, 0.0));
	vec3 tex_dy = dFdy(vec3(UV, 0.0));
	vec3 t = (tex_dy.y * pos_dx - tex_dx.y * pos_dy) / (tex_dx.x * tex_dy.y - tex_dy.x * tex_dx.y);

#ifdef ID_NORMAL
	vec3 ng = normalize(Input.Normal);
#else
	vec3 ng = cross(pos_dx, pos_dy);
#endif

	t = normalize(t - ng * dot(ng, t));
	vec3 b = normalize(cross(ng, t));
	mat3 tbn = mat3(t, b, ng);
#else // HAS_TANGENTS
	mat3 tbn = mat3(Input.Tangent, Input.Binormal, Input.Normal);
#endif

#ifdef ID_normalTexture
	vec2 xy = 2.0 * texture(u_NormalSampler, UV, myPerFrame.u_LodBias).rg - 1.0;
	float z = sqrt(1.0 - dot(xy, xy));
	vec3 n = vec3(xy, z);
	n = normalize(tbn * (n /* * vec3(u_NormalScale, u_NormalScale, 1.0) */));
#else
	// The tbn matrix is linearly interpolated, so we need to re-normalize
	vec3 n = tbn[2];
	n = normalize(n);
#endif

	return n;
}
vec3 getPixelNormal(VS2PS Input)
{
	vec2 UV = getNormalUV(Input);
	return getPixelNormal(Input, UV);
}
void get_tangent(VS2PS Input, vec2 uv, out vec3 tangent, out vec3 binormal)
{
#ifndef ID_TANGENT
	vec3 pos_dx = dFdx(Input.WorldPos);
	vec3 pos_dy = dFdy(Input.WorldPos);
	vec3 tex_dx = dFdx(vec3(uv, 0.0));
	vec3 tex_dy = dFdy(vec3(uv, 0.0));
	vec3 t = (tex_dy.y * pos_dx - tex_dx.y * pos_dy) / (tex_dx.x * tex_dy.y - tex_dy.x * tex_dx.y);

#ifdef ID_NORMAL
	vec3 ng = normalize(Input.Normal);
#else
	vec3 ng = cross(pos_dx, pos_dy);
#endif

	t = normalize(t - ng * dot(ng, t));
	vec3 b = normalize(cross(ng, t));
	//mat3 tbn = mat3(t, b, ng);
	tangent = t; binormal = b;
#else  
	tangent = Input.Tangent;
	binormal = Input.Binormal;
#endif
}
 


vec4 getBaseColor(VS2PS Input, vec2 uv)
{
	vec4 baseColor = vec4(0.0, 0.0, 0.0, 1.0);
#ifdef MATERIAL_SPECULARGLOSSINESS
	baseColor = getDiffuseTexture(Input, uv);
#endif

#ifdef MATERIAL_METALLICROUGHNESS
	// The albedo may be defined from a base texture or a flat color
	baseColor = getBaseColorTexture(Input, uv);
#endif
	return baseColor;
}
 

void discardPixelIfAlphaCutOff(VS2PS Input)
{
#ifdef ID_TEXCOORD_0
	vec2 uv = Input.UV[0];
#else
	vec2 uv = vec2(0.0, 0.0);
#endif
	vec4 baseColor = getBaseColor(Input, uv);

#if defined(DEF_alphaMode_BLEND)
	if (baseColor.a == 0)
		discard;
#elif defined(DEF_alphaMode_MASK) && defined(DEF_alphaCutoff)
	if (baseColor.a < DEF_alphaCutoff)
		discard;
#else
	//OPAQUE
#endif
}