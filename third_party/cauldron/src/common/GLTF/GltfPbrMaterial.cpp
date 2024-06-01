// AMD Cauldron code
// 
// Copyright(c) 2020 Advanced Micro Devices, Inc.All rights reserved.
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

#include "stdafx.h"

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "GltfPbrMaterial.h"
#include "GltfHelpers.h"

template <typename T>
const T& tinygltf::Value::Get() const {
	bool sav = std::is_same<T, glm::vec4>::value;
	T r = {};
	if (sav)
	{
		for (int i = 0; i < array_value_.size() && i < 4; i++)
		{
			r[i] = array_value_[i].GetNumberAsDouble();
		}
	}
	return r;
}
//
// Set some default parameters 
//

void SetDefaultMaterialParamters(PBRMaterialParameters* pPbrMaterialParameters)
{
	pPbrMaterialParameters->m_doubleSided = false;
	pPbrMaterialParameters->m_blending = false;

	pPbrMaterialParameters->m_params.m.m_emissiveFactor = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
	pPbrMaterialParameters->m_params.m.m_baseColorFactor = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
	pPbrMaterialParameters->m_params.m.m_metallicRoughnessValues = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
	pPbrMaterialParameters->m_params.m.m_specularGlossinessFactor = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
}


void SetDefaultMaterialParamters1(PBRMaterialParameters* pPbrMaterialParameters)
{
	pPbrMaterialParameters->m_doubleSided = false;
	pPbrMaterialParameters->m_blending = false;

	pPbrMaterialParameters->m_params.g.m_emissiveFactor = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
	pPbrMaterialParameters->m_params.g.m_baseColorFactor = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
	pPbrMaterialParameters->m_params.g.m_metallicRoughnessValues = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
	pPbrMaterialParameters->m_params.g.m_specularGlossinessFactor = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
}

bool ProcessGetTextureIndexAndTextCoord(const json::object_t& material, const std::string& textureName, int* pIndex, int* pTexCoord)
{
	if (pIndex)
	{
		*pIndex = -1;
		std::string strIndex = textureName + "/index";
		*pIndex = GetElementInt(material, strIndex.c_str(), -1);
		if (*pIndex == -1)
			return false;
	}

	if (pTexCoord)
	{
		*pTexCoord = -1;
		std::string strTexCoord = textureName + "/texCoord";
		*pTexCoord = GetElementInt(material, strTexCoord.c_str(), 0);
	}

	return true;
}
#if 1
void ProcessMaterials(const json::object_t& material, PBRMaterialParameters* tfmat, std::map<std::string, int>& textureIds)
{
	// Load material constants
	//
	json::array_t ones = { 1.0, 1.0, 1.0, 1.0 };
	json::array_t zeroes = { 0.0, 0.0, 0.0, 0.0 };
	tfmat->m_doubleSided = GetElementBoolean(material, "doubleSided", false);
	tfmat->m_blending = GetElementString(material, "alphaMode", "OPAQUE") == "BLEND";
	tfmat->m_params.m.m_emissiveFactor = GetVector(GetElementJsonArray(material, "emissiveFactor", zeroes));

	tfmat->m_defines["DEF_doubleSided"] = std::to_string(tfmat->m_doubleSided ? 1 : 0);
	tfmat->m_defines["DEF_alphaCutoff"] = std::to_string(GetElementFloat(material, "alphaCutoff", 0.5));
	tfmat->m_defines["DEF_alphaMode_" + GetElementString(material, "alphaMode", "OPAQUE")] = std::to_string(1);

	// look for textures and store their IDs in a map 
	//
	int index, texCoord;

	if (ProcessGetTextureIndexAndTextCoord(material, "normalTexture", &index, &texCoord))
	{
		textureIds["normalTexture"] = index;
		tfmat->m_defines["ID_normalTexCoord"] = std::to_string(texCoord);
	}

	if (ProcessGetTextureIndexAndTextCoord(material, "emissiveTexture", &index, &texCoord))
	{
		textureIds["emissiveTexture"] = index;
		tfmat->m_defines["ID_emissiveTexCoord"] = std::to_string(texCoord);
	}

	if (ProcessGetTextureIndexAndTextCoord(material, "occlusionTexture", &index, &texCoord))
	{
		textureIds["occlusionTexture"] = index;
		tfmat->m_defines["ID_occlusionTexCoord"] = std::to_string(texCoord);
	}

	// If using pbrMetallicRoughness
	//
	auto pbrMetallicRoughnessIt = material.find("pbrMetallicRoughness");
	if (pbrMetallicRoughnessIt != material.end())
	{
		const json& pbrMetallicRoughness = pbrMetallicRoughnessIt->second;

		tfmat->m_defines["MATERIAL_METALLICROUGHNESS"] = "1";

		float metallicFactor = GetElementFloat(pbrMetallicRoughness, "metallicFactor", 1.0);
		float roughnessFactor = GetElementFloat(pbrMetallicRoughness, "roughnessFactor", 1.0);
		tfmat->m_params.m.m_metallicRoughnessValues = glm::vec4(metallicFactor, roughnessFactor, 0, 0);
		tfmat->m_params.m.m_baseColorFactor = GetVector(GetElementJsonArray(pbrMetallicRoughness, "baseColorFactor", ones));

		if (ProcessGetTextureIndexAndTextCoord(pbrMetallicRoughness, "baseColorTexture", &index, &texCoord))
		{
			textureIds["baseColorTexture"] = index;
			tfmat->m_defines["ID_baseTexCoord"] = std::to_string(texCoord);
		}

		if (ProcessGetTextureIndexAndTextCoord(pbrMetallicRoughness, "metallicRoughnessTexture", &index, &texCoord))
		{
			textureIds["metallicRoughnessTexture"] = index;
			tfmat->m_defines["ID_metallicRoughnessTexCoord"] = std::to_string(texCoord);
		}
	}
	else
	{
		// If using KHR_materials_pbrSpecularGlossiness
		//
		auto extensionsIt = material.find("extensions");
		if (extensionsIt != material.end())
		{
			const json& extensions = extensionsIt->second;
			auto KHR_materials_pbrSpecularGlossinessIt = extensions.find("KHR_materials_pbrSpecularGlossiness");
			if (KHR_materials_pbrSpecularGlossinessIt != extensions.end())
			{
				const json& pbrSpecularGlossiness = KHR_materials_pbrSpecularGlossinessIt.value();

				tfmat->m_defines["MATERIAL_SPECULARGLOSSINESS"] = "1";

				float glossiness = GetElementFloat(pbrSpecularGlossiness, "glossinessFactor", 1.0);
				tfmat->m_params.m.m_DiffuseFactor = GetVector(GetElementJsonArray(pbrSpecularGlossiness, "diffuseFactor", ones));
				tfmat->m_params.m.m_specularGlossinessFactor = glm::vec4(glm::vec3(GetVector(GetElementJsonArray(pbrSpecularGlossiness, "specularFactor", ones))), glossiness);

				if (ProcessGetTextureIndexAndTextCoord(pbrSpecularGlossiness, "diffuseTexture", &index, &texCoord))
				{
					textureIds["diffuseTexture"] = index;
					tfmat->m_defines["ID_diffuseTexCoord"] = std::to_string(texCoord);
				}

				if (ProcessGetTextureIndexAndTextCoord(pbrSpecularGlossiness, "specularGlossinessTexture", &index, &texCoord))
				{
					textureIds["specularGlossinessTexture"] = index;
					tfmat->m_defines["ID_specularGlossinessTexCoord"] = std::to_string(texCoord);
				}
			}
		}
	}
}
#endif
bool DoesMaterialUseSemantic(DefineList& defines, const std::string semanticName)
{
	// search if any *TexCoord mentions this channel
	//
	if (semanticName.substr(0, 9) == "TEXCOORD_")
	{
		char id = semanticName[9];

		for (auto def : defines)
		{
			uint32_t size = static_cast<uint32_t>(def.first.size());
			if (size <= 8)
				continue;

			if (def.first.substr(size - 8) == "TexCoord")
			{
				if (id == def.second.c_str()[0])
				{
					return true;
				}
			}
		}
		return false;
	}

	return false;
}

//
// Identify what material uses this texture, this helps:
// 1) determine the color space if the texture and also the cut out level. Authoring software saves albedo and emissive images in SRGB mode, the rest are linear mode
// 2) tell the cutOff value, to prevent thinning of alpha tested PNGs when lower mips are used. 
//
#if 1
void GetSrgbAndCutOffOfImageGivenItsUse(int imageIndex, const json& materials, bool* pSrgbOut, float* pCutoff)
{
	*pSrgbOut = false;
	*pCutoff = 1.0f; // no cutoff

	for (int m = 0; m < materials.size(); m++)
	{
		const json& material = materials[m];

		if (GetElementInt(material, "pbrMetallicRoughness/baseColorTexture/index", -1) == imageIndex)
		{
			*pSrgbOut = true;

			*pCutoff = GetElementFloat(material, "alphaCutoff", 0.5);

			return;
		}

		if (GetElementInt(material, "extensions/KHR_materials_pbrSpecularGlossiness/specularGlossinessTexture/index", -1) == imageIndex)
		{
			*pSrgbOut = true;
			return;
		}

		if (GetElementInt(material, "extensions/KHR_materials_pbrSpecularGlossiness/diffuseTexture/index", -1) == imageIndex)
		{
			*pSrgbOut = true;
			return;
		}

		if (GetElementInt(material, "emissiveTexture/index", -1) == imageIndex)
		{
			*pSrgbOut = true;
			return;
		}
	}
}
#endif
template<class T>
glm::vec4 tov4(const T& v)
{
	return glm::vec4(v[0], v[1], v[2], v.size() > 3 ? v[3] : 0);
}

void ProcessMaterials(tinygltf::Material* pm, PBRMaterialParameters* tfmat, std::map<std::string, int>& textureIds)
{
	//njson material;
	// Load material constants
	auto& material = *pm;
	glm::vec4 ones = { 1.0, 1.0, 1.0, 1.0 };
	glm::vec4 zeroes = { 0.0, 0.0, 0.0, 0.0 };
	tfmat->m_doubleSided = material.doubleSided;
	tfmat->m_blending = material.alphaMode == "BLEND";
	tfmat->m_params.g.m_emissiveFactor = (material.emissiveFactor.size()) ? tov4(material.emissiveFactor) : zeroes;
	tfmat->m_defines["DEF_doubleSided"] = std::to_string(tfmat->m_doubleSided ? 1 : 0);
	tfmat->m_defines["DEF_alphaCutoff"] = std::to_string(material.alphaCutoff);
	tfmat->m_defines["DEF_alphaMode_" + material.alphaMode] = std::to_string(1);

	// look for textures and store their IDs in a map 
	//
	if (material.normalTexture.index != -1)
	{
		textureIds["normalTexture"] = material.normalTexture.index;
		tfmat->m_defines["ID_normalTexCoord"] = std::to_string(material.normalTexture.texCoord);
	}
	if (material.emissiveTexture.index != -1)
	{
		textureIds["emissiveTexture"] = material.emissiveTexture.index;
		tfmat->m_defines["ID_emissiveTexCoord"] = std::to_string(material.emissiveTexture.texCoord);
	}
	if (material.occlusionTexture.index != -1)
	{
		textureIds["occlusionTexture"] = material.occlusionTexture.index;
		tfmat->m_defines["ID_occlusionTexCoord"] = std::to_string(material.occlusionTexture.texCoord);
	}


	// If using pbrMetallicRoughness
	//
	auto pbrMetallicRoughness = material.pbrMetallicRoughness;// .find("pbrMetallicRoughness");

	do
	{
		if (material.extensions.size())
		{
			// If using KHR_materials_pbrSpecularGlossiness
			//
			auto extensionsIt = material.extensions;// .find("extensions");
			if (extensionsIt.size())
			{
				auto extensions = extensionsIt;
				auto KHR_materials_pbrSpecularGlossinessIt = extensions.find("KHR_materials_pbrSpecularGlossiness");
				if (KHR_materials_pbrSpecularGlossinessIt != extensions.end() && (pbrMetallicRoughness.baseColorTexture.index == -1))
				{
					auto pbrSpecularGlossiness = KHR_materials_pbrSpecularGlossinessIt->second;

					tfmat->m_defines["MATERIAL_SPECULARGLOSSINESS"] = "1";
					float glossiness = 1.0;
					if (pbrSpecularGlossiness.Has("glossinessFactor"))
					{
						glossiness = pbrSpecularGlossiness.Get("glossinessFactor").GetNumberAsDouble();
					}
					if (pbrSpecularGlossiness.Has("diffuseFactor"))
						tfmat->m_params.g.m_DiffuseFactor = pbrSpecularGlossiness.Get("diffuseFactor").Get<glm::vec4>();
					else
						tfmat->m_params.g.m_DiffuseFactor = ones;
					glm::vec4 specularFactor = ones;
					if (pbrSpecularGlossiness.Has("specularFactor"))
						specularFactor = pbrSpecularGlossiness.Get("specularFactor").Get<glm::vec4>();
					specularFactor.w = glossiness;

					auto itcb = [=, &textureIds](const std::string& k, const std::string& kd) {
						if (pbrSpecularGlossiness.Has(k))
						{
							auto t = pbrSpecularGlossiness.Get(k);
							int index = -1;
							int texCoord = 0;
							if (t.Has("index"))
							{
								index = t.Get("index").GetNumberAsInt();
								if (index != -1)
								{
									textureIds[k] = index;
									if (t.Has("texCoord"))
									{
										texCoord = t.Get("texCoord").GetNumberAsInt();
									}
									tfmat->m_defines[kd] = std::to_string(texCoord);
								}
							}
						}
					};

					itcb("diffuseTexture", "ID_diffuseTexCoord");
					itcb("specularGlossinessTexture", "ID_specularGlossinessTexCoord");
					break;
				}
			}
		}

		if (pbrMetallicRoughness.baseColorFactor.size())
		{
			float metallicFactor = pbrMetallicRoughness.metallicFactor;
			float roughnessFactor = pbrMetallicRoughness.roughnessFactor;
			tfmat->m_params.g.m_metallicRoughnessValues = glm::vec4(metallicFactor, roughnessFactor, 0, 0);
			tfmat->m_params.g.m_baseColorFactor = pbrMetallicRoughness.baseColorFactor.size() ? tov4(pbrMetallicRoughness.baseColorFactor) : ones;
			if (pbrMetallicRoughness.baseColorTexture.index != -1)
			{
				tfmat->m_defines["MATERIAL_METALLICROUGHNESS"] = "1";
				textureIds["baseColorTexture"] = pbrMetallicRoughness.baseColorTexture.index;
				tfmat->m_defines["ID_baseTexCoord"] = std::to_string(pbrMetallicRoughness.baseColorTexture.texCoord);
			}
			if (pbrMetallicRoughness.metallicRoughnessTexture.index != -1) {
				textureIds["metallicRoughnessTexture"] = pbrMetallicRoughness.metallicRoughnessTexture.index;
				tfmat->m_defines["ID_metallicRoughnessTexCoord"] = std::to_string(pbrMetallicRoughness.metallicRoughnessTexture.texCoord);
			}
		}

	} while (0);
}

void GetSrgbAndCutOffOfImageGivenItsUse(int imageIndex, const std::vector<tinygltf::Material>* p, bool* pSrgbOut, float* pCutoff)
{
	auto& materials = *p;
	*pSrgbOut = false;
	*pCutoff = 1.0f; // no cutoff

	for (int m = 0; m < materials.size(); m++)
	{
		auto material = materials[m];
		if (material.pbrMetallicRoughness.baseColorTexture.index == imageIndex)
		{
			*pSrgbOut = true;
			*pCutoff = material.alphaCutoff;// ", 0.5);
			return;
		}
		// "extensions/KHR_materials_pbrSpecularGlossiness/specularGlossinessTexture/index"
		if (material.extensions.find("KHR_materials_pbrSpecularGlossiness") != material.extensions.end())
		{
			auto mpbrsg = material.extensions["KHR_materials_pbrSpecularGlossiness"];
			auto sgtidx = mpbrsg.Get("specularGlossinessTexture").Get("index").GetNumberAsInt();
			if (sgtidx == imageIndex)
			{
				*pSrgbOut = true;
				return;
			}
			// "extensions/KHR_materials_pbrSpecularGlossiness/diffuseTexture/index"
			auto dtidx = mpbrsg.Get("diffuseTexture").Get("index").GetNumberAsInt();
			if (dtidx == imageIndex)
			{
				*pSrgbOut = true;
				return;
			}
		}

		if (material.emissiveTexture.index == imageIndex)
		{
			*pSrgbOut = true;
			return;
		}
	}
}


bool get_ext_pbrSpecularGlossiness(tinygltf::ExtensionMap extm, const std::string& textureName, int* pIndex, int* pTexCoord)
{
	int index = -1;
	int texCoord = -1;
	if (extm.size())
	{
		auto extensions = extm;
		auto KHR_materials_pbrSpecularGlossinessIt = extensions.find("KHR_materials_pbrSpecularGlossiness");
		if (KHR_materials_pbrSpecularGlossinessIt != extensions.end())
		{
			auto pbrSpecularGlossiness = KHR_materials_pbrSpecularGlossinessIt->second;
			if (pbrSpecularGlossiness.Has(textureName))
			{
				auto t = pbrSpecularGlossiness.Get(textureName);

				if (t.Has("index"))
				{
					index = t.Get("index").GetNumberAsInt();
					if (index != -1)
					{
						if (t.Has("texCoord"))
						{
							texCoord = t.Get("texCoord").GetNumberAsInt();
						}
					}
				}
			}
		}
	}
	if (pIndex)
	{
		*pIndex = index;
	}

	if (pTexCoord)
	{
		*pTexCoord = texCoord;
	}

	return (index > -1);
}