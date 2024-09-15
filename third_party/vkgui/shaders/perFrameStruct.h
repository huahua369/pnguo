// AMD Cauldron code
//
// Copyright(c) 2018 Advanced Micro Devices, Inc.All rights reserved.
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

// KHR_lights_punctual extension.
// see https://github.com/KhronosGroup/glTF/tree/master/extensions/2.0/Khronos/KHR_lights_punctual
#ifndef MAX_LIGHT_INSTANCES
#define MAX_LIGHT_INSTANCES  80
#endif
#define MAX_SHADOW_INSTANCES 32

struct Light
{
	mat4          mLightViewProj;
	mat4          mLightView;

	vec3          direction;
	float         range;            // 方向光的角度大小，聚光灯和点光源为 1/范围

	vec3          color;
	float         intensity;        // 光照度
	vec3          position;
	// spot
	float         innerConeCos;
	float         outerConeCos;
	int           type;
	float         depthBias;
	int           shadowMapIndex;
};

const int LightType_Directional = 0;
const int LightType_Point = 1;
const int LightType_Spot = 2;

struct PerFrame
{
	mat4          u_mCameraCurrViewProj;
	mat4          u_mCameraPrevViewProj;
	mat4          u_mCameraCurrViewProjInverse;

	vec4          u_CameraPos;
	float         u_iblFactor;
	float         u_EmissiveFactor;
	vec2          u_invScreenResolution;

	vec4          u_WireframeOptions;
	float         u_LodBias;
	int           useSky;  // 0: hdr, 1: sky 
	float		  envRotation;
	int           u_lightCount;
	Light         u_lights[MAX_LIGHT_INSTANCES];
};

//------------------------------------------------------------
// PBR getters
//------------------------------------------------------------

// KHR_materials_pbrSpecularGlossiness
// KHR_materials_unlit
// KHR_materials_ior
// KHR_materials_transmission
// KHR_materials_volume 
// KHR_materials_anisotropy
// KHR_materials_clearcoat
// KHR_materials_specular
// KHR_materials_iridescence
// KHR_materials_sheen 
// KHR_texture_transform

struct pbr0Material
{
	// pbrMetallicRoughness
	vec4  baseColorFactor;
	vec3  emissiveFactor;
	float metallicFactor;

	float roughnessFactor;
	float normalScale;// normalTextureScale;
	int   alphaMode;
	float alphaCutoff;
	// int   pbrBaseColorTexture;   
	// int   normalTexture;           
	// int   pbrMetallicRoughnessTexture;  
	// int   emissiveTexture;      


	// KHR_materials_pbrSpecularGlossiness
	vec4 pbrDiffuseFactor;
	vec3 pbrSpecularFactor;
	float glossinessFactor;

	// KHR_materials_unlit
	int unlit;

	// KHR_materials_ior
	float ior;

	// KHR_materials_transmission
	float transmissionFactor;
	//int   transmissionTexture;   

	// KHR_materials_volume 
	float attenuationDistance;
	vec3  attenuationColor;
	float thicknessFactor;
	//int   thicknessTexture;   

	// KHR_materials_anisotropy
	float anisotropyStrength;
	//int   anisotropyTexture; 
	float anisotropyRotation;

	// KHR_materials_clearcoat
	float clearcoatFactor;
	float clearcoatRoughness;
	//int   clearcoatTexture;            
	//int   clearcoatRoughnessTexture;  
	//int   clearcoatNormalTexture;     

	// KHR_materials_specular
	vec3  specularColorFactor;
	float specularFactor;
	//int   specularTexture;        
	//int   specularColorTexture;  

	// KHR_materials_iridescence
	float iridescenceFactor;
	//int   iridescenceTexture;         
	float iridescenceThicknessMinimum;  // 100
	float iridescenceThicknessMaximum;  // 400
	//int   iridescenceThicknessTexture;   
	float iridescenceIor;               // 1.3

	// KHR_materials_sheen 
	vec3  sheenColorFactor;
	float sheenRoughnessFactor;
	//int   sheenColorTexture;     
	//int   sheenRoughnessTexture;  
	//KHR_materials_dispersion
	float dispersion;
	float p0;
	vec2 transmissionFramebufferSize;

	// KHR_texture_transform
#ifdef m34
	mat3x4 uvTransform;
#else
	mat3 uvTransform;
#endif

};

struct pbrMaterial
{
	// pbrMetallicRoughness
	vec4 baseColorFactor;
	float metallicFactor;
	float roughnessFactor;
	float normalScale;
	// KHR_materials_emissive_strength
	float emissiveStrength;

	vec3 emissiveFactor;
	int   alphaMode;

	float alphaCutoff;
	float occlusionStrength;
	//KHR_materials_ior
	float ior;
	int mipCount;

	// KHR_materials_pbrSpecularGlossiness
	vec4 pbrDiffuseFactor;
	vec3 pbrSpecularFactor;
	float glossinessFactor;


	// Specular KHR_materials_specular
	vec3  specularColorFactor;
	float specularFactor;
	// KHR_materials_sheen 
	vec3 sheenColorFactor;
	float sheenRoughnessFactor;

	vec3 anisotropy;
	// KHR_materials_transmission
	float transmissionFactor;

	ivec2 transmissionFramebufferSize;
	float thicknessFactor;
	float diffuseTransmissionFactor;

	vec3 diffuseTransmissionColorFactor;
	//KHR_materials_dispersion
	float dispersion;

	vec3 attenuationColor;
	float attenuationDistance;

	// Iridescence
	float iridescenceFactor;
	float iridescenceIor;
	float iridescenceThicknessMinimum;
	float iridescenceThicknessMaximum;

	float clearcoatFactor;
	float clearcoatRoughness;
	float clearcoatNormalScale;
	float envIntensity;

	int unlit;
	float pad[3];
	mat3 uvTransform;
};

struct PBRFactors
{
	// pbrMetallicRoughness
	vec4 u_BaseColorFactor;
	float u_MetallicFactor;
	float u_RoughnessFactor;

	float u_AttenuationDistance;//KHR_materials_volume
	float u_ThicknessFactor;
	vec3 u_AttenuationColor;

	float u_TransmissionFactor;	//KHR_materials_transmission
	vec3 u_EmissiveFactor;
	float pad0;
	// KHR_materials_pbrSpecularGlossiness
	vec4 diffuseFactor;
	vec3 specularFactor;
	float glossinessFactor;

};



// 内部pbr用
struct MaterialInfo
{
	vec4 baseColor;
	float perceptualRoughness;    // roughness value, as authored by the model creator (input to shader)
	float alphaRoughness;         // roughness mapped to a more linear change in the roughness (proposed by [2])
	float alpha;
	vec3 reflectance0;            // full reflectance color (normal incidence angle)

	vec3 diffuseColor;            // color contribution from diffuse lighting

	vec3 reflectance90;           // reflectance color at grazing angle
	vec3 specularColor;           // color contribution from specular lighting

};
struct gpuMaterial
{
	vec3 reflectance0;            // full reflectance color (normal incidence angle)
	float perceptualRoughness;    // roughness value, as authored by the model creator (input to shader)

	vec3 diffuseColor;            // color contribution from diffuse lighting
	float alphaRoughness;         // roughness mapped to a more linear change in the roughness (proposed by [2])

	vec3 reflectance90;           // reflectance color at grazing angle
	float alpha;
	vec3 specularColorlight;           // color contribution from specular lighting
	float dispersion;

	vec4  baseColor;  // base color 
	vec2  roughness;  // 0 = smooth, 1 = rough (anisotropic: x = U, y = V)

	vec3 f90;                       // reflectance color at grazing angle
	vec3 f90_dielectric;

	float metallic;   // 0 = dielectric, 1 = metallic
	vec3  emissive;   // emissive color

	vec3 N;   // shading normal
	vec3 T;   // shading normal
	vec3 B;   // shading normal
	vec3 Ng;  // geometric normal

	vec2 uv;

	float ior1;  // index of refraction : current medium (i.e. air)
	float ior2;  // index of refraction : the other side (i.e. glass)

	float ior;
	vec3 f0_dielectric;

	float specularWeight; // product of specularFactor and specularTexture.a

	float specular;       // weight of the dielectric specular layer
	vec3  specularColor;  // color of the dielectric specular layer
	float transmission;   // KHR_materials_transmission

	vec3  attenuationColor;     // KHR_materials_volume
	float attenuationDistance;  //
	float thickness;            // Replace for isThinWalled?

	// KHR_materials_clearcoat
	vec3 clearcoatF0;
	vec3 clearcoatF90;
	float clearcoatFactor;
	vec3 clearcoatNormal;
	float clearcoatRoughness;

	float iridescence;
	float iridescenceIor;
	float iridescenceThickness;

	vec3  sheenColor;
	float sheenRoughness;
	float ao;

};

// alphaMode
#define ALPHA_OPAQUE 0
#define ALPHA_MASK 1
#define ALPHA_BLEND 2
 
struct MeshState
{
	vec3 N;   // Normal
	vec3 T;   // Tangent
	vec3 B;   // Bitangent
	vec3 Ng;  // Geometric normal
	vec2 tc;  // Texture coordinates
	float emissiveFactor;
	bool isInside;
};
#define MICROFACET_MIN_ROUGHNESS 0.0014142f

struct NormalInfo {
	vec3 ng;   // Geometry normal
	vec3 t;    // Geometry tangent
	vec3 b;    // Geometry bitangent
	vec3 n;    // Shading normal
	vec3 ntex; // Normal from texture, scaling is accounted for.
};