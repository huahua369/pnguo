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

// This shader code was ported from https://github.com/KhronosGroup/glTF-WebGL-PBR
// All credits should go to his original author.

//
// This fragment shader defines a reference implementation for Physically Based Shading of
// a microfacet surface material defined by a glTF model.
//
// References:
// [1] Real Shading in Unreal Engine 4
//     http://blog.selfshadow.com/publications/s2013-shading-course/karis/s2013_pbs_epic_notes_v2.pdf
// [2] Physically Based Shading at Disney
//     http://blog.selfshadow.com/publications/s2012-shading-course/burley/s2012_pbs_disney_brdf_notes_v3.pdf
// [3] README.md - Environment Maps
//     https://github.com/KhronosGroup/glTF-WebGL-PBR/#environment-maps
// [4] "An Inexpensive BRDF Model for Physically based Rendering" by Christophe Schlick
//     https://www.cs.virginia.edu/~jdl/bib/appearance/analytic%20models/schlick94b.pdf

#define USE_PUNCTUAL


//
//  Get SSAO from texture
//
#ifdef ID_SSAO
layout(set = 1, binding = ID_SSAO) uniform sampler2D ssaoSampler;

float GetSSAO(vec2 coords)
{
	return texture(ssaoSampler, coords).r;
}
#else
float GetSSAO(vec2 coords)
{
	return 1.0f;
}
#endif

// Calculation of the lighting contribution from an optional Image Based Light source.
// Precomputed Environment Maps are required uniform inputs and are computed as outlined in [1].
// See our README.md on Environment Maps [3] for additional discussion.
#ifdef USE_IBL
vec3 getIBLContribution(gpuMaterial materialInfo, vec3 n, vec3 v)
{
	float NdotV = clamp(dot(n, v), 0.0f, 1.0f);

	float u_MipCount = 9.0; // resolution of 512x512 of the IBL
	float lod = clamp(materialInfo.perceptualRoughness * float(u_MipCount), 0.0f, float(u_MipCount));
	vec3 reflection = normalize(reflect(-v, n));

	vec2 brdfSamplePoint = clamp(vec2(NdotV, materialInfo.perceptualRoughness), vec2(0.0, 0.0), vec2(1.0, 1.0));
	// retrieve a scale and bias to F0. See [1], Figure 3
	vec2 brdf = texture(u_brdfLUT, brdfSamplePoint).rg;

	vec3 diffuseLight = texture(u_DiffuseEnvSampler, n).rgb;

#ifdef USE_TEX_LOD
	vec3 specularLight = textureLod(u_SpecularEnvSampler, reflection, lod).rgb;
#else
	vec3 specularLight = texture(u_SpecularEnvSampler, reflection).rgb;
#endif

	vec3 diffuse = diffuseLight * materialInfo.diffuseColor;
	vec3 specular = specularLight * (materialInfo.specularColorlight * brdf.x + brdf.y);

	return diffuse + specular;
}
#endif

// Lambert lighting
// see https://seblagarde.wordpress.com/2012/01/08/pi-or-not-to-pi-in-game-lighting-equation/
vec3 diffuse(gpuMaterial materialInfo)
{
	return materialInfo.diffuseColor / M_PI;
}

// The following equation models the Fresnel reflectance term of the spec equation (aka F())
// Implementation of fresnel from [4], Equation 15
vec3 specularReflection(gpuMaterial materialInfo, AngularInfo angularInfo)
{
	return materialInfo.reflectance0 + (materialInfo.reflectance90 - materialInfo.reflectance0) * pow(clamp(1.0 - angularInfo.VdotH, 0.0, 1.0), 5.0);
}

// Smith Joint GGX
// Note: Vis = G / (4 * NdotL * NdotV)
// see Eric Heitz. 2014. Understanding the Masking-Shadowing Function in Microfacet-Based BRDFs. Journal of Computer Graphics Techniques, 3
// see Real-Time Rendering. Page 331 to 336.
// see https://google.github.io/filament/Filament.md.html#materialsystem/specularbrdf/geometricshadowing(specularg)
float visibilityOcclusion(gpuMaterial materialInfo, AngularInfo angularInfo)
{
	float NdotL = angularInfo.NdotL;
	float NdotV = angularInfo.NdotV;
	float alphaRoughnessSq = materialInfo.alphaRoughness * materialInfo.alphaRoughness;

	float GGXV = NdotL * sqrt(NdotV * NdotV * (1.0 - alphaRoughnessSq) + alphaRoughnessSq);
	float GGXL = NdotV * sqrt(NdotL * NdotL * (1.0 - alphaRoughnessSq) + alphaRoughnessSq);

	float GGX = GGXV + GGXL;
	if (GGX > 0.0)
	{
		return 0.5 / GGX;
	}
	return 0.0;
}

// The following equation(s) model the distribution of microfacet normals across the area being drawn (aka D())
// Implementation from "Average Irregularity Representation of a Roughened Surface for Ray Reflection" by T. S. Trowbridge, and K. P. Reitz
// Follows the distribution function recommended in the SIGGRAPH 2013 course notes from EPIC Games [1], Equation 3.
float microfacetDistribution(gpuMaterial materialInfo, AngularInfo angularInfo)
{
	float alphaRoughnessSq = materialInfo.alphaRoughness * materialInfo.alphaRoughness;
	float f = (angularInfo.NdotH * alphaRoughnessSq - angularInfo.NdotH) * angularInfo.NdotH + 1.0;
	return alphaRoughnessSq / (M_PI * f * f + 0.000001f);
}

vec3 getPointShade(vec3 pointToLight, gpuMaterial materialInfo, vec3 normal, vec3 view)
{
	AngularInfo angularInfo = getAngularInfo(pointToLight, normal, view);

	if (angularInfo.NdotL > 0.0 || angularInfo.NdotV > 0.0)
	{
		// Calculate the shading terms for the microfacet specular shading model
		vec3 F = specularReflection(materialInfo, angularInfo);
		float Vis = visibilityOcclusion(materialInfo, angularInfo);
		float D = microfacetDistribution(materialInfo, angularInfo);

		// Calculation of analytical lighting contribution
#ifndef __cplusplus
		vec3 diffuseContrib = (1.0 - F) * diffuse(materialInfo);
#else
		vec3 f1 = vec3(1, 1, 1) - F;
		vec3 diffuseContrib = f1 * diffuse(materialInfo);
#endif // __cplusplus

		vec3 specContrib = F * Vis * D;

		// Obtain final intensity as reflectance (BRDF) scaled by the energy of the light (cosine law)
		return angularInfo.NdotL * (diffuseContrib + specContrib);
	}

	return vec3(0.0, 0.0, 0.0);
}

// https://github.com/KhronosGroup/glTF/blob/master/extensions/2.0/Khronos/KHR_lights_punctual/README.md#range-property
float getRangeAttenuation(float range, float distance)
{
	if (range < 0.0)
	{
		// negative range means unlimited
		return 1.0;
	}
	return max(mix(1, 0, distance / range), 0);//max(min(1.0 - pow(distance / range, 4.0), 1.0), 0.0) / pow(distance, 2.0);
}

// https://github.com/KhronosGroup/glTF/blob/master/extensions/2.0/Khronos/KHR_lights_punctual/README.md#inner-and-outer-cone-angles
float getSpotAttenuation(vec3 pointToLight, vec3 spotDirection, float outerConeCos, float innerConeCos)
{
	float actualCos = dot(normalize(spotDirection), normalize(-pointToLight));
	if (actualCos > outerConeCos)
	{
		if (actualCos < innerConeCos)
		{
			return smoothstep(outerConeCos, innerConeCos, actualCos);
		}
		return 1.0;
	}
	return 0.0;
}

// 计算穿过体积的衰减光 Compute attenuated light as it travels through a volume.
vec3 applyVolumeAttenuation(vec3 radiance, float transmissionDistance, vec3 attenuationColor, float attenuationDistance)
{
	if (attenuationDistance == 0.0)
	{
		// Attenuation distance is +鈭?(which we indicate by zero), i.e. the transmitted color is not attenuated at all.
		return radiance;
	}
	else
	{
		// Compute light attenuation using Beer's law.
		vec3 transmittance = pow(attenuationColor, vec3(transmissionDistance / attenuationDistance));
		return transmittance * radiance;
	}
}


vec3 getVolumeTransmissionRay(vec3 n, vec3 v, float thickness, float ior, mat4 modelMatrix)
{
	// Direction of refracted light.
	vec3 refractionVector = refract(-v, normalize(n), 1.0f / ior);

	// Compute rotation-independant scaling of the model matrix.
	vec3 modelScale;
	modelScale.x = length(vec3(modelMatrix[0]));
	modelScale.y = length(vec3(modelMatrix[1]));
	modelScale.z = length(vec3(modelMatrix[2]));

	// The thickness is specified in local space.
	return normalize(refractionVector) * thickness * modelScale;
}


struct LightContrib
{
	vec3  incidentVector;
	float halfAngularSize;
	vec3  intensity;
	float distance;
	vec3 pointToLight;
};
LightContrib get_lc()
{
	LightContrib contrib;
	contrib.incidentVector = vec3(0.0F);
	contrib.halfAngularSize = 0.0F;
	contrib.intensity = vec3(0.0F);
	contrib.distance = INFINITE;
	return contrib;
}
LightContrib applyDirectionalLight(Light light, gpuMaterial materialInfo, vec3 normal, vec3 view)
{
	vec3 pointToLight = light.direction;
	vec3 shade = getPointShade(pointToLight, materialInfo, normal, view);

	LightContrib contrib = get_lc();
	contrib.intensity = light.intensity * light.color * shade;
	contrib.incidentVector = light.direction;
	contrib.pointToLight = pointToLight;
	return contrib;
}

LightContrib applyPointLight(Light light, gpuMaterial materialInfo, vec3 normal, vec3 worldPos, vec3 view)
{
	vec3 pointToLight = light.position - worldPos;
	float distance = length(pointToLight);
	float attenuation = getRangeAttenuation(light.range, distance);
	vec3 shade = getPointShade(pointToLight, materialInfo, normal, view);
	LightContrib contrib = get_lc();
	contrib.intensity = attenuation * light.intensity * light.color * shade;
	float r_distance = 1.0F / distance;
	contrib.incidentVector = pointToLight * r_distance;
	contrib.pointToLight = pointToLight;
	return contrib;
}

LightContrib applySpotLight(Light light, gpuMaterial materialInfo, vec3 normal, vec3 worldPos, vec3 view)
{
	vec3 pointToLight = light.position - worldPos;
	float distance = length(pointToLight);
	float rangeAttenuation = getRangeAttenuation(light.range, distance);
	float spotAttenuation = getSpotAttenuation(pointToLight, -light.direction, light.outerConeCos, light.innerConeCos);
	vec3 shade = getPointShade(pointToLight, materialInfo, normal, view);
	vec3 color = rangeAttenuation * spotAttenuation * light.intensity * light.color * shade;

	LightContrib contrib = get_lc();
	contrib.intensity = color;
	float r_distance = 1.0F / distance;
	contrib.incidentVector = pointToLight * r_distance;
	contrib.pointToLight = pointToLight;
	return contrib;
}

#if 1 
vec3 getDiffuseLight(vec3 n)
{
#ifndef __cplusplus  
	vec3 dir = rotate(n, vec3(0, 1, 0), -myPerFrame.envRotation);
	return texture(u_DiffuseEnvSampler, dir).rgb * myPerFrame.u_iblFactor;
#else
	return {};
#endif
}

vec4 getSpecularSample(vec3 reflection, float lod)
{
#ifndef __cplusplus  
	vec3 dir = rotate(reflection, vec3(0, 1, 0), -myPerFrame.envRotation);
	return textureLod(u_SpecularEnvSampler, dir, lod) * myPerFrame.u_iblFactor;
#else
	return {};
#endif
}
#if __cplusplus
#define textureQueryLevels(s) (0)
#define u_SpecularEnvSampler
#ifndef texture
#define texture()
#endif
#endif // __cplusplus

vec3 getIBLRadianceGGXa(vec3 n, vec3 v, float roughness)
{
	int   u_MipCount = textureQueryLevels(u_SpecularEnvSampler);// u_GGXEnvSampler);
	float NdotV = clampedDot(n, v);
	float lod = roughness * float(u_MipCount - 1);
	vec3 reflection = normalize(reflect(-v, n));
	vec4 specularSample = getSpecularSample(reflection, lod);

	vec3 specularLight = vec3(specularSample);

	return specularLight;
}

// specularWeight is introduced with KHR_materials_specular
vec3 getIBLRadianceLambertiana(vec3 n, vec3 v, float roughness, vec3 diffuseColor, vec3 F0)
{
#if 1
	//ndef __cplusplus
	float NdotV = clampedDot(n, v);
	vec2  brdfSamplePoint = clamp(vec2(NdotV, roughness), vec2(0.0, 0.0), vec2(1.0, 1.0));

	vec2  f_ab = vec2(texture(u_brdfLUT, brdfSamplePoint));// .rg;

	vec3 irradiance = getDiffuseLight(n);

	// see https://bruop.github.io/ibl/#single_scattering_results at Single Scattering Results
	// Roughness dependent fresnel, from Fdez-Aguera

	vec3 Fr = max(vec3(1.0 - roughness), F0) - F0;
	vec3 k_S = F0 + Fr * pow(1.0 - NdotV, 5.0);
	vec3 FssEss = k_S * f_ab.x + f_ab.y;  // <--- GGX / specular light contribution (scale it down if the specularWeight is low)

	// Multiple scattering, from Fdez-Aguera
	float Ems = (1.0 - (f_ab.x + f_ab.y));
	vec3  F_avg = (F0 + (vec3(1.0) - F0) / vec3(21.0));
	vec3  FmsEms = Ems * FssEss * F_avg / (vec3(1.0) - F_avg * Ems);
	vec3 k_D = diffuseColor * (vec3(1.0) - FssEss + FmsEms);  // we use +FmsEms as indicated by the formula in the blog post (might be a typo in the implementation)

	return (FmsEms + k_D) * irradiance;
#else
#endif 
}

#endif

float applyIorToRoughness(float roughness, float ior)
{
	// Scale roughness with IOR so that an IOR of 1.0 results in no microfacet refraction and
	// an IOR of 1.5 results in the default amount of microfacet refraction.
	return roughness * clamp(ior * 2.0 - 2.0, 0.0, 1.0);
}
 

vec3 doPbrLighting(VS2PS Input, PerFrame perFrame, gpuMaterial m)
{
	vec3 outColor = vec3(m.baseColor);
#ifdef __cplusplus
	pbrMaterial u_pbrParams = {};
	auto myPerFrame = perFrame;
#endif
#ifdef MATERIAL_UNLIT
	return outColor;
#endif

	// Roughness is authored as perceptual roughness; as is convention,
	// convert to material roughness by squaring the perceptual roughness [2].
	float alphaRoughness = m.perceptualRoughness * m.perceptualRoughness;
	vec3 specularColor = m.specularColorlight;
	// Compute reflectance.
	float reflectance = max(max(specularColor.r, specularColor.g), specularColor.b);

	vec3 specularEnvironmentR0 = specularColor;
	// Anything less than 2% is physically impossible and is instead considered to be shadowing. Compare to "Real-Time-Rendering" 4th editon on page 325.
	vec3 specularEnvironmentR90 = vec3(clamp(reflectance * 50.0, 0.0, 1.0));

	m.reflectance0 = specularEnvironmentR0;
	m.alphaRoughness = alphaRoughness;
	m.reflectance90 = specularEnvironmentR90;
	//materialInfo.u_ModelMatrix = myPerFrame.u_mCameraCurrViewProjInverse;

	// LIGHTING

	float clearcoatFactor = 0.0;
	vec3 clearcoatFresnel = vec3(0);

	vec3 color = vec3(0.0, 0.0, 0.0);
	vec3 normal = getPixelNormal(Input);
	vec3 worldPos = Input.WorldPos;
	vec3 f90 = vec3(1.0f);
#ifdef __cplusplus
	vec3 cpos = myPerFrame.u_CameraPos;
	vec3 view = normalize(cpos - worldPos);
#else
	vec3 view = normalize(myPerFrame.u_CameraPos.xyz - worldPos);
#endif

#if (DEF_doubleSided == 1)
	if (dot(normal, view) < 0)
	{
		normal = -normal;
	}
#endif

#ifdef USE_PUNCTUAL
	vec3 c = vec3(0.0, 0.0, 0.0);
	for (int i = 0; i < myPerFrame.u_lightCount; ++i)
	{
		Light light = myPerFrame.u_lights[i];

		float shadowFactor = DoSpotShadow(worldPos, light);
		LightContrib lc;
		if (light.type == LightType_Directional)
		{
			lc = applyDirectionalLight(light, m, normal, view); lc.intensity *= shadowFactor;
		}
		else if (light.type == LightType_Point)
		{
			lc = applyPointLight(light, m, normal, worldPos, view);
		}
		else if (light.type == LightType_Spot)
		{
			lc = applySpotLight(light, m, normal, worldPos, view); lc.intensity *= shadowFactor;
		}
		color += lc.intensity;
		//BsdfEvaluateData evalData;
		//evalData.xi = vec3(0, 0, 0);
		//evalData.k1 = view;
		//evalData.k2 = lc.incidentVector;
		//vec3 l = normalize(lc.pointToLight);
		//vec3 l_diffuse = vec3(0.0);

		//int lobe = bsdfEvaluate(evalData, m);
		//evalData.bsdf_diffuse = vec3(1, 1, 1);
		//evalData.bsdf_glossy = vec3(0, 0, 0);

		//const vec3 w = lc.intensity;
		//c += w * evalData.bsdf_diffuse;
		//c += w * evalData.bsdf_glossy;
	}
#endif

#if 1
	// Calculate lighting contribution from image based lighting source (IBL)
#ifdef USE_IBL
	color += getIBLContribution(m, normal, view) * myPerFrame.u_iblFactor * GetSSAO(gl_FragCoord.xy * myPerFrame.u_invScreenResolution);
#endif
	 

	// Apply optional PBR terms for additional (optional) shading
#ifdef ID_occlusionTexture 
	color = color * m.ao; //mix(color, color * ao, myPerFrame.u_OcclusionStrength);
#endif
	color += m.emissive * (vec3(1.0) - clearcoatFactor * clearcoatFresnel);
#endif



#ifndef DEBUG_OUTPUT // no debug 
	// regular shading
	outColor = color;
#else // debug output

#ifdef DEBUG_METALLIC
	outColor.rgb = vec3(metallic);
#endif

#ifdef DEBUG_ROUGHNESS
	outColor.rgb = vec3(perceptualRoughness);
#endif

#ifdef DEBUG_NORMAL
#ifdef ID_normalTexCoord
	outColor.rgb = texture(u_NormalSampler, getNormalUV()).rgb;
#else
	outColor.rgb = vec3(0.5, 0.5, 1.0);
#endif
#endif

#ifdef DEBUG_BASECOLOR
	outColor.rgb = (baseColor.rgb);
#endif

#ifdef DEBUG_OCCLUSION
	outColor.rgb = vec3(ao);
#endif

#ifdef DEBUG_EMISSIVE
	outColor.rgb = (emissive);
#endif

#ifdef DEBUG_F0
	outColor.rgb = vec3(f0);
#endif

#ifdef DEBUG_ALPHA
	outColor.rgb = vec3(baseColor.a);
#endif

#endif // !DEBUG_OUTPUT 
	return outColor;
}

#ifdef USE_IBL
vec3 getIBLContribution(MaterialInfo materialInfo, vec3 n, vec3 v)
{
	float NdotV = clamp(dot(n, v), 0.0, 1.0);

	float u_MipCount = 9.0; // resolution of 512x512 of the IBL
	float lod = clamp(materialInfo.perceptualRoughness * float(u_MipCount), 0.0, float(u_MipCount));
	vec3 reflection = normalize(reflect(-v, n));

	vec2 brdfSamplePoint = clamp(vec2(NdotV, materialInfo.perceptualRoughness), vec2(0.0, 0.0), vec2(1.0, 1.0));
	// retrieve a scale and bias to F0. See [1], Figure 3
	vec2 brdf = texture(u_brdfLUT, brdfSamplePoint).rg;

	vec3 diffuseLight = texture(u_DiffuseEnvSampler, n).rgb;

#ifdef USE_TEX_LOD
	vec3 specularLight = textureLod(u_SpecularEnvSampler, reflection, lod).rgb;
#else
	vec3 specularLight = texture(u_SpecularEnvSampler, reflection).rgb;
#endif

	vec3 diffuse = diffuseLight * materialInfo.diffuseColor;
	vec3 specular = specularLight * (materialInfo.specularColor * brdf.x + brdf.y);

	return diffuse + specular;
}
#endif

// Lambert lighting
// see https://seblagarde.wordpress.com/2012/01/08/pi-or-not-to-pi-in-game-lighting-equation/
vec3 diffuse(MaterialInfo materialInfo)
{
	return materialInfo.diffuseColor / M_PI;
}

// The following equation models the Fresnel reflectance term of the spec equation (aka F())
// Implementation of fresnel from [4], Equation 15
vec3 specularReflection(MaterialInfo materialInfo, AngularInfo angularInfo)
{
	return materialInfo.reflectance0 + (materialInfo.reflectance90 - materialInfo.reflectance0) * pow(clamp(1.0 - angularInfo.VdotH, 0.0, 1.0), 5.0);
}

// Smith Joint GGX
// Note: Vis = G / (4 * NdotL * NdotV)
// see Eric Heitz. 2014. Understanding the Masking-Shadowing Function in Microfacet-Based BRDFs. Journal of Computer Graphics Techniques, 3
// see Real-Time Rendering. Page 331 to 336.
// see https://google.github.io/filament/Filament.md.html#materialsystem/specularbrdf/geometricshadowing(specularg)
float visibilityOcclusion(MaterialInfo materialInfo, AngularInfo angularInfo)
{
	float NdotL = angularInfo.NdotL;
	float NdotV = angularInfo.NdotV;
	float alphaRoughnessSq = materialInfo.alphaRoughness * materialInfo.alphaRoughness;

	float GGXV = NdotL * sqrt(NdotV * NdotV * (1.0 - alphaRoughnessSq) + alphaRoughnessSq);
	float GGXL = NdotV * sqrt(NdotL * NdotL * (1.0 - alphaRoughnessSq) + alphaRoughnessSq);

	float GGX = GGXV + GGXL;
	if (GGX > 0.0)
	{
		return 0.5 / GGX;
	}
	return 0.0;
}
float microfacetDistribution(MaterialInfo materialInfo, AngularInfo angularInfo)
{
	float alphaRoughnessSq = materialInfo.alphaRoughness * materialInfo.alphaRoughness;
	float f = (angularInfo.NdotH * alphaRoughnessSq - angularInfo.NdotH) * angularInfo.NdotH + 1.0;
	return alphaRoughnessSq / (M_PI * f * f + 0.000001f);
}

vec3 getPointShade(vec3 pointToLight, MaterialInfo materialInfo, vec3 normal, vec3 view)
{
	AngularInfo angularInfo = getAngularInfo(pointToLight, normal, view);

	if (angularInfo.NdotL > 0.0 || angularInfo.NdotV > 0.0)
	{
		// Calculate the shading terms for the microfacet specular shading model
		vec3 F = specularReflection(materialInfo, angularInfo);
		float Vis = visibilityOcclusion(materialInfo, angularInfo);
		float D = microfacetDistribution(materialInfo, angularInfo);

		// Calculation of analytical lighting contribution
#ifndef __cplusplus
		vec3 diffuseContrib = (1.0 - F) * diffuse(materialInfo);
#else
		vec3 f1 = vec3(1, 1, 1) - F;
		vec3 diffuseContrib = f1 * diffuse(materialInfo);
#endif // __cplusplus 
		vec3 specContrib = F * Vis * D;

		// Obtain final intensity as reflectance (BRDF) scaled by the energy of the light (cosine law)
		return angularInfo.NdotL * (diffuseContrib + specContrib);
	}

	return vec3(0.0, 0.0, 0.0);
}
vec3 applyDirectionalLight(Light light, MaterialInfo materialInfo, vec3 normal, vec3 view)
{
	vec3 pointToLight = light.direction;
	vec3 shade = getPointShade(pointToLight, materialInfo, normal, view);
	return light.intensity * light.color * shade;
}

vec3 applyPointLight(Light light, MaterialInfo materialInfo, vec3 normal, vec3 worldPos, vec3 view)
{
	vec3 pointToLight = light.position - worldPos;
	float distance = length(pointToLight);
	float attenuation = getRangeAttenuation(light.range, distance);
	vec3 shade = getPointShade(pointToLight, materialInfo, normal, view);
	return attenuation * light.intensity * light.color * shade;
}

vec3 applySpotLight(Light light, MaterialInfo materialInfo, vec3 normal, vec3 worldPos, vec3 view)
{
	vec3 pointToLight = light.position - worldPos;
	float distance = length(pointToLight);
	float rangeAttenuation = getRangeAttenuation(light.range, distance);
	float spotAttenuation = getSpotAttenuation(pointToLight, -light.direction, light.outerConeCos, light.innerConeCos);
	vec3 shade = getPointShade(pointToLight, materialInfo, normal, view);
	return rangeAttenuation * spotAttenuation * light.intensity * light.color * shade;
}

vec3 doPbrLighting_old(VS2PS Input, PerFrame perFrame, vec2 uv, vec3 diffuseColor, vec3 specularColor, float perceptualRoughness, vec4 baseColor)
{
#ifdef __cplusplus
	vec3 outColor = baseColor;
#else
	vec3 outColor = baseColor.rgb;
#endif
#ifdef MATERIAL_UNLIT 
	return outColor;
#endif

	// Roughness is authored as perceptual roughness; as is convention,
	// convert to material roughness by squaring the perceptual roughness [2].
	float alphaRoughness = perceptualRoughness * perceptualRoughness;

	// Compute reflectance.
	float reflectance = max(max(specularColor.r, specularColor.g), specularColor.b);

	vec3 specularEnvironmentR0 = specularColor;
	// Anything less than 2% is physically impossible and is instead considered to be shadowing. Compare to "Real-Time-Rendering" 4th editon on page 325.
	vec3 specularEnvironmentR90 = vec3(clamp(reflectance * 50.0, 0.0, 1.0));

	MaterialInfo materialInfo;
	materialInfo.perceptualRoughness = perceptualRoughness;
	materialInfo.reflectance0 = specularEnvironmentR0;
	materialInfo.alphaRoughness = alphaRoughness;
	materialInfo.diffuseColor = diffuseColor;
	materialInfo.reflectance90 = specularEnvironmentR90;
	materialInfo.specularColor = specularColor;

	// LIGHTING

	vec3 color = vec3(0.0, 0.0, 0.0);
	vec3 normal = getPixelNormal(Input);
	vec3 worldPos = Input.WorldPos;
#ifdef __cplusplus
	vec3 cpos = myPerFrame.u_CameraPos;
	vec3 view = normalize(cpos - worldPos);
#else
	vec3 view = normalize(myPerFrame.u_CameraPos.xyz - worldPos);
#endif

#if (DEF_doubleSided == 1)
	if (dot(normal, view) < 0)
	{
		normal = -normal;
	}
#endif

#ifdef USE_PUNCTUAL
	for (int i = 0; i < myPerFrame.u_lightCount; ++i)
	{
		Light light = myPerFrame.u_lights[i];
		vec3 c = vec3(0);
		float shadowFactor = DoSpotShadow(worldPos, light);
		if (light.type == LightType_Directional)
		{
			c = applyDirectionalLight(light, materialInfo, normal, view);
		}
		else if (light.type == LightType_Point)
		{
			c = applyPointLight(light, materialInfo, normal, worldPos, view);
		}
		else if (light.type == LightType_Spot)
		{
			c = applySpotLight(light, materialInfo, normal, worldPos, view);
		}
		color += c * shadowFactor;
	}
#endif

	// Calculate lighting contribution from image based lighting source (IBL)
#ifdef USE_IBL
	color += getIBLContribution(materialInfo, normal, view) * myPerFrame.u_iblFactor * GetSSAO(gl_FragCoord.xy * perFrame.u_invScreenResolution);
#endif

	float ao = 1.0;
	// Apply optional PBR terms for additional (optional) shading
#ifdef ID_occlusionTexture
	ao = texture(u_OcclusionSampler, uv).r;// getOcclusionUV(Input)).r;
	color = color * ao; //mix(color, color * ao, myPerFrame.u_OcclusionStrength);
#endif

	vec3 emissive = vec3(0);
#ifdef ID_emissiveTexture
	emissive = (texture(u_EmissiveSampler, uv)).rgb * u_pbrParams.emissiveFactor.rgb * myPerFrame.u_EmissiveFactor;
#else
	emissive = u_pbrParams.emissiveFactor * perFrame.u_EmissiveFactor;
#endif
	color += emissive;

#ifndef DEBUG_OUTPUT // no debug

	// regular shading
	outColor = color;

#else // debug output

#ifdef DEBUG_METALLIC
	outColor.rgb = vec3(metallic);
#endif

#ifdef DEBUG_ROUGHNESS
	outColor.rgb = vec3(perceptualRoughness);
#endif

#ifdef DEBUG_NORMAL
#ifdef ID_normalTexCoord
	outColor.rgb = texture(u_NormalSampler, uv).rgb;
#else
	outColor.rgb = vec3(0.5, 0.5, 1.0);
#endif
#endif

#ifdef DEBUG_BASECOLOR
	outColor.rgb = (baseColor.rgb);
#endif

#ifdef DEBUG_OCCLUSION
	outColor.rgb = vec3(ao);
#endif

#ifdef DEBUG_EMISSIVE
	outColor.rgb = (emissive);
#endif

#ifdef DEBUG_F0
	outColor.rgb = vec3(f0);
#endif

#ifdef DEBUG_ALPHA
	outColor.rgb = vec3(baseColor.a);
#endif

#endif // !DEBUG_OUTPUT

	return outColor;
}



