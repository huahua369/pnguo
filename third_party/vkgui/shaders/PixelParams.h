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

gpuMaterial defaultPbrMaterial()
{
	gpuMaterial mat;
	mat.baseColor = vec4(1.0F);
	mat.roughness = vec2(1.0F);
	mat.metallic = 1.0F;
	mat.emissive = vec3(0.0F);

	mat.N = vec3(0.0F, 0.0F, 1.0F);
	mat.Ng = vec3(0.0F, 0.0F, 1.0F);
	mat.T = vec3(1.0F, 0.0F, 0.0F);
	mat.B = vec3(0.0F, 1.0F, 0.0F);

	mat.ior1 = 1.0F;
	mat.ior2 = 1.5F;

	mat.specular = 1.0F;
	mat.specularColor = vec3(1.0F);
	mat.transmission = 0.0F;

	mat.attenuationColor = vec3(1.0F);
	mat.attenuationDistance = 1.0F;
	mat.thickness = 0.0F;

	mat.clearcoatFactor = 0.0F;
	mat.clearcoatRoughness = 0.01F;
	mat.clearcoatNormal = vec3(0.0F, 0.0F, 1.0F);

	mat.iridescence = 0.0F;
	mat.iridescenceIor = 1.5F;
	mat.iridescenceThickness = 0.1F;

	mat.sheenColor = vec3(0.0F);
	mat.sheenRoughness = 0.0F;
	mat.ao = 0;
	return mat;
}

gpuMaterial defaultPbrMaterial(vec3 baseColor, float metallic, float roughness, vec3 N, vec3 Ng)
{
	gpuMaterial mat = defaultPbrMaterial();
	mat.baseColor = vec4(baseColor, 1.0f);
	mat.metallic = metallic;
	mat.roughness = vec2(roughness * roughness);
	mat.Ng = Ng;
	mat.N = N;
	orthonormalBasis(mat.N, mat.T, mat.B);

	return mat;
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

vec4 getBaseColor2(VS2PS Input, pbrMaterial params, vec2 uv)
{
	vec4 baseColor = getBaseColor(Input, uv);

#ifdef MATERIAL_SPECULARGLOSSINESS
	baseColor *= params.pbrDiffuseFactor;
#endif

#ifdef MATERIAL_METALLICROUGHNESS
	baseColor *= params.baseColorFactor;
#endif

	baseColor *= getPixelColor(Input);
	return baseColor;
}

void discardPixelIfAlphaCutOff(VS2PS Input)
{
#ifdef ID_TEXCOORD_0
	vec2 uv = Input.UV0;
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

vec4 get_roughness(VS2PS Input, pbrMaterial params, vec2 uv, out vec3 diffuseColor, out vec3  specularColor, out float perceptualRoughness, out float metallic)
{
	perceptualRoughness = 0.0;
	diffuseColor = vec3(0.0, 0.0, 0.0);
	specularColor = vec3(0.0, 0.0, 0.0);
	metallic = 0.0;
	vec3 f0 = vec3(0.04, 0.04, 0.04);
	vec4 baseColor = getBaseColor2(Input, params, uv);
	// Metallic and Roughness material properties are packed together
	// In glTF, these factors can be specified by fixed scalar values
	// or from a metallic-roughness map
#ifdef MATERIAL_SPECULARGLOSSINESS
	vec4 sgSample = getSpecularGlossinessTexture(Input, uv);
	perceptualRoughness = (1.0 - sgSample.a * params.glossinessFactor); // glossiness to roughness
	f0 = vec3(sgSample) * params.pbrSpecularFactor; // specular

	// f0 = specular
	specularColor = f0;
	float oneMinusSpecularStrength = 1.0 - max(max(f0.r, f0.g), f0.b);
	diffuseColor = vec3(baseColor) * oneMinusSpecularStrength;

#ifdef DEBUG_METALLIC
	// do conversion between metallic M-R and S-G metallic
	metallic = solveMetallic(vec3(baseColor), specularColor, oneMinusSpecularStrength);
#endif // ! DEBUG_METALLIC
#endif // ! MATERIAL_SPECULARGLOSSINESS

#ifdef MATERIAL_METALLICROUGHNESS
	// Roughness is stored in the 'g' channel, metallic is stored in the 'b' channel.
	// This layout intentionally reserves the 'r' channel for (optional) occlusion map data
	vec4 mrSample = getMetallicRoughnessTexture(Input, uv);
	perceptualRoughness = mrSample.g * params.roughnessFactor;
	metallic = mrSample.b * params.metallicFactor;
	diffuseColor = vec3(baseColor) * (vec3(1.0, 1.0, 1.0) - f0) * (1.0 - metallic);
	specularColor = mix(f0, vec3(baseColor), metallic);
#endif // ! MATERIAL_METALLICROUGHNESS

	perceptualRoughness = clamp(perceptualRoughness, 0.0f, 1.0f);
	metallic = clamp(metallic, 0.0f, 1.0f);
	return baseColor;
}

vec4 get_roughness(VS2PS Input, pbrMaterial params, vec2 uv, out vec3 diffuseColor, out vec3  specularColor, out float perceptualRoughness)
{
	float metallic;
	return get_roughness(Input, params, uv, diffuseColor, specularColor, perceptualRoughness, metallic);
}
//-----------------------------------------------------------------------

void getPBRParams(VS2PS Input, pbrMaterial material, inout gpuMaterial m)
{
	MeshState mesh;
	vec2 uv = m.uv;
	mesh.N = getPixelNormal(Input, uv);
	mesh.Ng = mesh.N;
	get_tangent(Input, uv, mesh.T, mesh.B);
	mesh.tc = uv;
	mesh.isInside = false;
	mesh.emissiveFactor = myPerFrame.u_EmissiveFactor;
	vec3 uv3 = vec3(mesh.tc, 1);
	// KHR_texture_transform 
#ifdef MATERIAL_TEXTURE_TRANSFORM
	uv3 *= material.uvTransform;
#endif
	vec2 texCoord = vec2(uv3);
	uv = texCoord;
	// Metallic and Roughness material properties are packed together
	// In glTF, these factors can be specified by fixed scalar values
	// or from a metallic-roughness map
	 
	vec3 f0 = vec3(0.04, 0.04, 0.04);

	m.baseColor = get_roughness(Input, material, uv, m.diffuseColor, m.specularColorlight, m.perceptualRoughness, m.metallic);
	// Roughness is authored as perceptual roughness; as is convention,
	// convert to material roughness by squaring the perceptual roughness.
	m.alphaRoughness = m.perceptualRoughness * m.perceptualRoughness;
	m.alpha = m.baseColor.a;

#if 1
	// Metallic-Roughness
	float roughness = material.roughnessFactor;
	roughness = max(m.perceptualRoughness, MICROFACET_MIN_ROUGHNESS);
	m.roughness = vec2(roughness * roughness);  // Square roughness for the microfacet model 

	// Normal Map
	vec3 normal = mesh.N;
	vec3 tangent = mesh.T;
	vec3 bitangent = mesh.B;
#ifdef ID_normalTexture
	//if (material.normalTexture > -1)
	{
		mat3 tbn = mat3(mesh.T, mesh.B, mesh.N);
		vec3 normal_vector = getPixelNormal(Input, texCoord);
		normal_vector = normal_vector * 2.0F - 1.0F;
		normal_vector *= vec3(material.normalScale, material.normalScale, 1.0F);
		normal = normalize(tbn * normal_vector);
}
#endif // ID_normalTexture
	// We should always have B = cross(N, T) * bitangentSign. Orthonormalize,
	// in case the normal was changed or the input wasn't orthonormal:
	m.N = normal;
	m.B = cross(m.N, tangent);
	float bitangentSign = sign(dot(bitangent, m.B));
	m.B = normalize(m.B) * bitangentSign;
	m.T = normalize(cross(m.B, m.N)) * bitangentSign;
	m.Ng = mesh.Ng;

	// Emissive term
	vec3 emissive = material.emissiveFactor * mesh.emissiveFactor;
#ifdef ID_emissiveTexture
	emissive *= texture(u_EmissiveSampler, texCoord).rgb;
#endif 
	m.emissive = max(vec3(0.0F), emissive);

	// KHR_materials_specular
	// https://github.com/KhronosGroup/glTF/tree/main/extensions/2.0/Khronos/KHR_materials_specular
	m.specularColor = material.specularColorFactor;
#ifdef ID_specularColorTexture  
	m.specularColor *= texture(u_specularColorTexture, texCoord).rgb;
#endif 
	m.specular = material.specularFactor;
#ifdef ID_specularTexture  
	m.specular *= texture(u_specularTexture, texCoord).a;
#endif

	// Dielectric Specular
	float ior1 = 1.0F;                                   // IOR of the current medium (e.g., air)
	float ior2 = material.ior;                           // IOR of the material
	if (mesh.isInside && (material.thicknessFactor > 0))  // If the material is thin-walled, we don't need to consider the inside IOR.
	{
		ior1 = material.ior;
		ior2 = 1.0F;
	}
	m.ior1 = ior1;
	m.ior2 = ior2;


	// KHR_materials_transmission
	m.transmission = material.transmissionFactor;
#ifdef ID_transmissionTexture  
	m.transmission *= texture(u_transmissionTexture, texCoord).r;
#endif
	// KHR_materials_volume
	m.attenuationColor = material.attenuationColor;
	m.attenuationDistance = material.attenuationDistance;
	m.thickness = material.thicknessFactor;
#ifdef ID_thicknessTexture  
	m.thickness *= texture(u_thicknessTexture, texCoord).r;
#endif
#if MATERIAL_CLEARCOAT
	// KHR_materials_clearcoat
	m.clearcoatFactor = material.clearcoatFactor;
	m.clearcoatRoughness = material.clearcoatRoughness;
	m.clearcoatF0 = vec3(pow((m.ior - 1.0) / (m.ior + 1.0), 2.0));
	m.clearcoatF90 = vec3(1.0);
	m.clearcoatNormal = m.N;
#endif

#ifdef ID_clearcoatTexture
	m.clearcoatFactor *= texture(u_clearcoatTexture, texCoord).r;
#endif

#ifdef ID_clearcoatRoughnessTexture
	m.clearcoatRoughness *= texture(u_clearcoatRoughnessTexture, texCoord).g;
#endif
	//m.clearcoatRoughness = max(m.clearcoatRoughness, 0.001F);
	m.clearcoatRoughness = clamp(m.clearcoatRoughness, 0.0f, 1.0f);
	 

	// Iridescence
	float iridescence = material.iridescenceFactor;
#ifdef ID_iridescenceTexture 
	iridescence *= texture(u_iridescenceTexture, texCoord).x;
#endif
	float iridescenceThickness = material.iridescenceThicknessMaximum;
#ifdef ID_iridescenceThicknessTexture
	const float t = texture(u_iridescenceThicknessTexture, texCoord).y;
	iridescenceThickness = mix(material.iridescenceThicknessMinimum, material.iridescenceThicknessMaximum, t);
#endif
	m.iridescence = (iridescenceThickness > 0.0f) ? iridescence : 0.0f;  // No iridescence when the thickness is zero.
	m.iridescenceIor = material.iridescenceIor;
	m.iridescenceThickness = iridescenceThickness;

#if 0
	// KHR_materials_anisotropy
	float anisotropyStrength = material.anisotropyStrength;
	vec2  anisotropyDirection = vec2(1.0f, 0.0f);  // By default the anisotropy strength is along the tangent.
#ifdef ID_anisotropyTexture 
	const vec4 anisotropyTex = texture(u_anisotropyTexture, texCoord);
	// .xy encodes the direction in (tangent, bitangent) space. Remap from [0, 1] to [-1, 1].
	anisotropyDirection = normalize(vec2(anisotropyTex) * 2.0f - 1.0f);
	// .z encodes the strength in range [0, 1].
	anisotropyStrength *= anisotropyTex.z;
#endif

	// If the anisotropyStrength == 0.0f (default), the roughness is isotropic.
	// No need to rotate the anisotropyDirection or tangent space.
	if (anisotropyStrength > 0.0F)
	{
		m.roughness.x = mix(m.roughness.y, 1.0f, anisotropyStrength * anisotropyStrength);

		const float s = sin(material.anisotropyRotation);  // FIXME PERF Precalculate sin, cos on host.
		const float c = cos(material.anisotropyRotation);

		anisotropyDirection =
			vec2(c * anisotropyDirection.x + s * anisotropyDirection.y, c * anisotropyDirection.y - s * anisotropyDirection.x);

		const vec3 T_aniso = m.T * anisotropyDirection.x + m.B * anisotropyDirection.y;

		m.B = normalize(cross(m.N, T_aniso));
		m.T = cross(m.B, m.N);
	}
#endif

	// KHR_materials_sheen
	vec3 sheenColor = material.sheenColorFactor;
#ifdef ID_sheenColorTexture 
	sheenColor *= vec3(texture(u_sheenColorTexture, texCoord));  // sRGB
#endif
	m.sheenColor = sheenColor;  // No sheen if this is black.

	float sheenRoughness = material.sheenRoughnessFactor;
#ifdef ID_sheenRoughnessTexture 
	sheenRoughness *= texture(u_sheenRoughnessTexture, texCoord).w;
#endif
	sheenRoughness = max(MICROFACET_MIN_ROUGHNESS, sheenRoughness);
	m.sheenRoughness = sheenRoughness;

#ifdef ID_occlusionTexture
	m.ao = texture(u_OcclusionSampler, texCoord).r;
#else
	m.ao = 1;
#endif
#endif 

}

