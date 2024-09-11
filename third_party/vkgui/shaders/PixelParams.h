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


#ifdef MATERIAL_IRIDESCENCE
// XYZ to sRGB color space
const mat3 XYZ_TO_REC709 = mat3(
	3.2404542, -0.9692660, 0.0556434,
	-1.5371385, 1.8760108, -0.2040259,
	-0.4985314, 0.0415560, 1.0572252
);

// Assume air interface for top
// Note: We don't handle the case fresnel0 == 1
vec3 Fresnel0ToIor(vec3 fresnel0) {
	vec3 sqrtF0 = sqrt(fresnel0);
	return (vec3(1.0) + sqrtF0) / (vec3(1.0) - sqrtF0);
}

// Conversion FO/IOR
vec3 IorToFresnel0(vec3 transmittedIor, float incidentIor) {
	return sq((transmittedIor - vec3(incidentIor)) / (transmittedIor + vec3(incidentIor)));
}

// ior is a value between 1.0 and 3.0. 1.0 is air interface
float IorToFresnel0(float transmittedIor, float incidentIor) {
	return sq((transmittedIor - incidentIor) / (transmittedIor + incidentIor));
}

// Fresnel equations for dielectric/dielectric interfaces.
// Ref: https://belcour.github.io/blog/research/2017/05/01/brdf-thin-film.html
// Evaluation XYZ sensitivity curves in Fourier space
vec3 evalSensitivity(float OPD, vec3 shift) {
	float phase = 2.0 * M_PI * OPD * 1.0e-9;
	vec3 val = vec3(5.4856e-13, 4.4201e-13, 5.2481e-13);
	vec3 pos = vec3(1.6810e+06, 1.7953e+06, 2.2084e+06);
	vec3 var = vec3(4.3278e+09, 9.3046e+09, 6.6121e+09);

	vec3 xyz = val * sqrt(2.0 * M_PI * var) * cos(pos * phase + shift) * exp(-sq(phase) * var);
	xyz.x += 9.7470e-14 * sqrt(2.0 * M_PI * 4.5282e+09) * cos(2.2399e+06 * phase + shift[0]) * exp(-4.5282e+09 * sq(phase));
	xyz /= 1.0685e-7;

	vec3 srgb = XYZ_TO_REC709 * xyz;
	return srgb;
}

vec3 evalIridescence(float outsideIOR, float eta2, float cosTheta1, float thinFilmThickness, vec3 baseF0) {
	vec3 I;

	// Force iridescenceIor -> outsideIOR when thinFilmThickness -> 0.0
	float iridescenceIor = mix(outsideIOR, eta2, smoothstep(0.0, 0.03, thinFilmThickness));
	// Evaluate the cosTheta on the base layer (Snell law)
	float sinTheta2Sq = sq(outsideIOR / iridescenceIor) * (1.0 - sq(cosTheta1));

	// Handle TIR:
	float cosTheta2Sq = 1.0 - sinTheta2Sq;
	if (cosTheta2Sq < 0.0) {
		return vec3(1.0);
	}

	float cosTheta2 = sqrt(cosTheta2Sq);

	// First interface
	float R0 = IorToFresnel0(iridescenceIor, outsideIOR);
	float R12 = F_Schlick(R0, cosTheta1);
	float R21 = R12;
	float T121 = 1.0 - R12;
	float phi12 = 0.0;
	if (iridescenceIor < outsideIOR) phi12 = M_PI;
	float phi21 = M_PI - phi12;

	// Second interface
	vec3 baseIOR = Fresnel0ToIor(clamp(baseF0, 0.0, 0.9999)); // guard against 1.0
	vec3 R1 = IorToFresnel0(baseIOR, iridescenceIor);
	vec3 R23 = F_Schlick(R1, cosTheta2);
	vec3 phi23 = vec3(0.0);
	if (baseIOR[0] < iridescenceIor) phi23[0] = M_PI;
	if (baseIOR[1] < iridescenceIor) phi23[1] = M_PI;
	if (baseIOR[2] < iridescenceIor) phi23[2] = M_PI;

	// Phase shift
	float OPD = 2.0 * iridescenceIor * thinFilmThickness * cosTheta2;
	vec3 phi = vec3(phi21) + phi23;

	// Compound terms
	vec3 R123 = clamp(R12 * R23, 1e-5, 0.9999);
	vec3 r123 = sqrt(R123);
	vec3 Rs = sq(T121) * R23 / (vec3(1.0) - R123);

	// Reflectance term for m = 0 (DC term amplitude)
	vec3 C0 = R12 + Rs;
	I = C0;

	// Reflectance term for m > 0 (pairs of diracs)
	vec3 Cm = Rs - T121;
	for (int m = 1; m <= 2; ++m)
	{
		Cm *= r123;
		vec3 Sm = 2.0 * evalSensitivity(float(m) * OPD, float(m) * phi);
		I += Cm * Sm;
	}

	// Since out of gamut colors might be produced, negative color values are clamped to 0.
	return max(I, vec3(0.0));
}

#endif 
//!MATERIAL_IRIDESCENCE



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

void getPBRParams(VS2PS Input, pbrMaterial material, out gpuMaterial m)
{ 
	vec2 uv = m.uv;
	MeshState mesh;
	mesh.N = getPixelNormal(Input, uv);
	mesh.Ng = mesh.N;
	get_tangent(Input, uv, mesh.T, mesh.B);
	mesh.tc = uv;
	mesh.isInside = false;
	mesh.emissiveFactor = myPerFrame.u_EmissiveFactor;
	vec3 uv3 = vec3(mesh.tc, 1);
	// KHR_texture_transform
#ifdef __cplusplus
	vec2 texCoord = uv3 * material.uvTransform;
#else
#ifdef MATERIAL_TEXTURE_TRANSFORM
	uv3 *= material.uvTransform;
#endif
	vec2 texCoord = uv3.xy;
#endif 
	uv = texCoord;
	// Metallic and Roughness material properties are packed together
	// In glTF, these factors can be specified by fixed scalar values
	// or from a metallic-roughness map
	m.alpha = 0.0;
	m.perceptualRoughness = 0.0;
	m.diffuseColor = vec3(0.0, 0.0, 0.0);
	m.specularColor = vec3(0.0, 0.0, 0.0);
	vec3 f0 = vec3(0.04, 0.04, 0.04);

	m.baseColor = get_roughness(Input, material, texCoord, m.diffuseColor, m.specularColorlight, m.perceptualRoughness, m.metallic);
	// Roughness is authored as perceptual roughness; as is convention,
	// convert to material roughness by squaring the perceptual roughness.
	m.alphaRoughness = m.perceptualRoughness * m.perceptualRoughness;
	m.alpha = m.baseColor.a;
	m.dispersion = material.dispersion;
	// The default index of refraction of 1.5 yields a dielectric normal incidence reflectance of 0.04.
	m.ior = 1.5;
	m.f0_dielectric = vec3(0.04);
	m.specularWeight = 1.0;

	// Anything less than 2% is physically impossible and is instead considered to be shadowing. Compare to "Real-Time-Rendering" 4th editon on page 325.
	m.f90 = vec3(1.0);
	m.f90_dielectric = m.f90;
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
		normal_vector *= vec3(material.normalTextureScale, material.normalTextureScale, 1.0F);
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
	m.specularWeight = m.specular;
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

#ifdef ID_clearcoatNormalTexture 
	mat3 tbn = mat3(m.T, m.B, m.Nc);
	vec3 normal_vector = texture(u_clearcoatNormalTexture, texCoord).xyz;
	normal_vector = normal_vector * 2.0F - 1.0F;
	m.Nc = normalize(tbn * normal_vector);
#endif

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


// Get normal, tangent and bitangent vectors.
NormalInfo getNormalInfo(vec3 v, vec2 UV, vec3 vpos)
{
	vec2 uv_dx = dFdx(UV);
	vec2 uv_dy = dFdy(UV);

	if (length(uv_dx) <= 1e-2) {
		uv_dx = vec2(1.0, 0.0);
	}

	if (length(uv_dy) <= 1e-2) {
		uv_dy = vec2(0.0, 1.0);
	}

	vec3 t_ = (uv_dy.t * dFdx(vpos) - uv_dx.t * dFdy(vpos)) /
		(uv_dx.s * uv_dy.t - uv_dy.s * uv_dx.t);

	vec3 n, t, b, ng;

	// Compute geometrical TBN:
#ifdef HAS_NORMAL_VEC3
#ifdef HAS_TANGENT_VEC4
	// Trivial TBN computation, present as vertex attribute.
	// Normalize eigenvectors as matrix is linearly interpolated.
	t = normalize(v_TBN[0]);
	b = normalize(v_TBN[1]);
	ng = normalize(v_TBN[2]);
#else
	// Normals are either present as vertex attributes or approximated.
	ng = normalize(v_Normal);
	t = normalize(t_ - ng * dot(ng, t_));
	b = cross(ng, t);
#endif
#else
	ng = normalize(cross(dFdx(vpos), dFdy(vpos)));
	t = normalize(t_ - ng * dot(ng, t_));
	b = cross(ng, t);
#endif


	// For a back-facing surface, the tangential basis vectors are negated.
	if (gl_FrontFacing == false)
	{
		t *= -1.0;
		b *= -1.0;
		ng *= -1.0;
	}

	// Compute normals:
	NormalInfo info;
	info.ng = ng;
#ifdef ID_normalTexture
	info.ntex = texture(u_NormalSampler, UV).rgb * 2.0 - vec3(1.0);
	info.ntex *= vec3(u_NormalScale, u_NormalScale, 1.0);
	info.ntex = normalize(info.ntex);
	info.n = normalize(mat3(t, b, ng) * info.ntex);
#else
	info.n = ng;
#endif
	info.t = t;
	info.b = b;
	return info;
}

// Calculate lighting contribution from image based lighting source (IBL)
vec4 calculate_mater(VS2PS Input, PerFrame mp, gpuMaterial materialInfo)
{
	vec4 g_finalColor = vec4(0.0);
	vec3 baseColor = vec3(materialInfo.baseColor);

	vec3 normal = getPixelNormal(Input);
	vec3 worldPos = Input.WorldPos;
	vec3 v_Position = Input.WorldPos;
	vec3 f90 = vec3(1.0f);

	vec3 cpos = myPerFrame.u_CameraPos;
	//vec3 view = normalize(cpos - worldPos);

	vec3 v = normalize(cpos - worldPos);
	NormalInfo normalInfo = getNormalInfo(v, materialInfo.uv, worldPos);
	vec3 n = normalInfo.n;
	vec3 t = normalInfo.t;
	vec3 b = normalInfo.b;
	vec3 color = vec3(0.0);
	// LIGHTING
	vec3 f_specular_dielectric = vec3(0.0);
	vec3 f_specular_metal = vec3(0.0);
	vec3 f_diffuse = vec3(0.0);
	vec3 f_dielectric_brdf_ibl = vec3(0.0);
	vec3 f_metal_brdf_ibl = vec3(0.0);
	vec3 f_emissive = vec3(0.0);
	vec3 clearcoat_brdf = vec3(0.0);
	vec3 f_sheen = vec3(0.0);
	vec3 f_specular_transmission = vec3(0.0);
	vec3 f_diffuse_transmission = vec3(0.0);

	float clearcoatFactor = 0.0;
	vec3 clearcoatFresnel = vec3(0);

	float albedoSheenScaling = 1.0;
	float diffuseTransmissionThickness = 1.0;

#ifdef MATERIAL_IRIDESCENCE
	vec3 iridescenceFresnel_dielectric = evalIridescence(1.0, materialInfo.iridescenceIor, NdotV, materialInfo.iridescenceThickness, materialInfo.f0_dielectric);
	vec3 iridescenceFresnel_metallic = evalIridescence(1.0, materialInfo.iridescenceIor, NdotV, materialInfo.iridescenceThickness, baseColor.rgb);

	if (materialInfo.iridescenceThickness == 0.0) {
		materialInfo.iridescenceFactor = 0.0;
	}
#endif

#ifdef MATERIAL_DIFFUSE_TRANSMISSION
#ifdef MATERIAL_VOLUME
	diffuseTransmissionThickness = materialInfo.thickness *
		(length(vec3(u_ModelMatrix[0].xyz)) + length(vec3(u_ModelMatrix[1].xyz)) + length(vec3(u_ModelMatrix[2].xyz))) / 3.0;
#endif
#endif

#ifdef MATERIAL_CLEARCOAT
	clearcoatFactor = materialInfo.clearcoatFactor;
	clearcoatFresnel = F_Schlick(materialInfo.clearcoatF0, materialInfo.clearcoatF90, clampedDot(materialInfo.clearcoatNormal, v));
#endif

#ifdef USE_IBL

	f_diffuse = getDiffuseLight(n) * baseColor;

#ifdef MATERIAL_DIFFUSE_TRANSMISSION
	vec3 diffuseTransmissionIBL = getDiffuseLight(-n) * materialInfo.diffuseTransmissionColorFactor;
#ifdef MATERIAL_VOLUME
	diffuseTransmissionIBL = applyVolumeAttenuation(diffuseTransmissionIBL, diffuseTransmissionThickness, materialInfo.attenuationColor, materialInfo.attenuationDistance);
#endif
	f_diffuse = mix(f_diffuse, diffuseTransmissionIBL, materialInfo.diffuseTransmissionFactor);
#endif


#if defined(MATERIAL_TRANSMISSION)
	f_specular_transmission = getIBLVolumeRefraction(
		n, v,
		materialInfo.perceptualRoughness,
		baseColor, materialInfo.f0_dielectric, materialInfo.f90,
		v_Position, u_ModelMatrix, u_ViewMatrix, u_ProjectionMatrix,
		materialInfo.ior, materialInfo.thickness, materialInfo.attenuationColor, materialInfo.attenuationDistance, materialInfo.dispersion);
	f_diffuse = mix(f_diffuse, f_specular_transmission, materialInfo.transmissionFactor);
#endif

#ifdef MATERIAL_ANISOTROPY
	f_specular_metal = getIBLRadianceAnisotropy(n, v, materialInfo.perceptualRoughness, materialInfo.anisotropyStrength, materialInfo.anisotropicB);
	f_specular_dielectric = f_specular_metal;
#else
	f_specular_metal = getIBLRadianceGGX(n, v, materialInfo.perceptualRoughness);
	f_specular_dielectric = f_specular_metal;
#endif

	// Calculate fresnel mix for IBL  

	vec3 f_metal_fresnel_ibl = getIBLGGXFresnel(n, v, materialInfo.perceptualRoughness, baseColor.rgb, 1.0);
	f_metal_brdf_ibl = f_metal_fresnel_ibl * f_specular_metal;

	vec3 f_dielectric_fresnel_ibl = getIBLGGXFresnel(n, v, materialInfo.perceptualRoughness, materialInfo.f0_dielectric, materialInfo.specularWeight);
	f_dielectric_brdf_ibl = mix(f_diffuse, f_specular_dielectric, f_dielectric_fresnel_ibl);

#ifdef MATERIAL_IRIDESCENCE
	f_metal_brdf_ibl = mix(f_metal_brdf_ibl, f_specular_metal * iridescenceFresnel_metallic, materialInfo.iridescenceFactor);
	f_dielectric_brdf_ibl = mix(f_dielectric_brdf_ibl, rgb_mix(f_diffuse, f_specular_dielectric, iridescenceFresnel_dielectric), materialInfo.iridescenceFactor);
#endif


#ifdef MATERIAL_CLEARCOAT
	clearcoat_brdf = getIBLRadianceGGX(materialInfo.clearcoatNormal, v, materialInfo.clearcoatRoughness);
#endif

#ifdef MATERIAL_SHEEN
	f_sheen = getIBLRadianceCharlie(n, v, materialInfo.sheenRoughnessFactor, materialInfo.sheenColorFactor);
	albedoSheenScaling = 1.0 - max3(materialInfo.sheenColorFactor) * albedoSheenScalingLUT(NdotV, materialInfo.sheenRoughnessFactor);
#endif

	color = mix(f_dielectric_brdf_ibl, f_metal_brdf_ibl, materialInfo.metallic);
	color = f_sheen + color * albedoSheenScaling;
	color = mix(color, clearcoat_brdf, clearcoatFactor * clearcoatFresnel);

#ifdef HAS_OCCLUSION_MAP
	float ao = 1.0;
	ao = texture(u_OcclusionSampler, getOcclusionUV()).r;
	color = color * (1.0 + u_OcclusionStrength * (ao - 1.0));
#endif

#endif //end USE_IBL

	f_diffuse = vec3(0.0);
	f_specular_dielectric = vec3(0.0);
	f_specular_metal = vec3(0.0);
	vec3 f_dielectric_brdf = vec3(0.0);
	vec3 f_metal_brdf = vec3(0.0);

#ifdef USE_PUNCTUAL
	for (int i = 0; i < LIGHT_COUNT; ++i)
	{
		Light light = u_Lights[i];

		vec3 pointToLight;
		if (light.type != LightType_Directional)
		{
			pointToLight = light.position - v_Position;
		}
		else
		{
			pointToLight = -light.direction;
		}

		// BSTF
		vec3 l = normalize(pointToLight);   // Direction from surface point to light
		vec3 h = normalize(l + v);          // Direction of the vector between l and v, called halfway vector
		float NdotL = clampedDot(n, l);
		float NdotV = clampedDot(n, v);
		float NdotH = clampedDot(n, h);
		float LdotH = clampedDot(l, h);
		float VdotH = clampedDot(v, h);

		vec3 dielectric_fresnel = F_Schlick(materialInfo.f0_dielectric * materialInfo.specularWeight, materialInfo.f90_dielectric, abs(VdotH));
		vec3 metal_fresnel = F_Schlick(baseColor.rgb, vec3(1.0), abs(VdotH));

		vec3 lightIntensity = getLighIntensity(light, pointToLight);

		vec3 l_diffuse = lightIntensity * NdotL * BRDF_lambertian(baseColor.rgb);
		vec3 l_specular_dielectric = vec3(0.0);
		vec3 l_specular_metal = vec3(0.0);
		vec3 l_dielectric_brdf = vec3(0.0);
		vec3 l_metal_brdf = vec3(0.0);
		vec3 l_clearcoat_brdf = vec3(0.0);
		vec3 l_sheen = vec3(0.0);
		float l_albedoSheenScaling = 1.0;

#ifdef MATERIAL_DIFFUSE_TRANSMISSION
		vec3 diffuse_btdf = lightIntensity * clampedDot(-n, l) * BRDF_lambertian(materialInfo.diffuseTransmissionColorFactor);

#ifdef MATERIAL_VOLUME
		diffuse_btdf = applyVolumeAttenuation(diffuse_btdf, diffuseTransmissionThickness, materialInfo.attenuationColor, materialInfo.attenuationDistance);
#endif
		l_diffuse = mix(l_diffuse, diffuse_btdf, materialInfo.diffuseTransmissionFactor);
#endif // MATERIAL_DIFFUSE_TRANSMISSION

		// BTDF (Bidirectional Transmittance Distribution Function)
#ifdef MATERIAL_TRANSMISSION
		// If the light ray travels through the geometry, use the point it exits the geometry again.
		// That will change the angle to the light source, if the material refracts the light ray.
		vec3 transmissionRay = getVolumeTransmissionRay(n, v, materialInfo.thickness, materialInfo.ior, u_ModelMatrix);
		pointToLight -= transmissionRay;
		l = normalize(pointToLight);

		vec3 transmittedLight = lightIntensity * getPunctualRadianceTransmission(n, v, l, materialInfo.alphaRoughness, materialInfo.f0_dielectric, materialInfo.f90, baseColor.rgb, materialInfo.ior);

#ifdef MATERIAL_VOLUME
		transmittedLight = applyVolumeAttenuation(transmittedLight, length(transmissionRay), materialInfo.attenuationColor, materialInfo.attenuationDistance);
#endif
		l_diffuse = mix(l_diffuse, transmittedLight, materialInfo.transmissionFactor);
#endif

		// Calculation of analytical light
		// https://github.com/KhronosGroup/glTF/tree/master/specification/2.0#acknowledgments AppendixB
		vec3 intensity = getLighIntensity(light, pointToLight);

#ifdef MATERIAL_ANISOTROPY
		l_specular_metal = intensity * NdotL * BRDF_specularGGXAnisotropy(materialInfo.alphaRoughness, materialInfo.anisotropyStrength, n, v, l, h, materialInfo.anisotropicT, materialInfo.anisotropicB);
		l_specular_dielectric = l_specular_metal;
#else
		l_specular_metal = intensity * NdotL * BRDF_specularGGX(materialInfo.alphaRoughness, NdotL, NdotV, NdotH);
		l_specular_dielectric = l_specular_metal;
#endif

		l_metal_brdf = metal_fresnel * l_specular_metal;
		l_dielectric_brdf = mix(l_diffuse, l_specular_dielectric, dielectric_fresnel); // Do we need to handle vec3 fresnel here?

#ifdef MATERIAL_IRIDESCENCE
		l_metal_brdf = mix(l_metal_brdf, l_specular_metal * iridescenceFresnel_metallic, materialInfo.iridescenceFactor);
		l_dielectric_brdf = mix(l_dielectric_brdf, rgb_mix(l_diffuse, l_specular_dielectric, iridescenceFresnel_dielectric), materialInfo.iridescenceFactor);
#endif

#ifdef MATERIAL_CLEARCOAT
		l_clearcoat_brdf = intensity * getPunctualRadianceClearCoat(materialInfo.clearcoatNormal, v, l, h, VdotH,
			materialInfo.clearcoatF0, materialInfo.clearcoatF90, materialInfo.clearcoatRoughness);
#endif

#ifdef MATERIAL_SHEEN
		l_sheen = intensity * getPunctualRadianceSheen(materialInfo.sheenColorFactor, materialInfo.sheenRoughnessFactor, NdotL, NdotV, NdotH);
		l_albedoSheenScaling = min(1.0 - max3(materialInfo.sheenColorFactor) * albedoSheenScalingLUT(NdotV, materialInfo.sheenRoughnessFactor),
			1.0 - max3(materialInfo.sheenColorFactor) * albedoSheenScalingLUT(NdotL, materialInfo.sheenRoughnessFactor));
#endif

		vec3 l_color = mix(l_dielectric_brdf, l_metal_brdf, materialInfo.metallic);
		l_color = l_sheen + l_color * l_albedoSheenScaling;
		l_color = mix(l_color, l_clearcoat_brdf, clearcoatFactor * clearcoatFresnel);
		color += l_color;
	}
#endif // USE_PUNCTUAL

	f_emissive = u_EmissiveFactor;
#ifdef MATERIAL_EMISSIVE_STRENGTH
	f_emissive *= u_EmissiveStrength;
#endif
#ifdef HAS_EMISSIVE_MAP
	f_emissive *= texture(u_EmissiveSampler, getEmissiveUV()).rgb;
#endif


#ifdef MATERIAL_UNLIT
	color = baseColor.rgb;
#else
	color = f_emissive * (1.0 - clearcoatFactor * clearcoatFresnel) + color;
#endif

#if DEBUG == DEBUG_NONE

#if ALPHAMODE == ALPHAMODE_MASK
	// Late discard to avoid sampling artifacts. See https://github.com/KhronosGroup/glTF-Sample-Viewer/issues/267
	if (baseColor.a < u_AlphaCutoff)
	{
		discard;
	}
	baseColor.a = 1.0;
#endif

#ifdef LINEAR_OUTPUT
	g_finalColor = vec4(color.rgb, baseColor.a);
#else
	g_finalColor = vec4(toneMap(color), baseColor.a);
#endif

#else
	// In case of missing data for a debug view, render a checkerboard.
	g_finalColor = vec4(1.0);
	{
		float frequency = 0.02;
		float gray = 0.9;

		vec2 v1 = step(0.5, fract(frequency * gl_FragCoord.xy));
		vec2 v2 = step(0.5, vec2(1.0) - fract(frequency * gl_FragCoord.xy));
		g_finalColor.rgb *= gray + v1.x * v1.y + v2.x * v2.y;
	}
#endif
	return g_finalColor;
}